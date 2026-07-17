/**
 * @file UserInterface.h
 * @brief Capa de interfaz gráfica ImGui del editor de Soulpher-Engine.
 *
 * @details
 * Esta clase es la **capa de interfaz gráfica** del motor, utilizando **Dear ImGui** para
 * mostrar menús, paneles, barras de herramientas y controles interactivos en tiempo real.
 *
 * ---
 * 🔹 **Conceptos clave para estudiantes de desarrollo de videojuegos**:
 * - **ImGui** es una librería de *Immediate Mode GUI* → cada frame se redibuja la interfaz.
 * - Se integra directamente en el *render loop* del motor.
 * - Es ideal para **herramientas de desarrollo**, depuración y edición en tiempo real.
 * - Permite personalizar completamente la apariencia (colores, fuentes, tamaños).
 *
 * ---
 * 💡 **Ejemplos de uso dentro de un motor**:
 * - Mostrar un **Inspector** para modificar propiedades de un actor.
 * - Implementar un **Outliner** para seleccionar objetos en la escena.
 * - Crear menús de herramientas para cambiar materiales, cargar modelos o ajustar luces.
 * - Visualizar el **framebuffer** en una ventana de preview.
 *
 * @note [GameDev] ImGui usa el patron Immediate Mode: la UI no tiene estado persistente,
 * se reconstruye completamente cada frame desde cero. Unreal Engine usa Slate (Retained
 * Mode) para su editor — los widgets son objetos C++ persistentes con estado propio.
 * UMG (para UI en juego) se compila a Slate internamente. ImGui es preferido para
 * herramientas internas por su overhead minimo y su facilidad de iteracion rapida.
 *
 * @note [GameDev] `UserInterface` NUNCA toca `Device`, `m_actors` ni ningún otro estado
 * de `BaseApp` directamente — no tiene acceso a ellos. Cuando un botón de la UI necesita
 * causar un efecto que solo `BaseApp` puede ejecutar (crear un actor nuevo, que requiere
 * el `Device`; duplicar o borrar uno, que requiere el vector de la escena), esta clase
 * simplemente marca la intención en un miembro público (`selectedActorIndex`,
 * `requestedLightType`, `requestedDuplicateIndex`, `requestedDeleteIndex`) y
 * `BaseApp::update()` lo revisa al inicio del siguiente frame. Este es el mismo patrón
 * de "comando diferido" (deferred command / intent flag) que usan los editores de
 * Unreal y Unity para desacoplar la capa de UI de la lógica de la escena: la UI nunca
 * modifica datos del juego "en caliente" mientras se está dibujando, evita mutar una
 * lista mientras se itera sobre ella (borrar un actor mientras `outliner()` recorre
 * `m_actors` sería exactamente ese bug), y mantiene a `UserInterface` reutilizable sin
 * conocer los detalles internos de cómo se construye un `Actor`.
 */

#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

class Window;
class SwapChain;
class Texture;
class Actor;
class ModelComponent;

/**
 * @class UserInterface
 * @brief Gestiona y renderiza la interfaz gráfica (ImGui) del motor.
 *
 * @note El estilo visual puede personalizarse para adaptarse al flujo de trabajo del usuario.
 */
class UserInterface {
public:
    /** @brief Constructor: prepara valores iniciales pero no crea la UI. */
    UserInterface();

    /** @brief Destructor: se asegura de liberar la UI si no se ha hecho. */
    ~UserInterface();

    /** @brief Aplica un esquema de colores tipo "Neon Red". */
    void NeonRedStyle();

    /** @brief Aplica el tema cyberpunk de Soulpher-Engine: neón cian/magenta sobre negro profundo. */
    void CyberpunkStyle();

    /**
     * @brief Tema NES: gris claro tipo plástico de consola, acentos rojo Nintendo y
     * naranja Zapper, texto oscuro para contraste.
     * @details Sobreescribe `ImGuiStyle::Colors` completo (no solo unos pocos campos) y
     * las métricas de bordes/redondeo — igual que `CyberpunkStyle()`/`greyStyle()`, para
     * que ningún color por defecto de ImGui (pensado para temas oscuros) se filtre y
     * quede ilegible sobre el fondo claro de este tema.
     * @note Debe llamarse después de `ImGui::StyleColorsLight()` (no `StyleColorsDark()`):
     * ese es el punto de partida correcto para un tema de fondo claro — algunos campos
     * que esta función no toca explícitamente (por ejemplo, colores de plots poco usados)
     * heredan un valor razonable de la base clara en vez de uno pensado para fondo oscuro.
     */
    void NESStyle();

    /**
     * @brief Inicializa ImGui y lo vincula a la ventana y dispositivo de render.
     * @param window Puntero a la ventana Win32.
     * @param device Dispositivo Direct3D.
     * @param deviceContext Contexto Direct3D.
     *
     * @note Este método debe llamarse antes de cualquier render de ImGui.
     */
    void init(void* window, ID3D11Device* device, ID3D11DeviceContext* deviceContext);

    /** @brief Actualiza la lógica de la interfaz (input, estados, ventanas). */
    void update();

    /** @brief Dibuja la interfaz gráfica en pantalla. */
    void render();

    /** @brief Libera todos los recursos asociados a ImGui. */
    void destroy();

    /**
     * @brief Control de tipo slider para modificar un vector3 (usado para Position/
     * Rotation/Scale en el Inspector), con botones de eje coloreados (X=rojo, Y=verde,
     * Z=azul) y un checkbox de "vínculo" para editar los 3 ejes a la vez.
     * @param label Texto identificador — también usado como ID de ImGui, así que debe
     *              ser único por llamada dentro de la misma ventana (ej. "Position",
     *              "Rotation", "Scale"); el estado del vínculo se guarda scopeado a este
     *              mismo ID, así cada campo tiene su propio toggle independiente.
     * @param values Puntero a tres floats contiguos (x, y, z) — se editan in-place.
     * @param resetValues Valor al que vuelve un eje si se hace clic en su botón (X/Y/Z).
     * @param columnWidth Ancho de la columna de etiqueta.
     *
     * @note Muy útil en motores para mover objetos en el espacio.
     * @note Con el vínculo activo, editar cualquier eje (arrastrando su valor o con su
     * botón de reset) copia ese mismo valor a los otros dos — útil para escalar
     * uniformemente sin tener que escribir el mismo número tres veces.
     */
    void vec3Control(const std::string& label,
        float* values,
        float resetValues = 0.0f,
        float columnWidth = 100.0f);

    /**
     * @brief Muestra la barra de menú principal.
     * @param window Ventana de aplicación.
     * @param swapChain Cadena de intercambio.
     * @param backBuffer Textura del back buffer.
     */
    void menuBar(Window window, SwapChain swapChain, Texture& backBuffer);

    /**
     * @brief Ventana de vista previa del framebuffer/render.
     * @param window Ventana de aplicación.
     * @param renderTexture Textura a mostrar.
     */
    void Renderer(Window window, ID3D11ShaderResourceView* renderTexture);

    /** @brief Inspector general de propiedades de un actor. */
    void inspectorGeneral(EU::TSharedPointer<Actor> actor);

    /** @brief Inspector para componentes contenedores de un actor. */
    void inspectorContainer(EU::TSharedPointer<Actor> actor);

    /**
     * @brief Consola de logs: lee LogManager::instance().entries() y las muestra
     * filtrables por nivel (Message/Warning/Error), con botón "Clear" y auto-scroll.
     */
    void output();

    /**
     * @brief Panel de estadísticas en tiempo real: FPS, frame-time (ms) y draw calls.
     * @param fps         FPS suavizado (EMA).
     * @param frameTimeMs Tiempo del último frame en milisegundos.
     * @param drawCalls   Draw calls emitidos en el último frame.
     */
    void statsPanel(float fps, float frameTimeMs, unsigned int drawCalls);

    /** @brief Aplica estilo oscuro. */
    void darkStyle();

    /** @brief Aplica estilo gris neutro. */
    void greyStyle();

    /** @brief Aplica estilo inspirado en Visual Studio. */
    void visualStudioStyle();

    /** @brief Muestra tooltip con ícono y texto. */
    void ToolTip(std::string icon, std::string tip);

    /** @brief Muestra tooltip solo con texto. */
    void ToolTip(std::string tip);

    /** @brief Configura datos predefinidos para tooltips. */
    void toolTipData();

    /** @brief Renderiza barra de herramientas con accesos rápidos. */
    void ToolBar();

    /** @brief Solicita cierre de la aplicación. */
    void closeApp();

    /** @brief Ventana transparente que cubre toda la pantalla. */
    void RenderFullScreenTransparentWindow();

    /**
     * @brief Muestra la lista jerárquica de actores en escena ("Hierarchy").
     * @param actors Lista de actores de la escena, en el mismo orden/índice que usa
     *               `selectedActorIndex` para referenciarlos.
     * @details Además de listar y seleccionar, incluye:
     *  - Botón "+ Add Light" con un popup para crear un Light Actor nuevo (ver
     *    `requestedLightType`).
     *  - Menú contextual (clic derecho sobre cualquier fila) con "Duplicate"/"Delete"
     *    (ver `requestedDuplicateIndex`/`requestedDeleteIndex`).
     * Esta función NO crea/destruye actores directamente — no tiene acceso al `Device`
     * necesario para construir un `Actor` nuevo ni a la escena para borrarlos. Solo
     * marca la intención en los miembros públicos de arriba; `BaseApp::update()` los
     * revisa cada frame y ejecuta la operación real.
     */
    void outliner(const std::vector<EU::TSharedPointer<Actor>>& actors);

    /**
     * @brief Panel de sombras/UI general. La edición de luces (dirección/color/intensidad/
     * range/ángulo) vive ahora en el Inspector, por Light Actor — ver `inspectorGeneral()`.
     * @param shadowBias Bias del shadow map (ajusta shadow acne vs Peter Pan).
     * @param viewShadowMap Checkbox "Ver Shadow Map": visualiza el shadow map (DebugViewMode 7
     *                      en DeferredLighting.hlsl) en el viewport principal.
     */
    void lightPanel(float& shadowBias, bool& viewShadowMap);

    /**
     * @brief Panel "Interface": ajustes de la UI misma (p.ej. escala de fuente). Es su propia
     * ventana colapsable, independiente de "Lighting" (antes vivía anidado ahí por error).
     */
    void interfacePanel();

public:
    int selectedActorIndex = -1; ///< Índice del actor actualmente seleccionado.

    /**
     * @brief Solicitud de creación de un Light Actor desde el botón "+" del outliner.
     * -1 = sin solicitud. Si no, es un valor de `LightType` (Directional/Point/Spot) pedido
     * este frame. `BaseApp::update()` la consume (crea el actor) y la resetea a -1.
     */
    int requestedLightType = -1;

    /**
     * @brief Índice (en el vector de actores) solicitado para duplicar/borrar desde el menú
     * contextual (clic derecho) del outliner. -1 = sin solicitud. `BaseApp::update()` las
     * consume y las resetea a -1 cada frame.
     */
    int requestedDuplicateIndex = -1;
    int requestedDeleteIndex    = -1;

private:
    bool checkboxValue = true;   ///< Ejemplo de valor booleano para UI.
    bool checkboxValue2 = false; ///< Segundo valor booleano.
    std::vector<const char*> m_objectsNames; ///< Lista de nombres de objetos.
    std::vector<const char*> m_tooltips;     ///< Lista de tooltips de la UI.

    bool show_exit_popup = false; ///< Control para mostrar popup de salida.
    bool m_imguiInitialized = false; ///< Bandera de inicialización de ImGui.
    ImFont* m_mainFont = nullptr; ///< Fuente principal cargada desde archivo.
};
