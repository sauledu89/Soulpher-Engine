/**
 * @file Window.h
 * @brief Declaración de la clase Window para la gestión básica de ventanas Win32.
 *
 * @details
 * Esta clase encapsula las operaciones necesarias para crear y administrar
 * la ventana principal de un motor de videojuegos basado en Direct3D.
 *
 * ---
 * 🔹 **Conceptos clave para estudiantes**:
 * - En Win32, una ventana es la "superficie" donde Direct3D dibuja.
 * - El **handle (HWND)** identifica la ventana en el sistema operativo.
 * - El **bucle de ventana** (update/render) mantiene el programa en ejecución.
 * - El **procedimiento de ventana (WNDPROC)** gestiona los eventos como teclado, ratón, redimensionado, etc.
 *
 * ---
 * 💡 **Relación con otros sistemas**:
 * - Direct3D necesita el HWND para crear el **swap chain**.
 * - El tamaño de la ventana (`m_width`, `m_height`) se usa para inicializar el **viewport**.
 * - Se conecta con el **UserInterface** y otros sistemas para sincronizar entrada y salida.
 */

#pragma once
#include "Prerequisites.h"

 /**
  * @class Window
  * @brief Administra la ventana principal de la aplicación.
  *
  * @note Parte del motor gráfico **The Visionary**.
  */
class Window {
public:
    /** @brief Constructor por defecto: no crea la ventana. */
    Window() = default;

    /** @brief Destructor por defecto: no libera recursos automáticamente. */
    ~Window() = default;

    /**
     * @brief Inicializa y crea la ventana Win32.
     * @param hInstance Instancia de la aplicación (proporcionada por WinMain/wWinMain).
     * @param nCmdShow Modo de visualización inicial (ej. SW_SHOW).
     * @param wndproc Procedimiento de ventana (función callback para eventos).
     * @return HRESULT indicando éxito (`S_OK`) o error.
     *
     * @warning Llamar antes de inicializar Direct3D, ya que el HWND es necesario para el swap chain.
     */
    HRESULT init(HINSTANCE hInstance, int nCmdShow, WNDPROC wndproc);

    /**
     * @brief Procesa mensajes y actualiza el estado de la ventana (por cuadro).
     *
     * @note Este método normalmente se llama en cada iteración del bucle principal del motor.
     */
    void update();

    /**
     * @brief Llama a las rutinas de render asociadas a la ventana.
     *
     * @note No dibuja directamente: delega en el sistema de render del motor.
     */
    void render();

    /**
     * @brief Libera recursos y destruye la ventana.
     *
     * @note Debe llamarse antes de cerrar la aplicación para evitar fugas de memoria.
     */
    void destroy();

public:
    HWND m_hWnd = nullptr; ///< Handle de la ventana Win32.
    unsigned int m_width;  ///< Ancho actual de la ventana (px).
    unsigned int m_height; ///< Alto actual de la ventana (px).

private:
    HINSTANCE m_hInst = nullptr; ///< Instancia de la aplicación.
    RECT m_rect;                 ///< Área de cliente (coordenadas internas de la ventana).
    std::string m_windowName = "The Visionary Engine"; ///< Título/nombre de la ventana.
};
