/**
 * @file DeferredRenderer.cpp
 * @brief Implementa la logica de DeferredRenderer dentro del subsistema Rendering.
 * @ingroup rendering
 */
#include "Rendering/DeferredRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "Device.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "EngineUtilities/Utilities/LayoutBuilder.h"
#include "EngineUtilities/Utilities/Skybox.h"
#include "Rendering/Material.h"
#include "Rendering/MaterialInstance.h"
#include "Rendering/Mesh.h"

namespace {
constexpr unsigned int kGBufferTargetCount = 4;

/**
 * @brief Elige qué luz de la escena proyecta el (único) shadow map de este frame.
 * @param scene Escena del frame, con `lights` ya recolectadas por `BaseApp` (mezcla de
 *              Directional/Point/Spot, ver `LightComponent::resolve`).
 * @return La primera luz `Directional` encontrada; si no hay ninguna, la primera luz de
 *         cualquier tipo; si la escena no tiene luces, `nullptr`.
 *
 * @note [GameDev] Este motor solo calcula UN shadow map por frame (compartido por toda
 * la escena), así que necesita decidir cuál luz es "la que importa" para sombras. La
 * regla — preferir Directional — es la misma que usan la mayoría de motores para su
 * "sombra principal" (el sol/luna), porque una luz direccional cubre toda la escena con
 * un solo shadow map ortográfico; luces Point/Spot necesitarían proyecciones en
 * perspectiva (y Point, hasta 6 caras tipo cubemap) para cubrir su volumen de
 * influencia. Escalar a "sombras por cada luz" es posible pero cada una cuesta un pase
 * de render completo — es la razón por la que juegos AAA limitan cuántas luces
 * dinámicas pueden proyectar sombra simultáneamente (a menudo solo 1-4).
 */
const LightData*
findPrimaryShadowLight(const RenderScene& scene) {
	for (const LightData& light : scene.lights) {
		if (light.type == LightType::Directional) {
			return &light;
		}
	}

	return scene.lights.empty() ? nullptr : &scene.lights.front();
}

/**
 * @brief Empaqueta una `LightData` resuelta (ya con position/direction del Transform del
 * actor) en el slot `lightIndex` de los arreglos multi-luz de `CBPerFrame`.
 * @param buffer     CBPerFrame de este frame (se escribe in-place).
 * @param lightIndex Índice del slot [0, kMaxSceneLights).
 * @param light      Datos ya resueltos de la luz (ver `LightComponent::resolve`).
 *
 * @details Cada luz ocupa 3 `float4` (`LightPositionsRanges`, `LightColorsTypes`,
 * `LightDirectionsIntensities`) más un cuarto `float4` dedicado solo al ángulo del cono
 * (`LightSpotAngles`, usado únicamente por luces Spot) — un campo que no cabía en los
 * tres arreglos originales sin gastar un `float4` completo por un solo escalar.
 *
 * @note [GameDev] Antes de este cambio, `DeferredLighting.hlsl` NO LEÍA estos arreglos:
 * el pixel shader calculaba la iluminación entera con una sola dirección global
 * (`LightDir`), así que mover o cambiar el tipo de cualquier luz que no fuera "la luz
 * principal" no tenía ningún efecto visual. Es un recordatorio de que en un pipeline
 * CPU→GPU como este, tener el dato correctamente empaquetado en el constant buffer NO
 * garantiza que el shader lo use — la validación real solo se puede hacer verificando
 * ambos lados (el layout en C++ Y el cuerpo del shader que lo consume).
 */
void
writeLightToFrameBuffer(CBPerFrame& buffer, int lightIndex, const LightData& light) {
	const float range = light.range > 0.0f ? light.range : 10.0f;
	const EU::Vector3 lightColor = light.color * light.intensity;
	buffer.LightPositionsRanges[lightIndex] = XMFLOAT4(light.position.x, light.position.y, light.position.z, range);
	buffer.LightColorsTypes[lightIndex] = XMFLOAT4(lightColor.x, lightColor.y, lightColor.z, static_cast<float>(static_cast<int>(light.type)));
	buffer.LightDirectionsIntensities[lightIndex] = XMFLOAT4(light.direction.x, light.direction.y, light.direction.z, light.intensity);
	const float spotAngle = light.spotAngle > 0.0f ? light.spotAngle : XMConvertToRadians(30.0f);
	buffer.LightSpotAngles[lightIndex] = XMFLOAT4(spotAngle, 0.0f, 0.0f, 0.0f);
}
}

/**
 * @brief Crea, en orden, todos los recursos GPU del pipeline deferred: constant buffers,
 * depth-stencil states, shadow map, G-Buffer (4 MRTs), shaders de lighting, fullscreen
 * quad y blend states.
 * @param device Dispositivo D3D11 activo.
 * @param width  Ancho inicial del G-Buffer (normalmente el tamaño del viewport de editor).
 * @param height Alto inicial del G-Buffer.
 * @return `S_OK` si TODOS los sub-recursos se crearon; el primer `HRESULT` fallido en
 * caso contrario (early-return — no se intenta crear el resto).
 *
 * @note [GameDev] El orden de creación aquí no es arbitrario: los constant buffers y
 * depth-stencil states son baratos y no dependen de nada, así que van primero para fallar
 * rápido si el Device está en mal estado. `createShadowResources` y
 * `createGBufferResources` compilan shaders desde disco (`.hlsl`), que es la parte más
 * lenta y propensa a errores del init — por eso cada una loguea explícitamente OK/FAIL con
 * `LOG_MESSAGE`/`LOG_ERROR`, a diferencia de los buffers previos que solo devuelven el
 * `HRESULT`. Si vas a depurar un "pantalla negra" al arrancar, el Output de VS es el primer
 * lugar a mirar: si `createGBufferResources` o `createLightingResources` fallaron, ningún
 * pase de render subsecuente va a tener a dónde escribir ni qué shader usar.
 */
HRESULT
DeferredRenderer::init(Device& device, unsigned int width, unsigned int height) {
	m_renderWidth  = width;
	m_renderHeight = height;

	HRESULT hr = m_perFrameBuffer.init(device, sizeof(CBPerFrame));
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_perObjectBuffer.init(device, sizeof(CBPerObject));
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_perMaterialBuffer.init(device, sizeof(CBPerMaterial));
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_lightingDebugBuffer.init(device, sizeof(DeferredLightingDebugData));
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_transparentDepthStencil.init(device,
		true,
		D3D11_DEPTH_WRITE_MASK_ZERO,
		D3D11_COMPARISON_LESS_EQUAL);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_disabledDepthStencil.init(device,
		false,
		D3D11_DEPTH_WRITE_MASK_ZERO,
		D3D11_COMPARISON_ALWAYS);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_shadowDepthStencil.init(device,
		true,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS);
	if (FAILED(hr)) {
		return hr;
	}

	hr = createShadowResources(device);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "init", "createShadowResources OK");
	else LOG_ERROR("DeferredRenderer", "init", "createShadowResources FAIL hr=" + std::to_string(hr));
	if (FAILED(hr)) return hr;

	hr = createGBufferResources(device, m_renderWidth, m_renderHeight);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "init", "createGBufferResources OK " + std::to_string(m_renderWidth) + "x" + std::to_string(m_renderHeight));
	else LOG_ERROR("DeferredRenderer", "init", "createGBufferResources FAIL");
	if (FAILED(hr)) return hr;

	hr = createLightingResources(device);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "init", "createLightingResources OK");
	else LOG_ERROR("DeferredRenderer", "init", "createLightingResources FAIL");
	if (FAILED(hr)) return hr;

	hr = createFullScreenQuad(device);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "init", "createFullScreenQuad OK");
	else LOG_ERROR("DeferredRenderer", "init", "createFullScreenQuad FAIL");
	if (FAILED(hr)) return hr;

	hr = createBlendStates(device);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "init", "createBlendStates OK");
	else LOG_ERROR("DeferredRenderer", "init", "createBlendStates FAIL");
	if (FAILED(hr)) return hr;

	return S_OK;
}

/**
 * @brief Destruye y vuelve a crear los 4 G-Buffers al nuevo tamaño (p. ej. al redimensionar
 * la ventana del viewport de editor).
 * @param device Dispositivo D3D11.
 * @param width  Nuevo ancho, con piso de 64px.
 * @param height Nuevo alto, con piso de 64px.
 *
 * @note [GameDev] El piso de 64px evita crear texturas de 0×0 (o negativas si algún caller
 * pasa un tamaño de viewport todavía no inicializado) — D3D11 rechaza esas dimensiones con
 * un `HRESULT` de fallo, y como `resize()` no propaga el `HRESULT` de
 * `createGBufferResources`, ese fallo pasaría silencioso hasta el próximo `render()`, que
 * intentaría dibujar sobre RTVs nulos. El shadow map NO se recrea aquí: su resolución
 * (`m_shadowMapSize`) es fija e independiente del tamaño de pantalla, a diferencia del
 * G-Buffer que debe cubrir 1:1 los píxeles del viewport.
 */
void
DeferredRenderer::resize(Device& device, unsigned int width, unsigned int height) {
	if (width < 64) width = 64;
	if (height < 64) height = 64;

	m_renderWidth = width;
	m_renderHeight = height;
	createGBufferResources(device, width, height);
}

/**
 * @brief Punto de entrada del frame: clasifica objetos, sube CBPerFrame, dibuja el shadow
 * map y ejecuta el pipeline deferred completo contra `viewportPass`.
 * @param deviceContext Contexto D3D11 del frame.
 * @param camera        Cámara activa (usada para ordenar transparencias y armar CBPerFrame).
 * @param scene         Escena con actores, luces y skybox del frame.
 * @param viewportPass  Target final (color + depth) donde termina escribiendo el pipeline.
 *
 * @note [GameDev] Este método solo orquesta el ORDEN de las fases — la lógica de cada fase
 * vive en su propio método (`renderShadowPass`, `renderSceneToTarget` → geometry/lighting/
 * skybox/transparent). Mantenerlo así de corto hace que el flujo completo del frame se lea
 * de un vistazo: primero se decide QUÉ se va a dibujar (`buildQueues`) y CÓMO está iluminado
 * (`updatePerFrame`), y solo después se emiten draw calls. El shadow pass se ejecuta ANTES
 * que `renderSceneToTarget` porque el G-Buffer y el lighting pass, más adelante, necesitan
 * leer el shadow map ya completo desde el slot t6.
 */
void
DeferredRenderer::render(DeviceContext& deviceContext,
	const Camera& camera,
	RenderScene& scene,
	EditorViewportPass& viewportPass) {
	buildQueues(scene, camera);
	updatePerFrame(camera, scene, deviceContext);

	renderShadowPass(deviceContext);
	renderSceneToTarget(deviceContext, scene, viewportPass, true, camera);
}

/** @brief Libera, en orden inverso a la creación, todos los recursos GPU del renderer. */
void
DeferredRenderer::destroy() {
	m_opaqueQueue.clear();
	m_transparentQueue.clear();

	SAFE_RELEASE(m_defaultWhiteSRV);
	SAFE_RELEASE(m_defaultFlatNormalSRV);
	SAFE_RELEASE(m_alphaBlendState);
	SAFE_RELEASE(m_opaqueBlendState);
	SAFE_RELEASE(m_additiveBlendState);
	SAFE_RELEASE(m_premultipliedBlendState);

	m_fullscreenIndexBuffer.destroy();
	m_fullscreenVertexBuffer.destroy();

	m_gBufferEmissiveAlphaRTV.destroy();
	m_gBufferEmissiveAlphaSRV.destroy();
	m_gBufferEmissiveAlphaTexture.destroy();
	m_gBufferWorldAoRTV.destroy();
	m_gBufferWorldAoSRV.destroy();
	m_gBufferWorldAoTexture.destroy();
	m_gBufferNormalRoughnessRTV.destroy();
	m_gBufferNormalRoughnessSRV.destroy();
	m_gBufferNormalRoughnessTexture.destroy();
	m_gBufferAlbedoMetallicRTV.destroy();
	m_gBufferAlbedoMetallicSRV.destroy();
	m_gBufferAlbedoMetallicTexture.destroy();

	m_fullscreenRasterizer.destroy();
	m_lightingSampler.destroy();
	m_deferredLightingShader.destroy();
	m_gBufferShader.destroy();

	m_transparentDepthStencil.destroy();
	m_disabledDepthStencil.destroy();
	m_shadowDepthStencil.destroy();
	m_perMaterialBuffer.destroy();
	m_lightingDebugBuffer.destroy();
	m_perObjectBuffer.destroy();
	m_perFrameBuffer.destroy();

	m_shadowRasterizer.destroy();
	m_shadowShader.destroy();
	m_shadowDSV.destroy();
	m_shadowDepthSRV.destroy();
	m_shadowDepthTexture.destroy();
}

/**
 * @brief Clasifica los objetos de la escena en las colas `m_opaqueQueue` /
 * `m_transparentQueue` y las ordena para minimizar cambios de estado y errores de blending.
 * @param scene  Escena del frame (ya separada en `opaqueObjects` / `transparentObjects` por
 *               `RenderScene` — este método solo copia punteros y ordena, no reclasifica).
 * @param camera Recibida por firma de la interfaz pero sin uso aquí: el orden por distancia
 *               ya usa `distanceToCamera`, precalculado en otro punto del frame.
 *
 * @note [GameDev] Los opacos se ordenan por `materialInstance` (no por distancia) porque en
 * un G-Buffer pass no hay overdraw que evitar con el orden de dibujo — el depth test ya
 * garantiza que solo el píxel más cercano sobrevive — así que el único costo real es cambiar
 * de shader/textura entre draw calls; agrupar por material minimiza esos cambios de estado
 * en la GPU. Los transparentes, en cambio, se ordenan back-to-front (`>` en vez de `<`)
 * porque SÍ hay overdraw real: sin depth write, cada capa transparente debe componerse
 * sobre lo que ya está detrás, y dibujar en el orden equivocado produce blending incorrecto
 * (un vidrio rojo pintado antes que uno azul detrás se ve "al revés").
 *
 * @note [GameDev] Este back-to-front sort es "por objeto": un solo `distanceToCamera`
 * representa a TODO un `RenderObject`, incluidas todas sus submallas. Es la técnica de
 * transparencia más simple que existe (sorted/blended transparency) y es exactamente lo
 * que falla cuando un objeto transparente no es una forma delgada/convexa (como un vidrio)
 * sino una malla cerrada con varias partes (confirmado con Kirby durante el desarrollo del
 * Material Editor: sus submallas internas, normalmente ocultas, se dibujan en orden de
 * array — no de profundidad real — entre sí). El motor no implementa ninguna forma de
 * "Order-Independent Transparency" (OIT) — depth peeling, per-pixel linked lists (A-buffer),
 * ni siquiera un sort por submesh — que son las técnicas reales para resolver esto en
 * geometría arbitraria; ver la limitación documentada al final de esta función.
 */
void
DeferredRenderer::buildQueues(RenderScene& scene, const Camera& camera) {
	(void)camera;
	m_opaqueQueue.clear();
	m_transparentQueue.clear();

	for (auto& object : scene.opaqueObjects) {
		m_opaqueQueue.push_back(&object);
	}

	for (auto& object : scene.transparentObjects) {
		m_transparentQueue.push_back(&object);
	}

	std::sort(m_opaqueQueue.begin(), m_opaqueQueue.end(),
		[](const RenderObject* lhs, const RenderObject* rhs) {
			if (lhs->materialInstance != rhs->materialInstance) {
				return lhs->materialInstance < rhs->materialInstance;
			}
			return lhs->distanceToCamera < rhs->distanceToCamera;
		});

	std::sort(m_transparentQueue.begin(), m_transparentQueue.end(),
		[](const RenderObject* lhs, const RenderObject* rhs) {
			return lhs->distanceToCamera > rhs->distanceToCamera;
		});

	// LIMITACION CONOCIDA: este orden es por-objeto (un distanceToCamera para todo el
	// RenderObject), no por-submesh ni por-triangulo. renderForwardObject dibuja las
	// submallas de un mismo objeto en su orden de array, sin resolver profundidad entre
	// ellas (m_transparentDepthStencil tiene DEPTH_WRITE_MASK_ZERO — necesario para que el
	// blending funcione, pero significa que las propias partes del objeto dejan de
	// ocluirse entre si). Para una forma simple/casi convexa (el vidrio de SciFiToad) esto
	// no se nota; para una malla cerrada con varias partes (ej. Kirby, si se le asigna un
	// material Transparent) las partes internas normalmente ocultas pueden dibujarse
	// encima de las que deberian taparlas — confirmado manualmente durante el desarrollo
	// del Material Editor. Arreglarlo de verdad requiere ordenar submallas (o triangulos)
	// por objeto, no solo objetos entre si; queda como trabajo futuro, no un parche chico.
}

/**
 * @brief Rellena `m_cbPerFrame` (matrices, cámara, luz "principal" legacy + arreglos
 * multi-luz) y lo sube a GPU en el slot b0.
 * @param camera Cámara activa: aporta View/Projection y CameraPos.
 * @param scene  Fuente de las luces del frame (`scene.lights`, ya resueltas por
 *               `LightComponent::resolve` en `BaseApp`).
 * @param deviceContext Contexto D3D11 usado para el `Map/Unmap` del buffer.
 *
 * @note [GameDev] Este método escribe la luz "principal" DOS VECES: una vez en los campos
 * legacy planos (`LightDir`, `LightColor`, `LightPosition`...) y otra vez dentro de los
 * arreglos `LightPositionsRanges[0]`/etc. vía `writeLightToFrameBuffer`. Los campos legacy
 * ya no los lee `DeferredLighting.hlsl` (que solo usa los arreglos), pero se mantienen
 * porque `ForwardRenderer` y el shadow pass (`ShadowMap.hlsl`) sí dependen de `LightDir`
 * para orientar el shadow frustum — eliminar esos campos rompería el otro renderer. Antes
 * de tocar el layout de `CBPerFrame`, revisa ambos shaders y ambos renderers.
 * @note [GameDev] Los arreglos multi-luz se limpian a cero ANTES del `if
 * (!scene.lights.empty())` — sin este reset, una escena que pasa de 3 luces a 1 dejaría
 * "luces fantasma" con los datos del frame anterior en los slots [1] y [2], porque
 * `LightCount` le dice al shader hasta dónde iterar pero los datos viejos seguirían en el
 * buffer si no se sobreescriben explícitamente.
 */
void
DeferredRenderer::updatePerFrame(const Camera& camera,
	const RenderScene& scene,
	DeviceContext& deviceContext) {
	updateLightMatrices(camera, scene);
	XMStoreFloat4x4(&m_cbPerFrame.View, XMMatrixTranspose(camera.getView()));
	XMStoreFloat4x4(&m_cbPerFrame.Projection, XMMatrixTranspose(camera.getProj()));
	m_cbPerFrame.CameraPos = camera.getPosition();
	m_cbPerFrame.LightDir = EU::Vector3(0.0f, -1.0f, 0.0f);
	m_cbPerFrame.LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);
	m_cbPerFrame.LightPosition = EU::Vector3(0.0f, 3.0f, 0.0f);
	m_cbPerFrame.LightRange = 10.0f;
	m_cbPerFrame.LightType = static_cast<int>(LightType::Directional);
	m_cbPerFrame.LightCount = 0;
	for (int lightIndex = 0; lightIndex < kMaxSceneLights; ++lightIndex) {
		m_cbPerFrame.LightPositionsRanges[lightIndex] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_cbPerFrame.LightColorsTypes[lightIndex] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		m_cbPerFrame.LightDirectionsIntensities[lightIndex] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
		m_cbPerFrame.LightSpotAngles[lightIndex] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	if (!scene.lights.empty()) {
		const LightData* primaryShadowLight = findPrimaryShadowLight(scene);
		int lightCount = 0;
		if (primaryShadowLight) {
			writeLightToFrameBuffer(m_cbPerFrame, lightCount++, *primaryShadowLight);
		}
		for (const LightData& light : scene.lights) {
			if (&light == primaryShadowLight || lightCount >= kMaxSceneLights) {
				continue;
			}
			writeLightToFrameBuffer(m_cbPerFrame, lightCount++, light);
		}
		m_cbPerFrame.LightCount = lightCount;

		const LightData& mainLight = primaryShadowLight ? *primaryShadowLight : scene.lights[0];
		m_cbPerFrame.LightDir = mainLight.direction;
		m_cbPerFrame.LightColor = mainLight.color * mainLight.intensity;
		m_cbPerFrame.LightPosition = mainLight.position;
		m_cbPerFrame.LightRange = mainLight.range > 0.0f ? mainLight.range : 10.0f;
		m_cbPerFrame.LightType = static_cast<int>(mainLight.type);
	}

	m_perFrameBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerFrame, 0, 0);
}

/**
 * @brief Calcula `LightViewProjection` (matriz combinada view+proj de la luz principal)
 * usada tanto por `renderShadowPass` (para escribir el shadow map) como por
 * `ComputeShadow()` en `DeferredLighting.hlsl` (para proyectar cada píxel al espacio de
 * la luz y compararlo contra el shadow map).
 * @param camera Cámara activa: el frustum de sombra se centra en `camera.getPosition()`,
 *               no en el centro de la escena.
 * @param scene  Se usa solo para obtener la dirección de la luz principal vía
 *               `findPrimaryShadowLight`.
 *
 * @note [GameDev] Este motor usa un shadow frustum ortográfico fijo (40×40 unidades, near
 * 1, far 80) centrado en la cámara, en vez de un "cascade" que ajusta dinámicamente al
 * frustum de vista (Cascaded Shadow Maps / CSM). Es la técnica más simple posible para
 * sombras direccionales: funciona bien para escenas de tamaño moderado donde 40 unidades
 * cubren lo visible, pero en escenas grandes vas a ver sombras "cortarse" en los bordes del
 * área cubierta, o perder resolución de sombra (todo el shadow map de 2048² repartido en un
 * área demasiado grande). CSM resuelve esto dividiendo el frustum de cámara en 2-4 zonas de
 * distancia, cada una con su propio shadow map ajustado — mucho más código pero sombras
 * nítidas tanto cerca como lejos.
 * @note [GameDev] El `worldUp` se cambia a `(0,0,1)` cuando la dirección de luz es casi
 * paralela al eje Y — sin este chequeo, `XMMatrixLookAtLH` produciría una matriz degenerada
 * (cross product de vectores paralelos = vector cero) para una luz apuntando casi
 * directamente hacia abajo, un caso común para "el sol al mediodía".
 */
void
DeferredRenderer::updateLightMatrices(const Camera& camera, const RenderScene& scene) {
	EU::Vector3 lightDir = EU::Vector3(0.0f, -1.0f, 0.0f);
	const LightData* primaryShadowLight = findPrimaryShadowLight(scene);
	if (primaryShadowLight) {
		lightDir = primaryShadowLight->direction;
	}

	XMVECTOR lightDirVec = XMVector3Normalize(XMVectorSet(lightDir.x, lightDir.y, lightDir.z, 0.0f));
	XMVECTOR cameraPos = XMVectorSet(camera.getPosition().x, camera.getPosition().y, camera.getPosition().z, 1.0f);
	XMVECTOR lightTarget = cameraPos;
	XMVECTOR lightEye = XMVectorSubtract(lightTarget, XMVectorScale(lightDirVec, 35.0f));
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	if (fabsf(XMVectorGetX(XMVector3Dot(lightDirVec, worldUp))) > 0.98f) {
		worldUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	XMMATRIX lightView = XMMatrixLookAtLH(lightEye, lightTarget, worldUp);
	XMMATRIX lightProjection = XMMatrixOrthographicLH(40.0f, 40.0f, 1.0f, 80.0f);
	XMStoreFloat4x4(&m_cbPerFrame.LightViewProjection, XMMatrixTranspose(lightView * lightProjection));
}

/**
 * @brief Ejecuta las 4 fases restantes del pipeline deferred (geometry → lighting →
 * skybox → transparent) contra un `EditorViewportPass` arbitrario. Es el cuerpo compartido
 * que usan tanto `render()` (contra el viewport principal) como cualquier vista de debug
 * que quiera reutilizar el mismo pipeline contra otro target.
 * @param deviceContext Contexto D3D11.
 * @param scene         Escena a dibujar.
 * @param targetPass    Target de color+depth ya redimensionado a la resolución deseada.
 * @param applyShadows  Si es `false`, el lighting pass ignora el shadow map (ver
 *                      `m_applyShadows` y su uso en `renderLightingPass`/`renderTransparentPass`).
 * @param camera        Cámara activa; se reenvía a `renderSkyboxPass`.
 *
 * @note [GameDev] El orden geometry → lighting → skybox → transparent es la secuencia
 * canónica de un deferred renderer y no es intercambiable: el G-Buffer debe estar completo
 * ANTES de que el lighting pass lo lea (de ahí que `bindGBufferTargets` limpie los SRVs
 * primero — no se puede tener la misma textura enlazada como RTV y SRV al mismo tiempo,
 * D3D11 lo rechaza). El skybox va DESPUÉS del lighting pass porque este último es un
 * fullscreen quad que escribe TODOS los píxeles sin depth test — un skybox dibujado antes
 * quedaría completamente sobrescrito. Y las transparencias van al final porque, al no
 * escribir profundidad, dependen de que el color de fondo (opacos + skybox) ya esté
 * resuelto para poder mezclarse (`blend`) sobre él correctamente.
 */
void
DeferredRenderer::renderSceneToTarget(DeviceContext& deviceContext,
	RenderScene& scene,
	EditorViewportPass& targetPass,
	bool applyShadows,
	const Camera& camera) {
	const float clearColor[4] = { 0.10f, 0.10f, 0.10f, 1.0f };

	targetPass.begin(deviceContext, clearColor);
	targetPass.setViewport(deviceContext);

	m_applyShadows = applyShadows;
	bindGBufferTargets(deviceContext, targetPass.getDSV());
	renderGeometryPass(deviceContext);

	bindFinalTarget(deviceContext, targetPass.getRTV(), targetPass.getDSV());
	renderLightingPass(deviceContext);
	renderSkyboxPass(deviceContext, scene, camera);
	renderTransparentPass(deviceContext);
}

/**
 * @brief Enlaza los 4 RTVs del G-Buffer como Multiple Render Targets (MRT) activos y los
 * limpia con valores por-canal específicos a cada uno.
 * @param deviceContext   Contexto D3D11.
 * @param depthStencilView DSV compartido por los 4 RTVs (mismo tamaño, mismo formato de
 *                          profundidad que el target final).
 *
 * @note [GameDev] Cada RT se limpia con un color distinto porque cada canal tiene un
 * "valor neutro" diferente: RT0 (albedo) a negro transparente es el marcador que usa
 * `DeferredLighting.hlsl` para detectar píxeles de fondo (`dot(rt0,rt0) < 0.001`). RT1
 * (normal codificada) se limpia a `(0.5, 0.5, 1.0)`, que decodificado (`*2-1`) da
 * `(0,0,1)` — una normal "hacia la cámara" válida en vez de un vector cero que rompería
 * cualquier `normalize()` posterior. RT2 (world pos + AO) limpia el canal alfa (AO) a 1.0
 * para que el fondo no aparezca con oclusión ambiental total. Este detalle importa: un
 * clear "genérico" a `(0,0,0,0)` en todos los RTs produciría normales inválidas y errores
 * de NaN al normalizar un vector cero.
 */
void
DeferredRenderer::bindGBufferTargets(DeviceContext& deviceContext, ID3D11DepthStencilView* depthStencilView) {
	clearDeferredSRVs(deviceContext);

	ID3D11RenderTargetView* renderTargets[kGBufferTargetCount] = {
		m_gBufferAlbedoMetallicRTV.get(),
		m_gBufferNormalRoughnessRTV.get(),
		m_gBufferWorldAoRTV.get(),
		m_gBufferEmissiveAlphaRTV.get()
	};

	const float clear0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float clear1[4] = { 0.5f, 0.5f, 1.0f, 0.0f };
	const float clear2[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float clear3[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	deviceContext.ClearRenderTargetView(renderTargets[0], clear0);
	deviceContext.ClearRenderTargetView(renderTargets[1], clear1);
	deviceContext.ClearRenderTargetView(renderTargets[2], clear2);
	deviceContext.ClearRenderTargetView(renderTargets[3], clear3);
	deviceContext.ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	deviceContext.OMSetRenderTargets(kGBufferTargetCount, renderTargets, depthStencilView);
	deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);
}

/** @brief Restaura el target final (single RTV, ej. back buffer del viewport) tras el geometry pass. */
void
DeferredRenderer::bindFinalTarget(DeviceContext& deviceContext,
	ID3D11RenderTargetView* renderTargetView,
	ID3D11DepthStencilView* depthStencilView) {
	ID3D11RenderTargetView* renderTargets[1] = { renderTargetView };
	deviceContext.OMSetRenderTargets(1, renderTargets, depthStencilView);
	deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);
}

/**
 * @brief Desenlaza los 8 primeros slots de SRV del Pixel Shader.
 *
 * @note [GameDev] Es el fix al "hazard" clásico de deferred rendering: una textura no puede
 * estar enlazada simultáneamente como Render Target (para escribirla) y como Shader
 * Resource (para leerla) — D3D11 detecta el conflicto y automáticamente desenlaza el SRV
 * (con un warning en el debug layer), pero confiar en ese comportamiento implícito es
 * frágil. Este método se llama explícitamente ANTES de volver a usar los G-Buffers como
 * RTVs (`bindGBufferTargets`) y de nuevo al terminar de leerlos en el lighting pass
 * (`renderLightingPass`), dejando el estado del pipeline predecible en vez de depender de
 * que el runtime "adivine" la intención.
 */
void
DeferredRenderer::clearDeferredSRVs(DeviceContext& deviceContext) {
	ID3D11ShaderResourceView* nullViews[8] = {};
	deviceContext.PSSetShaderResources(0, 8, nullViews);
}

/** @brief Sube CBPerFrame al VS (slot b0) y dibuja toda la cola de opacos al G-Buffer. */
void
DeferredRenderer::renderGeometryPass(DeviceContext& deviceContext) {
	m_perFrameBuffer.render(deviceContext, 0, 1, false);

	for (const RenderObject* object : m_opaqueQueue) {
		if (!object) {
			continue;
		}
		renderGeometryObject(deviceContext, *object);
	}
}

/**
 * @brief Dibuja un actor al G-Buffer, submesh por submesh, resolviendo el `MaterialInstance`
 * correcto por slot y rellenando texturas ausentes con los defaults neutros de la clase.
 * @param deviceContext Contexto D3D11.
 * @param object        Objeto de render con su malla y su(s) `MaterialInstance`.
 *
 * @note [GameDev] Este método (y su gemelo `renderForwardObject`) resuelve el
 * `MaterialInstance` POR SUBMESH, no uno solo por objeto: `object.materialInstances[submesh.materialSlot]`
 * permite que un único modelo FBX con varios materiales (ej. un personaje con piel + tela +
 * metal) use el material correcto en cada parte, cayendo al `object.materialInstance`
 * genérico solo si ese slot específico no tiene uno asignado. Es el patrón estándar de
 * "material slots" que usan Blender/Unity/Unreal para malla multi-material.
 * @note [GameDev] Solo se dibujan materiales que NO son `MaterialDomain::Transparent` — los
 * transparentes se saltan aquí a propósito porque el G-Buffer no puede representar
 * translucidez (cada píxel solo guarda UN valor de albedo/normal/profundidad, no una mezcla
 * de "lo que hay detrás"); por eso el pipeline los dibuja aparte en `renderTransparentPass`,
 * en forward, después de que el resultado deferred opaco ya está resuelto.
 */
void
DeferredRenderer::renderGeometryObject(DeviceContext& deviceContext, const RenderObject& object) {
	if (!object.mesh || (!object.materialInstance && object.materialInstances.empty())) {
		return;
	}

	deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<Submesh>& submeshes = object.mesh->getSubmeshes();
	for (Submesh& submesh : submeshes) {
		MaterialInstance* materialInstance = object.materialInstance;
		if (submesh.materialSlot < object.materialInstances.size() &&
			object.materialInstances[submesh.materialSlot]) {
			materialInstance = object.materialInstances[submesh.materialSlot];
		}

		if (!materialInstance) {
			continue;
		}

		Material* material = materialInstance->getMaterial();
		if (!material) {
			continue;
		}
		if (material->getDomain() == MaterialDomain::Transparent) {
			continue;
		}

		if (material->getRasterizerState()) {
			material->getRasterizerState()->render(deviceContext);
		}

		if (material->getDepthStencilState()) {
			material->getDepthStencilState()->render(deviceContext, 0, false);
		}

		m_gBufferShader.render(deviceContext);
		if (material->getSamplerState()) {
			material->getSamplerState()->render(deviceContext, 0, 1);
		}
		XMMATRIX submeshWorld = XMLoadFloat4x4(&submesh.localTransform) * object.world;
		XMStoreFloat4x4(&m_cbPerObject.World, XMMatrixTranspose(submeshWorld));
		m_perObjectBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerObject, 0, 0);
		m_perObjectBuffer.render(deviceContext, 1, 1, false);

		materialInstance->bindTextures(deviceContext);
		bindTextureFallbacks(deviceContext, materialInstance);

		const MaterialParams& params = materialInstance->getParams();
		m_cbPerMaterial.BaseColor = computeTintedBaseColor(params, object.tint);
		m_cbPerMaterial.Metallic = params.metallic;
		m_cbPerMaterial.Roughness = params.roughness;
		m_cbPerMaterial.AO = params.ao;
		m_cbPerMaterial.NormalScale = params.normalScale;
		m_cbPerMaterial.EmissiveStrength = params.emissiveStrength;
		m_cbPerMaterial.AlphaCutoff = material->getDomain() == MaterialDomain::Masked ? params.alphaCutoff : 0.0f;
		m_cbPerMaterial.UVTiling = params.uvTiling;
		m_cbPerMaterial.UVOffset = params.uvOffset;
		m_perMaterialBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerMaterial, 0, 0);
		m_perMaterialBuffer.render(deviceContext, 2, 1, true);

		submesh.vertexBuffer.render(deviceContext, 0, 1);
		submesh.indexBuffer.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
		deviceContext.DrawIndexed(submesh.indexCount, submesh.startIndex, 0);
	}
}

/**
 * @brief Fullscreen quad: enlaza los 4 G-Buffers + shadow map como SRVs y ejecuta
 * `DeferredLighting.hlsl` una sola vez sobre toda la pantalla.
 *
 * @note [GameDev] Este es EL paso que justifica la existencia del Deferred Rendering: en
 * vez de que cada objeto recalcule iluminación en su propio pixel shader (pagando el costo
 * de N_luces por cada triángulo dibujado, incluso los que luego quedan tapados por otros
 * más cercanos), aquí se calcula la iluminación UNA VEZ POR PÍXEL FINAL, leyendo del
 * G-Buffer ya resuelto. El costo pasa de "por triángulo × por luz" a "por píxel × por luz",
 * lo cual es más barato cuantas más luces o más overdraw tenga la escena.
 * @note [GameDev] Si `m_applyShadows` es `false` (vista de debug), el slot t6 se limpia a
 * `nullptr` en vez de dejar el shadow map de un frame viejo enlazado — sin esto,
 * `ComputeShadow()` en el shader seguiría leyendo sombras "congeladas" de la última vez que
 * el shadow pass corrió, dando resultados inconsistentes con el resto de la vista de debug.
 * @note [GameDev] `clearDeferredSRVs` se llama tanto ANTES (para poder leer los G-Buffers
 * sin conflicto RTV/SRV) como DESPUÉS (para dejarlos libres y que el próximo frame los
 * pueda volver a usar como render targets) — ver el comentario en `clearDeferredSRVs`.
 */
void
DeferredRenderer::renderLightingPass(DeviceContext& deviceContext) {
	clearDeferredSRVs(deviceContext);

	ID3D11ShaderResourceView* gBufferResources[4] = {
		m_gBufferAlbedoMetallicSRV.m_textureFromImg,
		m_gBufferNormalRoughnessSRV.m_textureFromImg,
		m_gBufferWorldAoSRV.m_textureFromImg,
		m_gBufferEmissiveAlphaSRV.m_textureFromImg
	};

	deviceContext.PSSetShaderResources(0, 4, gBufferResources);
	if (m_applyShadows && m_shadowDepthSRV.m_textureFromImg) {
		deviceContext.PSSetShaderResources(6, 1, &m_shadowDepthSRV.m_textureFromImg);
	}
	else {
		ID3D11ShaderResourceView* nullShadowSRV[1] = { nullptr };
		deviceContext.PSSetShaderResources(6, 1, nullShadowSRV);
	}
	m_disabledDepthStencil.render(deviceContext, 0, false);
	m_fullscreenRasterizer.render(deviceContext);
	m_lightingSampler.render(deviceContext, 0, 1);
	m_deferredLightingShader.render(deviceContext);
	m_perFrameBuffer.render(deviceContext, 0, 1, true);
	m_lightingDebugData.DebugViewMode = m_shadowFactorDebugEnabled ? 1 : m_deferredDebugViewMode;
	m_lightingDebugData.ShadowStrength = 1.0f;
	m_lightingDebugBuffer.update(deviceContext, nullptr, 0, nullptr, &m_lightingDebugData, 0, 0);
	m_lightingDebugBuffer.render(deviceContext, 1, 1, true);

	deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_fullscreenVertexBuffer.render(deviceContext, 0, 1);
	m_fullscreenIndexBuffer.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
	deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);
	deviceContext.DrawIndexed(6, 0, 0);

	clearDeferredSRVs(deviceContext);
}

/** @brief Dibuja el skybox (si existe) tras el lighting pass — ver doc en el header para el orden. */
void
DeferredRenderer::renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene, const Camera& camera) {
	if (!scene.skybox) {
		return;
	}
	scene.skybox->render(deviceContext, camera);
}

/**
 * @brief Dibuja la cola de transparentes en FORWARD (no deferred) sobre el resultado ya
 * iluminado, eligiendo el blend state según el `BlendMode` de cada material.
 *
 * @note [GameDev] Que el pipeline sea "Deferred Rendering" no significa que TODO se dibuje
 * deferred — la mayoría de motores híbridos (incluido este) reservan el G-Buffer solo para
 * opacos y resuelven transparencias en un pase forward tradicional al final, porque el
 * G-Buffer no puede almacenar "lo que hay detrás de un vidrio" (cada texel es un solo
 * valor). El costo es que las transparencias no reciben las optimizaciones del deferred
 * (siguen pagando O(N_luces) por objeto en su propio pixel shader), pero a cambio sí pueden
 * hacer blending real, cosa que el G-Buffer no permite.
 */
void
DeferredRenderer::renderTransparentPass(DeviceContext& deviceContext) {
	m_perFrameBuffer.render(deviceContext, 0, 1, true);
	if (m_applyShadows && m_shadowDepthSRV.m_textureFromImg) {
		deviceContext.PSSetShaderResources(6, 1, &m_shadowDepthSRV.m_textureFromImg);
	}
	else {
		ID3D11ShaderResourceView* nullShadowSRV[1] = { nullptr };
		deviceContext.PSSetShaderResources(6, 1, nullShadowSRV);
	}

	for (const RenderObject* object : m_transparentQueue) {
		if (!object) {
			continue;
		}

		Material* material = object->materialInstance ? object->materialInstance->getMaterial() : nullptr;
		deviceContext.OMSetBlendState(resolveBlendState(material), m_blendFactor, 0xffffffff);
		renderForwardObject(deviceContext, *object, RenderPassType::Transparent);
	}

	deviceContext.OMSetBlendState(m_opaqueBlendState, m_blendFactor, 0xffffffff);
}

/**
 * @brief Dibuja un objeto en forward (shader propio del material, no el shader de G-Buffer),
 * filtrando submeshes por dominio según `passType` (Opaque u.Transparent).
 * @param deviceContext Contexto D3D11.
 * @param object        Objeto a dibujar.
 * @param passType       `RenderPassType::Transparent` procesa solo submeshes con
 *                       `MaterialDomain::Transparent`; cualquier otro valor hace lo inverso.
 *
 * @note [GameDev] Aunque hoy solo `renderTransparentPass` llama a este método, está separado
 * de `renderGeometryObject` porque el camino forward usa `material->getShader()` (el shader
 * PROPIO de cada material) en vez del `m_gBufferShader` fijo del pase deferred — un
 * material transparente puede necesitar lógica de PS distinta (ej. refracción, distorsión)
 * que no tendría sentido compilar contra el layout de MRTs del G-Buffer.
 */
void
DeferredRenderer::renderForwardObject(DeviceContext& deviceContext,
	const RenderObject& object,
	RenderPassType passType) {
	if (!object.mesh || (!object.materialInstance && object.materialInstances.empty())) {
		return;
	}

	deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<Submesh>& submeshes = object.mesh->getSubmeshes();
	for (Submesh& submesh : submeshes) {
		MaterialInstance* materialInstance = object.materialInstance;
		if (submesh.materialSlot < object.materialInstances.size() &&
			object.materialInstances[submesh.materialSlot]) {
			materialInstance = object.materialInstances[submesh.materialSlot];
		}

		if (!materialInstance) {
			continue;
		}

		Material* material = materialInstance->getMaterial();
		if (!material) {
			continue;
		}
		if (passType == RenderPassType::Transparent &&
			material->getDomain() != MaterialDomain::Transparent) {
			continue;
		}
		if (passType != RenderPassType::Transparent &&
			material->getDomain() == MaterialDomain::Transparent) {
			continue;
		}

		if (material->getRasterizerState()) {
			material->getRasterizerState()->render(deviceContext);
		}

		if (passType == RenderPassType::Transparent) {
			deviceContext.OMSetBlendState(resolveBlendState(material), m_blendFactor, 0xffffffff);
			m_transparentDepthStencil.render(deviceContext, 0, false);
		}
		else if (material->getDepthStencilState()) {
			material->getDepthStencilState()->render(deviceContext, 0, false);
		}

		if (material->getShader()) {
			material->getShader()->render(deviceContext);
		}

		if (material->getSamplerState()) {
			material->getSamplerState()->render(deviceContext, 0, 1);
		}

		XMMATRIX submeshWorld = XMLoadFloat4x4(&submesh.localTransform) * object.world;
		XMStoreFloat4x4(&m_cbPerObject.World, XMMatrixTranspose(submeshWorld));
		m_perObjectBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerObject, 0, 0);
		m_perObjectBuffer.render(deviceContext, 1, 1, true);

		materialInstance->bindTextures(deviceContext);
		// Mismo relleno de defaults que renderGeometryObject — sin esto, un MaterialInstance
		// transparente sin textura de albedo asignada (ej. un material recien creado en el
		// Material Editor) samplea (0,0,0,0) en HLSL: albedo.a queda en 0 sin importar el
		// BaseColor.a configurado, y el objeto sale invisible/negro.
		bindTextureFallbacks(deviceContext, materialInstance);

		const MaterialParams& params = materialInstance->getParams();
		m_cbPerMaterial.BaseColor = computeTintedBaseColor(params, object.tint);
		m_cbPerMaterial.Metallic = params.metallic;
		m_cbPerMaterial.Roughness = params.roughness;
		m_cbPerMaterial.AO = params.ao;
		m_cbPerMaterial.NormalScale = params.normalScale;
		m_cbPerMaterial.EmissiveStrength = params.emissiveStrength;
		m_cbPerMaterial.AlphaCutoff = material->getDomain() == MaterialDomain::Masked ? params.alphaCutoff : 0.0f;
		m_cbPerMaterial.UVTiling = params.uvTiling;
		m_cbPerMaterial.UVOffset = params.uvOffset;
		m_perMaterialBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerMaterial, 0, 0);
		m_perMaterialBuffer.render(deviceContext, 2, 1, true);

		submesh.vertexBuffer.render(deviceContext, 0, 1);
		submesh.indexBuffer.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
		deviceContext.DrawIndexed(submesh.indexCount, submesh.startIndex, 0);
	}
}

/**
 * @brief Rellena con SRVs de default (blanco / normal plano) cualquier slot t0-t4 sin
 * textura asignada. `bindTextures()` deja sin enlazar (null) cualquier slot cuyo
 * `MaterialInstance` no tenga esa textura; sin un default, HLSL muestrea (0,0,0,0) para
 * esos slots — el t0 (albedo) en particular rompe la detección de fondo del lighting pass
 * en el pase opaco, y deja alpha en 0 (invisible) en el pase transparente.
 * @note Usado tanto por `renderGeometryObject` como por `renderForwardObject`.
 */
void
DeferredRenderer::bindTextureFallbacks(DeviceContext& deviceContext, MaterialInstance* materialInstance) {
	auto bindFallback = [&](UINT slot, Texture* tex, ID3D11ShaderResourceView* fallback) {
		if (fallback && (!tex || !tex->srv())) {
			deviceContext.m_deviceContext->PSSetShaderResources(slot, 1, &fallback);
		}
	};
	bindFallback(0, materialInstance->getAlbedo(),    m_defaultWhiteSRV);
	bindFallback(1, materialInstance->getNormal(),    m_defaultFlatNormalSRV);
	bindFallback(2, materialInstance->getMetallic(),  m_defaultWhiteSRV);
	bindFallback(3, materialInstance->getRoughness(), m_defaultWhiteSRV);
	bindFallback(4, materialInstance->getAO(),        m_defaultWhiteSRV);
}

/**
 * @brief Calcula el BaseColor final para `CBPerMaterial`: RGB de `params.baseColor`
 * multiplicado por `tint` (resaltado de selección, ver `RenderObject::tint`), alpha sin
 * tocar. Usado tanto por `renderGeometryObject` como por `renderForwardObject`.
 */
XMFLOAT4
DeferredRenderer::computeTintedBaseColor(const MaterialParams& params, const XMFLOAT3& tint) {
	return XMFLOAT4(
		params.baseColor.x * tint.x,
		params.baseColor.y * tint.y,
		params.baseColor.z * tint.z,
		params.baseColor.w);
}

/**
 * @brief Dibuja la escena, solo profundidad, desde el punto de vista de la luz principal —
 * llena `m_shadowDepthTexture`, que luego se lee como SRV (t6) en el lighting y el
 * transparent pass.
 *
 * @note [GameDev] `OMSetRenderTargets(0, nullptr, m_shadowDSV...)` enlaza CERO render
 * targets de color — solo el DSV. Es la forma correcta de hacer un "depth-only pass": el VS
 * del shadow shader (`ShadowMap.hlsl`) sí corre (necesita transformar posiciones al espacio
 * de la luz), pero no hay PS enlazado, así que la GPU no gasta tiempo calculando color, solo
 * profundidad. Es la optimización más simple y más importante de cualquier shadow pass.
 * @note [GameDev] El slot t6 se limpia a `nullptr` al INICIO de este método —necesario
 * porque el shadow map todavía está enlazado como SRV del frame anterior (el lighting pass
 * lo dejó ahí); sin este clear, D3D11 lanzaría un warning de "recurso enlazado como RTV/DSV
 * y SRV simultáneamente" al intentar escribirlo como DSV unas líneas después.
 */
void
DeferredRenderer::renderShadowPass(DeviceContext& deviceContext) {
	if (!m_shadowDSV.m_depthStencilView || !m_shadowShader.m_VertexShader) {
		return;
	}

	ID3D11ShaderResourceView* nullShadowSRV[1] = { nullptr };
	deviceContext.PSSetShaderResources(6, 1, nullShadowSRV);
	deviceContext.OMSetRenderTargets(0, nullptr, m_shadowDSV.m_depthStencilView);
	deviceContext.ClearDepthStencilView(m_shadowDSV.m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT shadowViewport{};
	shadowViewport.TopLeftX = 0.0f;
	shadowViewport.TopLeftY = 0.0f;
	shadowViewport.Width = static_cast<float>(m_shadowMapSize);
	shadowViewport.Height = static_cast<float>(m_shadowMapSize);
	shadowViewport.MinDepth = 0.0f;
	shadowViewport.MaxDepth = 1.0f;
	deviceContext.RSSetViewports(1, &shadowViewport);

	m_shadowRasterizer.render(deviceContext);
	m_shadowDepthStencil.render(deviceContext, 0, false);
	m_perFrameBuffer.render(deviceContext, 0, 1, false);

	for (const RenderObject* object : m_opaqueQueue) {
		if (!object || !object->castShadow) {
			continue;
		}
		renderShadowObject(deviceContext, *object);
	}
}

/**
 * @brief Dibuja un objeto al shadow depth target, saltando submeshes con material
 * transparente (que no deberían proyectar sombra sólida).
 *
 * @note [GameDev] `PSSetShader(nullptr, ...)` se llama EXPLÍCITAMENTE después de
 * `m_shadowShader.render()` porque `ShaderProgram::render()` solo enlaza el VS del shadow
 * shader (no tiene PS propio, ver el campo `m_shadowShader` en el header) — pero el PS de
 * cualquier draw call anterior (ej. el `m_gBufferShader` del frame previo) podría seguir
 * enlazado desde antes si nadie lo desenlaza. Un PS "colado" en un depth-only pass no
 * rompería la profundidad escrita (el PS no afecta el DSV), pero sí desperdicia trabajo de
 * GPU ejecutando un pixel shader completo cuyo resultado se descarta.
 */
void
DeferredRenderer::renderShadowObject(DeviceContext& deviceContext, const RenderObject& object) {
	if (!object.mesh) {
		return;
	}

	m_shadowShader.render(deviceContext);
	deviceContext.m_deviceContext->PSSetShader(nullptr, nullptr, 0);
	deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<Submesh>& submeshes = object.mesh->getSubmeshes();
	for (Submesh& submesh : submeshes) {
		MaterialInstance* materialInstance = object.materialInstance;
		if (submesh.materialSlot < object.materialInstances.size() &&
			object.materialInstances[submesh.materialSlot]) {
			materialInstance = object.materialInstances[submesh.materialSlot];
		}

		Material* material = materialInstance ? materialInstance->getMaterial() : nullptr;
		if (material && material->getDomain() == MaterialDomain::Transparent) {
			continue;
		}

		XMMATRIX submeshWorld = XMLoadFloat4x4(&submesh.localTransform) * object.world;
		XMStoreFloat4x4(&m_cbPerObject.World, XMMatrixTranspose(submeshWorld));
		m_perObjectBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbPerObject, 0, 0);
		m_perObjectBuffer.render(deviceContext, 1, 1, false);

		submesh.vertexBuffer.render(deviceContext, 0, 1);
		submesh.indexBuffer.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
		deviceContext.DrawIndexed(submesh.indexCount, submesh.startIndex, 0);
	}
}

/**
 * @brief Crea la textura de profundidad del shadow map, su DSV (para escribir) y su SRV
 * alias (para leer), más el shader y rasterizer del shadow pass.
 *
 * @note [GameDev] `DXGI_FORMAT_R24G8_TYPELESS` es el truco clásico para "depth-as-texture"
 * en D3D11: una textura `TYPELESS` no tiene un formato fijo — se le asigna uno distinto
 * según la VISTA que se cree sobre ella. Aquí se crea un DSV con `D24_UNORM_S8_UINT` (para
 * que la GPU la trate como depth/stencil normal al escribir durante el shadow pass) y un
 * SRV con `R24_UNORM_X8_TYPELESS` (para leer solo los 24 bits de profundidad como textura
 * normal en el pixel shader del lighting pass). Sin `TYPELESS`, D3D11 no permite crear dos
 * vistas con formatos distintos sobre el mismo recurso — no se podría reusar la misma
 * textura para ambos propósitos y habría que copiarla a una textura aparte cada frame.
 * @note [GameDev] El shadow rasterizer usa `D3D11_CULL_BACK` y NO `CULL_FRONT` (a pesar de
 * que muchos tutoriales de shadow mapping recomiendan cullear las caras frontales para
 * reducir "shadow acne" con auto-sombreado) — el bias fijo de `0.003` en
 * `ComputeShadow()` (`DeferredLighting.hlsl`) ya compensa ese acne, así que aquí se prioriza
 * evitar "peter-panning" (sombra despegada del objeto) que CULL_FRONT agresivo puede causar
 * en mallas delgadas.
 */
HRESULT
DeferredRenderer::createShadowResources(Device& device) {
	HRESULT hr = m_shadowDepthTexture.init(
		device,
		m_shadowMapSize,
		m_shadowMapSize,
		DXGI_FORMAT_R24G8_TYPELESS,
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_shadowDepthSRV.init(device, m_shadowDepthTexture, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_shadowDSV.init(device, m_shadowDepthTexture, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_DSV_DIMENSION_TEXTURE2D);
	if (FAILED(hr)) {
		return hr;
	}

	LayoutBuilder builder;
	builder.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);

	hr = m_shadowShader.init(device, "ShadowMap.hlsl", builder);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "createShadowResources", "ShadowMap.hlsl compiled OK");
	else LOG_ERROR("DeferredRenderer", "createShadowResources", "ShadowMap.hlsl compile FAIL hr=" + std::to_string(hr));
	if (FAILED(hr)) return hr;

	return m_shadowRasterizer.init(device, D3D11_FILL_SOLID, D3D11_CULL_BACK, false, true);
}

/**
 * @brief Destruye (si existían) y vuelve a crear los 4 targets del G-Buffer al tamaño dado.
 * @param device Dispositivo D3D11.
 * @param width  Ancho en píxeles.
 * @param height Alto en píxeles.
 *
 * @note [GameDev] Los 3 formatos usados aquí no son arbitrarios — cada uno se eligió por lo
 * que necesita guardar: RT0 (albedo+metallic) usa `R8G8B8A8_UNORM` (8 bits/canal) porque
 * color y metallic son valores [0,1] donde 256 niveles ya son suficientes para el ojo
 * humano. RT1 (normal+roughness) y RT3 (emissive+alpha) usan `R16G16B16A16_FLOAT` porque
 * las normales necesitan más precisión que 8 bits para evitar banding visible en
 * superficies curvas, y el emissive puede superar el rango [0,1] (HDR) para simular brillo
 * intenso. RT2 (world position+AO) usa el formato más caro, `R32G32B32A32_FLOAT` (16
 * bytes/píxel — el doble que los otros dos MRTs juntos), porque guardar la posición en
 * espacio de mundo directamente (en vez de reconstruirla desde la profundidad, la técnica
 * más común en motores AAA) necesita precisión float completa para no acumular error en
 * escenas grandes. Es la decisión de diseño más cara en bandwidth de este G-Buffer — ver
 * la nota de bandwidth en el header del archivo.
 */
HRESULT
DeferredRenderer::createGBufferResources(Device& device, unsigned int width, unsigned int height) {
	m_gBufferEmissiveAlphaRTV.destroy();
	m_gBufferEmissiveAlphaSRV.destroy();
	m_gBufferEmissiveAlphaTexture.destroy();
	m_gBufferWorldAoRTV.destroy();
	m_gBufferWorldAoSRV.destroy();
	m_gBufferWorldAoTexture.destroy();
	m_gBufferNormalRoughnessRTV.destroy();
	m_gBufferNormalRoughnessSRV.destroy();
	m_gBufferNormalRoughnessTexture.destroy();
	m_gBufferAlbedoMetallicRTV.destroy();
	m_gBufferAlbedoMetallicSRV.destroy();
	m_gBufferAlbedoMetallicTexture.destroy();

	HRESULT hr = createGBufferTarget(device,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_gBufferAlbedoMetallicTexture,
		m_gBufferAlbedoMetallicSRV,
		m_gBufferAlbedoMetallicRTV);
	if (FAILED(hr)) {
		return hr;
	}

	hr = createGBufferTarget(device,
		width,
		height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_gBufferNormalRoughnessTexture,
		m_gBufferNormalRoughnessSRV,
		m_gBufferNormalRoughnessRTV);
	if (FAILED(hr)) {
		return hr;
	}

	hr = createGBufferTarget(device,
		width,
		height,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		m_gBufferWorldAoTexture,
		m_gBufferWorldAoSRV,
		m_gBufferWorldAoRTV);
	if (FAILED(hr)) {
		return hr;
	}

	return createGBufferTarget(device,
		width,
		height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_gBufferEmissiveAlphaTexture,
		m_gBufferEmissiveAlphaSRV,
		m_gBufferEmissiveAlphaRTV);
}

/**
 * @brief Crea una textura con `RENDER_TARGET | SHADER_RESOURCE`, su RTV (para escribir en
 * el geometry pass) y su SRV (para leer en el lighting pass) — el trío que forma un slot
 * individual del G-Buffer.
 * @param format Formato DXGI del target (ver `createGBufferResources` para la elección de
 *               cada uno).
 */
HRESULT
DeferredRenderer::createGBufferTarget(Device& device,
	unsigned int width,
	unsigned int height,
	DXGI_FORMAT format,
	Texture& texture,
	Texture& srv,
	RenderTargetView& rtv) {
	HRESULT hr = texture.init(device,
		width,
		height,
		format,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		1,
		0);
	if (FAILED(hr)) {
		return hr;
	}

	hr = rtv.init(device, texture, D3D11_RTV_DIMENSION_TEXTURE2D, format);
	if (FAILED(hr)) {
		return hr;
	}

	return srv.init(device, texture, format);
}

/**
 * @brief Compila los shaders del geometry pass (`DeferredGBuffer.hlsl`) y del lighting pass
 * (`DeferredLighting.hlsl`), crea el sampler y rasterizer del fullscreen quad, y genera las
 * texturas 1×1 de fallback (blanco neutro y normal plana) usadas cuando un `MaterialInstance`
 * no trae alguna textura.
 *
 * @note [GameDev] Ambos shaders declaran el mismo input layout (`POSITION/NORMAL/TANGENT/
 * BITANGENT/TEXCOORD`) aunque `DeferredLighting.hlsl` solo use `POSITION`/`TEXCOORD` del
 * fullscreen quad — es así porque el motor reusa el mismo vertex buffer (`createFullScreenQuad`,
 * con el layout completo de `SimpleVertex`) para todo, así que el VS del lighting pass debe
 * aceptar ese layout aunque descarte la mayoría de sus campos.
 */
HRESULT
DeferredRenderer::createLightingResources(Device& device) {
	LayoutBuilder geometryBuilder;
	geometryBuilder.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);

	HRESULT hr = m_gBufferShader.init(device, "DeferredGBuffer.hlsl", geometryBuilder);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "createLightingResources", "DeferredGBuffer.hlsl compiled OK");
	else LOG_ERROR("DeferredRenderer", "createLightingResources", "DeferredGBuffer.hlsl compile FAIL hr=" + std::to_string(hr));
	if (FAILED(hr)) return hr;

	LayoutBuilder fullscreenBuilder;
	fullscreenBuilder.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);

	hr = m_deferredLightingShader.init(device, "DeferredLighting.hlsl", fullscreenBuilder);
	if (SUCCEEDED(hr)) LOG_MESSAGE("DeferredRenderer", "createLightingResources", "DeferredLighting.hlsl compiled OK");
	else LOG_ERROR("DeferredRenderer", "createLightingResources", "DeferredLighting.hlsl compile FAIL hr=" + std::to_string(hr));
	if (FAILED(hr)) return hr;

	hr = m_lightingSampler.init(device);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_fullscreenRasterizer.init(device, D3D11_FILL_SOLID, D3D11_CULL_NONE, false, false);
	if (FAILED(hr)) {
		return hr;
	}

	// Textura 1×1 blanca como albedo por defecto cuando el MaterialInstance no tiene textura.
	// Sin esto, el slot t0 queda sin enlazar → HLSL muestrea (0,0,0,0) → el G-buffer tiene
	// albedo negro → la detección de fondo "dot(rt0,rt0)<0.001" se dispara para esos píxeles.
	{
		UINT32 white = 0xFFFFFFFF;
		D3D11_TEXTURE2D_DESC td = {};
		td.Width = td.Height = 1;
		td.MipLevels = td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		D3D11_SUBRESOURCE_DATA sd = { &white, 4, 0 };
		ID3D11Texture2D* pTex = nullptr;
		if (SUCCEEDED(device.m_device->CreateTexture2D(&td, &sd, &pTex))) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			device.m_device->CreateShaderResourceView(pTex, &srvDesc, &m_defaultWhiteSRV);
			pTex->Release();
		}
	}

	// Textura 1×1 "normal plana" (128,128,255,255) como normal map por defecto cuando el
	// MaterialInstance no tiene textura de normal. Decodifica a (0,0,1) en tangent space
	// (N*0.5+0.5 inverso), es decir "sin relieve" — igual que no tener normal mapping.
	{
		UINT32 flatNormal = 0xFFFF8080; // ABGR little-endian: A=FF, B=FF, G=80, R=80
		D3D11_TEXTURE2D_DESC td = {};
		td.Width = td.Height = 1;
		td.MipLevels = td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		D3D11_SUBRESOURCE_DATA sd = { &flatNormal, 4, 0 };
		ID3D11Texture2D* pTex = nullptr;
		if (SUCCEEDED(device.m_device->CreateTexture2D(&td, &sd, &pTex))) {
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			device.m_device->CreateShaderResourceView(pTex, &srvDesc, &m_defaultFlatNormalSRV);
			pTex->Release();
		}
	}

	return S_OK;
}

/**
 * @brief Crea el vertex/index buffer de un quad en NDC ya listo para el lighting pass —
 * dos triángulos que cubren toda la pantalla, sin necesidad de transformación por matrices.
 *
 * @note [GameDev] Las posiciones ya están en el rango `[-1,1]` de NDC (no en espacio de
 * mundo), y el VS de `DeferredLighting.hlsl` las pasa directo a `SV_POSITION` sin
 * multiplicar por View/Projection (ver `VS()` en ese shader) — es la técnica estándar de
 * "fullscreen pass" en cualquier renderer: en vez de dibujar un quad 3D y proyectarlo, se
 * define el quad DIRECTAMENTE en el espacio de salida del rasterizador, evitando el costo
 * (mínimo, pero innecesario) de una multiplicación de matriz por vértice para algo que va a
 * cubrir toda la pantalla de todos modos. Las UVs con Y invertida (`(0,1)` arriba-izquierda)
 * compensan que las texturas de D3D11 tienen el origen en la esquina superior izquierda.
 */
HRESULT
DeferredRenderer::createFullScreenQuad(Device& device) {
	MeshComponent mesh;
	mesh.m_vertex = {
		{ EU::Vector3(-1.0f, -1.0f, 0.0f), EU::Vector3(), EU::Vector3(), EU::Vector3(), EU::Vector2(0.0f, 1.0f) },
		{ EU::Vector3(-1.0f,  1.0f, 0.0f), EU::Vector3(), EU::Vector3(), EU::Vector3(), EU::Vector2(0.0f, 0.0f) },
		{ EU::Vector3( 1.0f,  1.0f, 0.0f), EU::Vector3(), EU::Vector3(), EU::Vector3(), EU::Vector2(1.0f, 0.0f) },
		{ EU::Vector3( 1.0f, -1.0f, 0.0f), EU::Vector3(), EU::Vector3(), EU::Vector3(), EU::Vector2(1.0f, 1.0f) }
	};
	mesh.m_index = { 0, 1, 2, 0, 2, 3 };
	mesh.m_numVertex = static_cast<int>(mesh.m_vertex.size());
	mesh.m_numIndex = static_cast<int>(mesh.m_index.size());

	HRESULT hr = m_fullscreenVertexBuffer.init(device, mesh, D3D11_BIND_VERTEX_BUFFER);
	if (FAILED(hr)) {
		return hr;
	}

	return m_fullscreenIndexBuffer.init(device, mesh, D3D11_BIND_INDEX_BUFFER);
}

/**
 * @brief Crea los 4 `ID3D11BlendState` que usa el pipeline: opaque (sin mezcla), alpha
 * (transparencia estándar), additive (partículas/fuego/luces) y premultiplied alpha.
 *
 * @note [GameDev] Los 4 estados reusan el mismo `D3D11_BLEND_DESC` mutándolo entre cada
 * `CreateBlendState` (en vez de 4 structs independientes) — es solo un detalle de escritura
 * del código, D3D11 copia la desc al crear el state, así que mutar y reusar la variable no
 * afecta a los blend states ya creados. La diferencia real entre los 4 modos está en
 * `SrcBlend`/`DestBlend`: Alpha hace `src*srcAlpha + dst*(1-srcAlpha)` (mezcla estándar);
 * Additive hace `src*srcAlpha + dst*1` (suma el color sin restar nada del fondo — por eso
 * sirve para brillos que se acumulan, pero satura a blanco con suficientes capas);
 * Premultiplied asume que el color YA viene multiplicado por su propio alpha (`src*1 +
 * dst*(1-srcAlpha)`), lo cual evita el "halo oscuro" típico en bordes de sprites con alpha
 * blending directo cuando hay mipmapping o compositing en cadena.
 */
HRESULT
DeferredRenderer::createBlendStates(Device& device) {
	if (!device.m_device) {
		return E_POINTER;
	}

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	D3D11_RENDER_TARGET_BLEND_DESC& renderTarget = blendDesc.RenderTarget[0];
	renderTarget.BlendEnable = FALSE;
	renderTarget.SrcBlend = D3D11_BLEND_ONE;
	renderTarget.DestBlend = D3D11_BLEND_ZERO;
	renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
	renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
	renderTarget.DestBlendAlpha = D3D11_BLEND_ZERO;
	renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	renderTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT hr = device.m_device->CreateBlendState(&blendDesc, &m_opaqueBlendState);
	if (FAILED(hr)) {
		return hr;
	}

	renderTarget.BlendEnable = TRUE;
	renderTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	renderTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	renderTarget.BlendOp = D3D11_BLEND_OP_ADD;
	renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
	renderTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	renderTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;

	hr = device.m_device->CreateBlendState(&blendDesc, &m_alphaBlendState);
	if (FAILED(hr)) {
		return hr;
	}

	renderTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	renderTarget.DestBlend = D3D11_BLEND_ONE;
	renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
	renderTarget.DestBlendAlpha = D3D11_BLEND_ONE;

	hr = device.m_device->CreateBlendState(&blendDesc, &m_additiveBlendState);
	if (FAILED(hr)) {
		return hr;
	}

	renderTarget.SrcBlend = D3D11_BLEND_ONE;
	renderTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	renderTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
	renderTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;

	return device.m_device->CreateBlendState(&blendDesc, &m_premultipliedBlendState);
}

/**
 * @brief Elige, para un material dado, cuál de los 4 blend states creados en
 * `createBlendStates` corresponde a su `BlendMode` — con fallback a alpha estándar si el
 * estado específico no se creó correctamente.
 * @param material Material del submesh a dibujar; `nullptr` o dominio no-transparente
 *                 siempre resuelve a `m_opaqueBlendState`.
 */
ID3D11BlendState*
DeferredRenderer::resolveBlendState(const Material* material) const {
	if (!material) {
		return m_opaqueBlendState;
	}

	if (material->getDomain() != MaterialDomain::Transparent) {
		return m_opaqueBlendState;
	}

	switch (material->getBlendMode()) {
	case BlendMode::Additive:
		return m_additiveBlendState ? m_additiveBlendState : m_alphaBlendState;
	case BlendMode::PremultipliedAlpha:
		return m_premultipliedBlendState ? m_premultipliedBlendState : m_alphaBlendState;
	case BlendMode::Alpha:
	case BlendMode::Opaque:
	default:
		return m_alphaBlendState ? m_alphaBlendState : m_opaqueBlendState;
	}
}
