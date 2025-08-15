/**
 * @file UserInterface.h
 * @brief Declaración de la clase UserInterface para gestionar ImGui en The Visionary Engine.
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
 */

#pragma once
#include "Prerequisites.h"
#include "imgui.h"
#include <imgui_internal.h>
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
     * @brief Control de tipo slider para modificar un vector3.
     * @param label Texto identificador.
     * @param values Puntero a tres floats (x, y, z).
     * @param resetValues Valor por defecto para reinicio.
     * @param columnWidth Ancho de la columna de etiqueta.
     *
     * @note Muy útil en motores para mover objetos en el espacio.
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

    /** @brief Ventana de consola/log de salida. */
    void output();

    /** @brief Aplica estilo oscuro. */
    void darkStyle();

    /** @brief Aplica estilo gris neutro. */
    void greyStyle();

    /** @brief Aplica estilo similar a GameMaker Studio. */
    void GameMakerStyle();

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

    /** @brief Muestra la lista jerárquica de actores en escena. */
    void outliner(const std::vector<EU::TSharedPointer<Actor>>& actors);

public:
    int selectedActorIndex = -1; ///< Índice del actor actualmente seleccionado.

private:
    bool checkboxValue = true;   ///< Ejemplo de valor booleano para UI.
    bool checkboxValue2 = false; ///< Segundo valor booleano.
    std::vector<const char*> m_objectsNames; ///< Lista de nombres de objetos.
    std::vector<const char*> m_tooltips;     ///< Lista de tooltips de la UI.

    bool show_exit_popup = false; ///< Control para mostrar popup de salida.
    bool m_imguiInitialized = false; ///< Bandera de inicialización de ImGui.
};
