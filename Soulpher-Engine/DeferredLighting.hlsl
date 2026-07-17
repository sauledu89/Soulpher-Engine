// =============================================================================
//  DeferredLighting.hlsl  |  Lighting pass — fullscreen quad reads G-Buffers
//  Reads RT0–RT3 + shadow map, computes Lambert diffuse + Blinn-Phong specular.
//
//  Debug view modes (DebugViewMode):
//    0 = final lit result      4 = Roughness
//    1 = Albedo                5 = AO
//    2 = Normals (decoded)     6 = Emissive
//    3 = World Position        7 = Shadow factor
// =============================================================================

// ─── Constant Buffers ────────────────────────────────────────────────────────

cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection;
    float3 CameraPos;
    float  _pad0;
    float3 LightDir;       // Direction OF the light (toward the scene; negate for Lambert)
    float  _pad1;
    float3 LightColor;
    float  LightRange;
    float3 LightPosition;
    int    LightType;
    float4 LightPositionsRanges[8];
    float4 LightColorsTypes[8];
    float4 LightDirectionsIntensities[8];
    int    LightCount;
    float3 _pad2;
    float4 LightSpotAngles[8]; // .x = semiangulo del cono (radianes) de cada luz Spot
}

cbuffer DebugData : register(b1)
{
    int   DebugViewMode;   // Which channel to visualize (0 = final)
    float ShadowStrength;  // Shadow blend factor (1 = full shadow)
    float _dpad0;
    float _dpad1;
}

// ─── G-Buffer textures (slots t0–t3) + shadow map (t6) ───────────────────────
Texture2D GBuffer0  : register(t0); // Albedo (RGB) + Metallic (A)
Texture2D GBuffer1  : register(t1); // Encoded Normal (RGB) + Roughness (A)
Texture2D GBuffer2  : register(t2); // World Position (RGB) + AO (A)
Texture2D GBuffer3  : register(t3); // Emissive (RGB) + Alpha (A)
Texture2D ShadowMap : register(t6); // Shadow depth map (R24_UNORM)

SamplerState Sampler : register(s0);

// ─── Vertex I/O (fullscreen quad uses full SimpleVertex layout) ───────────────
struct VS_INPUT
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float3 bitangent : BITANGENT;
    float2 tex       : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

// ─── Vertex Shader — pass-through (positions already in NDC) ─────────────────
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.tex = input.tex;
    return output;
}

// ─── PCF 3×3 shadow sampling ─────────────────────────────────────────────────
// [GameDev] PCF (Percentage-Closer Filtering) es la técnica estándar para suavizar los
// bordes duros y aliased de un shadow map básico. En vez de un solo sample "está en sombra
// sí/no", se toman 9 samples en una grilla 3×3 alrededor del texel y se promedia cuántos
// pasan la comparación de profundidad — el resultado es un valor continuo en [0,1] (borde
// de sombra suave) en vez de binario (borde de sombra dentado, escalonado). Es barato
// (9 samples) comparado con soluciones más avanzadas (PCSS, VSM) y suficiente para la
// mayoría de escenas de tamaño moderado.
float ComputeShadow(float3 worldPos)
{
    // Proyecta el píxel al espacio homogéneo de la luz, igual que hace la GPU para SV_POSITION
    // en el vertex shader normal — solo que aquí se hace manualmente en el pixel shader
    // porque LightViewProjection es una cámara DISTINTA a la de la escena.
    float4 shadowPosH = mul(float4(worldPos, 1.0f), LightViewProjection);
    float3 proj = shadowPosH.xyz / shadowPosH.w; // división perspectiva → NDC [-1,1]

    // NDC [-1,1] → UV [0,1] del shadow map. La Y se invierte porque NDC crece hacia arriba
    // y las coordenadas de textura de D3D11 crecen hacia abajo.
    float2 uv;
    uv.x =  proj.x * 0.5f + 0.5f;
    uv.y = -proj.y * 0.5f + 0.5f;

    // Outside shadow frustum → fully lit
    // [GameDev] Esto es consecuencia directa de usar un frustum ortográfico FIJO (ver la
    // nota en DeferredRenderer::updateLightMatrices): cualquier píxel fuera del área
    // cubierta por el shadow map simplemente no puede saber si está en sombra o no, así
    // que el fallback seguro es "iluminado" en vez de "en sombra" — lo contrario dejaría
    // toda la escena fuera de rango con un tinte oscuro falso.
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || proj.z > 1.0f)
        return 1.0f;

    // [GameDev] El bias resta un pequeño margen a la profundidad del píxel ANTES de
    // comparar contra el shadow map — sin él, la resolución finita del shadow map (2048²
    // texels para cubrir 40×40 unidades de mundo) hace que una superficie se auto-sombree
    // a sí misma por errores de redondeo ("shadow acne", un patrón de rayas tipo moiré).
    // Un bias demasiado grande, en cambio, causa "peter-panning": la sombra se despega
    // visiblemente de la base del objeto que la proyecta. 0.003 es un valor ajustado a ojo
    // para esta escena — si cambias el rango del frustum ortográfico (40×40, near/far
    // 1–80 en updateLightMatrices), probablemente necesites reajustar este bias también.
    float depth      = proj.z - 0.003f; // shadow bias
    float texelSize  = 1.0f / 2048.0f;  // debe coincidir con DeferredRenderer::m_shadowMapSize
    float shadow     = 0.0f;

    [unroll] for (int x = -1; x <= 1; ++x)
    [unroll] for (int y = -1; y <= 1; ++y)
    {
        float smDepth = ShadowMap.Sample(Sampler, uv + float2(x, y) * texelSize).r;
        shadow += (depth <= smDepth) ? 1.0f : 0.0f;
    }

    return shadow / 9.0f;
}

// ─── Pixel Shader ────────────────────────────────────────────────────────────
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float2 uv = input.tex;

    // Read G-Buffers
    float4 rt0 = GBuffer0.Sample(Sampler, uv);
    float4 rt1 = GBuffer1.Sample(Sampler, uv);
    float4 rt2 = GBuffer2.Sample(Sampler, uv);
    float4 rt3 = GBuffer3.Sample(Sampler, uv);

    // Unpack G-Buffer data — el orden y significado de cada canal debe coincidir EXACTO
    // con lo que escribe DeferredGBuffer.hlsl::PS(); ver la nota de "contrato estricto" ahí.
    float3 albedo    = rt0.rgb;
    float  metallic  = rt0.a;
    float3 N         = normalize(rt1.rgb * 2.0f - 1.0f); // decode [0,1] → [-1,1]
    float  roughness = rt1.a;
    float3 worldPos  = rt2.rgb;
    float  ao        = rt2.a;
    float3 emissive  = rt3.rgb;

    // ─── Debug views ─────────────────────────────────────────────────────────
    if (DebugViewMode == 1) return float4(albedo, 1.0f);
    if (DebugViewMode == 2) return float4(N * 0.5f + 0.5f, 1.0f);
    if (DebugViewMode == 3) return float4(frac(abs(worldPos) * 0.05f), 1.0f);
    if (DebugViewMode == 4) return float4(roughness, roughness, roughness, 1.0f);
    if (DebugViewMode == 5) return float4(ao, ao, ao, 1.0f);
    if (DebugViewMode == 6) return float4(emissive, 1.0f);
    if (DebugViewMode == 7) return float4(ComputeShadow(worldPos).xxx, 1.0f);

    // ─── Skip lighting for background pixels (worldPos ≈ 0 and normal invalid) ─
    // G-Buffer clear sets RT2 to (0,0,0,1). If all 4 comps are near default,
    // this is a background pixel — return a sky color.
    // [GameDev] Esta heurística asume que NINGÚN objeto real de la escena tiene albedo
    // (0,0,0) exacto (negro puro) — si algún día agregas un material completamente negro,
    // sus píxeles se confundirían con fondo y quedarían pintados del color de cielo en vez
    // de negro real. La alternativa más robusta es un canal de "stencil"/máscara dedicado
    // que marque explícitamente "aquí SÍ se dibujó algo", pero eso cuesta un RT o un canal
    // extra — este truco evita ese costo asumiendo que el caso límite (objeto negro puro)
    // no ocurre en esta escena. También es la razón de que renderSkyboxPass() (en
    // DeferredRenderer.cpp) se llame DESPUÉS de este lighting pass: si hay un Skybox
    // cargado, pinta por encima de este color de fondo plano con el cubemap real.
    if (dot(rt0, rt0) < 0.001f)
        return float4(0.0f, 0.125f, 0.30f, 1.0f); // engine background color

    // ─── Multi-light accumulation (Directional / Point / Spot) ────────────────
    // Cada luz activa (hasta LightCount, empacadas por DeferredRenderer::writeLightToFrameBuffer)
    // aporta su propia direccion L y atenuacion: Directional no atenua; Point atenua por
    // distancia hasta 'range'; Spot ademas recorta por el cono definido en LightSpotAngles.
    // [GameDev] Esto es Blinn-Phong, no PBR "de verdad" (Cook-Torrance/GGX): es un modelo
    // más viejo y barato que aproxima el brillo especular con `pow(NdotH, shininess)` en vez
    // de una BRDF basada en microfacetas. `shininess` se deriva de `roughness` de forma
    // heurística (roughness alto → shininess bajo → highlight ancho y tenue; roughness bajo
    // → highlight angosto y brillante), pero no sigue ninguna ecuación física — es una
    // aproximación "que se ve bien" y barata de calcular, apropiada para un motor educativo
    // o de tamaño moderado. Migrar a PBR real requeriría reemplazar este bloque por una BRDF
    // con Fresnel (Schlick), distribución normal (GGX/Trowbridge-Reitz) y geometría
    // (Smith), y tratar `metallic` como el factor que interpola entre dieléctrico (specular
    // ~4%) y conductor (specular = albedo) — hoy `metallic` no se usa directamente en la
    // ecuación de luz, solo se lee del G-Buffer y se pasa a `totalSpecular *= ...` como un
    // atajo simplificado.
    float3 V = normalize(CameraPos - worldPos);
    float  shininess = max(1.0f, (1.0f - roughness) * 256.0f);

    float3 totalDiffuse  = float3(0.0f, 0.0f, 0.0f);
    float3 totalSpecular = float3(0.0f, 0.0f, 0.0f);

    // [GameDev] El límite de 8 no es arbitrario: coincide con `kMaxSceneLights` en
    // RenderTypes.h (C++), que determina el tamaño de los 4 arreglos `float4[8]` de
    // CBPerFrame. `min(LightCount, 8)` es una red de seguridad — LightCount YA debería venir
    // acotado a 8 desde `updatePerFrame`, pero si algún día ese acotamiento se rompe (o el
    // buffer trae basura de un frame anterior sin inicializar), este `min()` evita leer más
    // allá del arreglo (out-of-bounds en HLSL generalmente no crashea, pero produce lecturas
    // de memoria de otro cbuffer — un bug silencioso y difícil de diagnosticar).
    int count = min(LightCount, 8);
    for (int i = 0; i < count; ++i)
    {
        float4 posRange     = LightPositionsRanges[i];
        float4 colorType    = LightColorsTypes[i];
        float4 dirIntensity = LightDirectionsIntensities[i];
        int    lightType    = (int)colorType.w;
        float3 lightColorI  = colorType.xyz; // ya incluye intensity (color * intensity)

        float3 L;
        float  attenuation = 1.0f;

        if (lightType == 0) // Directional
        {
            L = normalize(-dirIntensity.xyz);
        }
        else // Point (1) o Spot (2)
        {
            float3 toLight = posRange.xyz - worldPos;
            float  dist    = length(toLight);
            L = toLight / max(dist, 0.0001f);

            // [GameDev] `falloff*falloff` (atenuación "smooth" cuadrática por distancia
            // normalizada, no la fórmula física real 1/dist²) es una aproximación común en
            // motores de videojuegos: la atenuación físicamente correcta (inverse-square)
            // tiende a infinito cerca de la fuente y nunca llega exactamente a cero, lo cual
            // complica definir un "range" limpio para culling y para que el artista controle
            // el alcance visual de una luz. Esta versión garantiza atenuación = 0 exacto en
            // `dist >= range` (útil para saber hasta dónde una luz "importa") a cambio de no
            // ser físicamente exacta — el mismo trade-off que usan Unity y Unreal en sus
            // modos de atenuación "no realista"/"windows".
            float range   = max(posRange.w, 0.001f);
            float falloff = saturate(1.0f - (dist / range));
            attenuation   = falloff * falloff;

            if (lightType == 2) // Spot: recorte adicional por el cono
            {
                // [GameDev] `spotFactor` interpola suavemente entre 0 (fuera del cono) y 1
                // (eje central del cono) usando el coseno del ángulo entre la dirección hacia
                // el píxel y la dirección de la luz — comparar cosenos evita un `acos()` caro
                // en el pixel shader. `cosCutoff` es el coseno del semiángulo del cono
                // (`LightSpotAngles[i].x`, en radianes); dividir por `(1 - cosCutoff)` crea un
                // degradado suave desde el borde del cono hacia el centro, en vez de un corte
                // binario duro que se vería como un círculo con bordes dentados.
                float3 spotDir   = normalize(dirIntensity.xyz);
                float  cosAngle  = dot(-L, spotDir);
                float  cosCutoff = cos(LightSpotAngles[i].x);
                float  spotFactor = saturate((cosAngle - cosCutoff) / max(1.0f - cosCutoff, 0.001f));
                attenuation *= spotFactor;
            }
        }

        float NdotL = saturate(dot(N, L));
        totalDiffuse += NdotL * lightColorI * attenuation;

        float3 H     = normalize(L + V);
        float  NdotH = saturate(dot(N, H));
        totalSpecular += pow(NdotH, shininess) * lightColorI * attenuation;
    }

    // [GameDev] Atenuar el specular por `(1-metallic)` es al revés de cómo funcionaría PBR
    // real (ahí los metales SON el specular, los dieléctricos casi no tienen), pero en este
    // modelo Blinn-Phong simplificado `metallic` solo empuja el brillo hacia menos
    // specular Blinn-Phong genérico — un aproximado tosco, no físicamente correcto (ver la
    // nota completa sobre PBR vs. Blinn-Phong más arriba, antes del loop de luces).
    totalSpecular *= (1.0f - roughness) * (1.0f - metallic);

    float3 ambient = 0.08f * albedo * ao; // termino ambiental fijo, independiente del numero de luces
    float3 diffuse = totalDiffuse * albedo;

    // [GameDev] El shadow factor se aplica SOLO a diffuse+specular, nunca a ambient ni a
    // emissive — es la razón por la que las zonas en sombra no quedan negro puro: la luz
    // ambiental (simula rebotes indirectos de forma muy tosca, sin GI real) y cualquier
    // emisión propia del material siguen visibles incluso donde el shadow map dice "en
    // sombra". `ShadowStrength` (parámetro de debug, `DebugData.ShadowStrength`) permite
    // apagar sombras gradualmente sin tocar el shader — en 0.0, `lerp` devuelve 1.0 siempre
    // (escena completamente iluminada, útil para aislar problemas de iluminación de
    // problemas de sombra al depurar).
    float shadowFactor = lerp(1.0f, ComputeShadow(worldPos), ShadowStrength);

    float3 color = ambient
                 + shadowFactor * (diffuse + totalSpecular)
                 + emissive;

    return float4(color, 1.0f);
}
