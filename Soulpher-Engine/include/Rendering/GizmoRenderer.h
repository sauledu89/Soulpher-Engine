/**
 * @file GizmoRenderer.h
 * @brief Dibuja los gizmos de transformación (traslación/rotación/escala) del actor
 *        seleccionado en el viewport del editor.
 *
 * @details
 * No usa el ECS (`Actor`/`Transform`) ni el pipeline deferred: es geometría procedural
 * en espacio unitario ("plantilla"), dibujada con un shader unlit propio (`Gizmo.fx`)
 * directamente sobre el `EditorViewportPass`, con el depth test deshabilitado para
 * quedar siempre por encima de la escena (así el usuario nunca pierde el handle detrás
 * de un objeto). Un mismo par de mallas plantilla (apuntando a +X) se reutiliza para
 * los tres ejes rotando la matriz `World` en incrementos de 90° — evita triplicar la
 * geometría y sus buffers en GPU.
 *
 * @note [GameDev] Este es el mismo componente que en Unity se llama "Transform Tool
 * gizmo" y en Unreal Engine "Widget"/"FWidget": una herramienta de edición que vive
 * completamente fuera de la simulación del juego (no es un Actor, no tiene física, no
 * se guarda en la escena) pero comparte la cámara y el viewport con el resto del
 * render. Estudiar cómo se separa de la geometría "real" enseña un principio general
 * de arquitectura de motores: el contenido del juego y las herramientas de autoría
 * viven en capas de render distintas, con reglas de profundidad/oclusión distintas
 * (aquí, depth test OFF) aunque comparten la misma GPU y el mismo frame.
 *
 * @see Gizmo.fx, BaseApp::update() (picking/arrastre), LightGizmoRenderer (mismo shader,
 *      pensado para íconos de luces en vez de handles de edición)
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "ShaderProgram.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"

class Device;
class DeviceContext;

/**
 * @class GizmoRenderer
 * @brief Geometría + draw calls del gizmo de transformación (Translate/Rotate/Scale).
 *
 * @details
 * No calcula el picking ni el arrastre — solo dibuja. `BaseApp::update()` es quien
 * decide qué eje está bajo el cursor (`hoverAxis`) y cuál se está arrastrando
 * (`dragAxis`), y quién finalmente modifica el `Transform` del actor; esta clase solo
 * necesita esos dos valores para pintar el resaltado correcto.
 *
 * @note [GameDev] Separar "qué se dibuja" (esta clase) de "qué significa cada handle y
 * cómo reacciona al mouse" (la lógica de picking en `BaseApp`) es el mismo principio de
 * Model-View que usan los editores de Unreal/Unity/Blender: el gizmo es puramente
 * visual, y un sistema de picking independiente (normalmente basado en un color-ID
 * buffer o, como aquí, en intersección analítica rayo-primitiva) decide la interacción.
 * Mezclar ambas responsabilidades en una sola clase dificulta reemplazar después el
 * método de picking (por ejemplo, pasar de intersección analítica a un color-ID buffer
 * para hit-testing pixel-perfecto) sin tocar el código de dibujo.
 */
class GizmoRenderer {
public:
    /** @brief Modo de edición activo (qué handles se dibujan y pueden pickearse). */
    enum class Mode { None, Translate, Rotate, Scale };

    /**
     * @brief Eje sobre el que opera un handle. Los ejes son fijos al MUNDO, no al
     * espacio local del actor (una decisión de diseño explícita — ver nota abajo).
     * @details `All` es el handle central de escala uniforme (solo en `Mode::Scale`):
     * escala los 3 ejes a la vez en vez de solo uno.
     *
     * @note [GameDev] Unreal Engine y Unity ofrecen ambos modos (world-space y
     * local-space gizmo) intercambiables con una tecla. Aquí se implementó solo
     * world-space por simplicidad: los handles siempre apuntan a X/Y/Z globales sin
     * importar la rotación del actor, lo que hace el picking y el cálculo de "cuánto
     * se movió el mouse a lo largo del eje" mucho más simples (no hay que rotar el
     * rayo al espacio local del objeto). El costo es que mover un objeto rotado a lo
     * largo de "su propio eje X" requiere activar Local-space, que este motor no
     * soporta todavía — una extensión natural sería agregar un tercer estado
     * (World/Local) que elija entre `axisDirectionVec()` (mundo) y las columnas de la
     * matriz de rotación del actor (local).
     */
    enum class Axis { None, X, Y, Z, All };

    // Dimensiones de la malla "plantilla" (espacio unitario, sin escalar por pantalla).
    // Compartidas con el picking en BaseApp — deben coincidir con la geometría generada.
    static constexpr float kArmLength        = 1.0f;  ///< Longitud del brazo (flecha/cubo), desde el centro.
    static constexpr float kShaftEnd         = 0.65f; ///< Punto donde termina el cilindro delgado y empieza la punta.
    static constexpr float kRingRadius       = 0.9f;  ///< Radio del anillo de rotación.
    static constexpr float kPickRadius       = 0.10f; ///< Radio de tolerancia (cápsula) para picking de flecha/cubo.
    static constexpr float kRingPickTol      = 0.06f; ///< Tolerancia radial para picking del anillo de rotación.
    static constexpr float kCenterPickRadius = 0.20f; ///< Radio de tolerancia para el handle central (Axis::All).

    /**
     * @brief Compila el shader, crea los constant buffers y genera las 4 mallas plantilla
     * (flecha, cubo, anillo, cubo central) una sola vez.
     * @param device Dispositivo Direct3D 11.
     * @return `S_OK` si todos los recursos se crearon correctamente.
     * @note Debe llamarse una sola vez, después de tener un `Device` válido (normalmente
     * en `BaseApp::init()`), antes de la primera llamada a `render()`.
     */
    HRESULT init(Device& device);

    /** @brief Libera shader, buffers y las 4 mallas plantilla. Llamar antes de destruir el `Device`. */
    void destroy();

    /**
     * @brief Dibuja los handles del modo activo sobre el render target actualmente enlazado.
     * @param deviceContext Contexto Direct3D 11 (debe tener el RTV/DSV del viewport ya bindeado).
     * @param viewProj    Matriz View*Proj de la cámara activa (sin transponer; se transpone internamente).
     * @param center      Centro del gizmo en espacio mundo (posición del actor seleccionado).
     * @param screenScale Factor de escala ya calculado por `BaseApp` para mantener un tamaño
     *                    constante en pantalla sin importar el zoom (ver nota abajo).
     * @param mode        Modo activo (Translate/Rotate/Scale). `Mode::None` no dibuja nada.
     * @param hoverAxis   Eje bajo el cursor este frame (resaltado tenue). `Axis::None` si ninguno.
     * @param dragAxis    Eje siendo arrastrado activamente (resaltado fuerte). `Axis::None` si no hay arrastre.
     *
     * @note [GameDev] `screenScale` es la solución clásica al problema de "el gizmo se
     * vuelve gigante o invisible según el zoom": en vez de un tamaño fijo en unidades
     * de mundo, se escala linealmente con la distancia cámara-objeto
     * (`distancia * constante`), aproximando el hecho de que en una proyección en
     * perspectiva un objeto de tamaño angular constante crece proporcionalmente con la
     * distancia. Motores como Unreal recalculan esto cada frame para cada gizmo/ícono
     * de editor visible; aquí se hace en `BaseApp::update()` justo antes de llamar a
     * `render()`, así que siempre refleja la posición de cámara del frame actual.
     */
    void render(DeviceContext& deviceContext,
                const XMMATRIX& viewProj,
                const XMFLOAT3& center,
                float screenScale,
                Mode mode,
                Axis hoverAxis,
                Axis dragAxis);

    /**
     * @brief Dirección unitaria en espacio mundo de un eje lógico.
     * @param axis Eje a consultar. `Axis::None`/`Axis::All` devuelven el vector cero.
     * @return `(1,0,0)`, `(0,1,0)` o `(0,0,1)` según el eje — nunca depende del actor,
     *         ya que los ejes del gizmo están fijos al mundo (ver `Axis`).
     * @note Público porque `BaseApp` lo reutiliza para el picking (proyectar el rayo del
     * mouse sobre la misma línea/eje que se está dibujando aquí).
     */
    static XMVECTOR axisDirectionVec(Axis axis);

private:
    /** @brief Vértice mínimo del gizmo: solo posición (el shader es unlit, sin normales/UV). */
    struct GizmoVertex { XMFLOAT3 Pos; };

    /** @brief Buffers GPU de una malla plantilla (vertex + index buffer, en espacio unitario). */
    struct GizmoMesh {
        Buffer vb, ib;
        unsigned int indexCount = 0;
    };

    /**
     * @brief Matriz que reorienta la plantilla (siempre construida apuntando a +X local)
     * hacia el eje mundial solicitado.
     * @param axis Eje destino. `Axis::X` es la identidad (la plantilla ya apunta ahí);
     *             `Axis::Y`/`Axis::Z` rotan 90° sobre Z/Y respectivamente.
     * @return Matriz de rotación pura (sin escala ni traslación).
     *
     * @note [GameDev] Este es el truco central para no triplicar geometría: en vez de
     * generar 3 flechas/cubos/anillos (uno ya orientado por eje), se genera **una sola
     * plantilla** apuntando a +X y se dibuja 3 veces con una rotación distinta en la
     * matriz `World`. Es el mismo patrón de "instancing manual" que usan los motores
     * para iconos/gizmos de editor: geometría barata (unos pocos cientos de vértices),
     * reutilizada con transformaciones distintas — la GPU nunca ve una malla "Y" o "Z"
     * per se, solo la misma malla "X" con una matriz diferente cada draw call.
     */
    static XMMATRIX axisAlignRotation(Axis axis);

    /**
     * @brief Sube los vértices/índices de una malla plantilla a la GPU.
     * @param device  Dispositivo Direct3D 11.
     * @param mesh    Malla destino (se sobreescribe).
     * @param verts   Vértices en espacio unitario, ya generados por alguno de los `buildXxxGeometry`.
     * @param indices Índices de triángulos (`D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST`).
     * @return `S_OK` si ambos buffers se crearon correctamente.
     */
    HRESULT buildMesh(Device& device, GizmoMesh& mesh,
                       const std::vector<GizmoVertex>& verts,
                       const std::vector<unsigned int>& indices);

    /** @brief Genera la flecha de Translate: cilindro + cono, apuntando a +X. */
    void buildArrowGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices);
    /** @brief Genera el handle de Scale por eje: cilindro + cubo, apuntando a +X. */
    void buildCubeHandleGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices);
    /** @brief Genera el anillo de Rotate: toro delgado en el plano YZ (perpendicular a +X). */
    void buildRingGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices);
    /** @brief Genera el handle central de escala uniforme: cubo pequeño centrado en el origen. */
    void buildCenterHandleGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices);

    /**
     * @brief Sube `World`+`Color` y emite el draw call de una malla plantilla ya orientada/escalada.
     * @param deviceContext Contexto Direct3D 11.
     * @param mesh   Malla plantilla a dibujar (arrow/cube/ring/center).
     * @param center Centro del gizmo en espacio mundo.
     * @param scale  Escala uniforme (ver `screenScale` en `render()`).
     * @param axis   Eje al que corresponde este handle — determina la rotación (`axisAlignRotation`).
     * @param color  Color final (ya resuelto: base, resaltado por hover, o resaltado por arrastre).
     */
    void drawMesh(DeviceContext& deviceContext, GizmoMesh& mesh,
                  const XMFLOAT3& center, float scale,
                  Axis axis, const XMFLOAT4& color);

    ShaderProgram m_shader;
    Buffer m_cbFrame;  ///< b0: CBGizmoFrame (ViewProj).
    Buffer m_cbObject; ///< b1: CBGizmoObject (World, Color).

    DepthStencilState m_depthDisabled; ///< Depth test/write OFF — siempre encima de la escena.
    RasterizerState    m_rasterizer;   ///< FILL_SOLID, CULL_BACK.

    GizmoMesh m_arrowMesh;  ///< Traslación: cilindro + cono, apuntando a +X.
    GizmoMesh m_cubeMesh;   ///< Escala por eje: cilindro + cubo, apuntando a +X.
    GizmoMesh m_ringMesh;   ///< Rotación: toro delgado en el plano YZ (perpendicular a +X).
    GizmoMesh m_centerMesh; ///< Escala uniforme: cubo pequeño centrado en el origen (Axis::All).
};

/** @brief Constant buffer b0 de Gizmo.fx: la matriz ViewProj de la cámara (transpuesta antes de subir). */
struct CBGizmoFrame {
    XMFLOAT4X4 ViewProj{};
};

/** @brief Constant buffer b1 de Gizmo.fx: World del handle actual + su color final (ya resaltado). */
struct CBGizmoObject {
    XMFLOAT4X4 World{};
    XMFLOAT4   Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};
