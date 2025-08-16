/**
 * @file Window.cpp
 * @brief Implementación de la clase Window para Soulpher-Engine.
 *
 * @details
 * Esta clase encapsula la creación y gestión de una ventana Win32, la cual es
 * el contenedor principal donde se renderiza todo el contenido del motor gráfico.
 *
 * En el contexto de desarrollo de videojuegos:
 * - La ventana es el primer elemento visual del motor.
 * - Define el área donde se mostrarán los gráficos de DirectX.
 * - Proporciona el handle (HWND) necesario para inicializar el dispositivo gráfico.
 *
 * @note
 * El motor usa la API Win32 para manejar la ventana, lo que significa que este código
 * solo funcionará en sistemas Windows. Cambiar a multiplataforma requeriría abstraer
 * esta parte y reemplazarla con bibliotecas como SDL o GLFW.
 */

#include "Window.h"

 /**
  * @brief Inicializa y crea una ventana Win32.
  *
  * @param hInstance  Instancia de la aplicación (provista por WinMain).
  * @param nCmdShow   Parámetro que indica cómo mostrar la ventana (provisto por WinMain).
  * @param wndproc    Función callback para procesar los mensajes de Windows.
  * @return HRESULT   S_OK si se creó la ventana correctamente, o código de error si falló.
  *
  * @note
  * Este método:
  *  - Registra la clase de ventana (WNDCLASSEX).
  *  - Crea la ventana con un tamaño inicial de 1200x1010 px.
  *  - Calcula el área cliente ajustada para el render.
  *
  * @par Ejemplo de uso:
  * @code
  * Window window;
  * if (FAILED(window.init(hInstance, nCmdShow, WndProc))) {
  *     return -1;
  * }
  * @endcode
  */
HRESULT Window::init(HINSTANCE hInstance, int nCmdShow, WNDPROC wndproc) {
    // Guarda la instancia de la aplicación
    m_hInst = hInstance;

    // Configuración y registro de la clase de ventana
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wndproc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInst;
    wcex.hIcon = LoadIcon(m_hInst, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = "TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);

    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, "RegisterClassEx failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR RegisterClassEx");
        return E_FAIL;
    }

    // Crea la ventana con tamaño inicial
    RECT rc = { 0, 0, 1200, 1010 };
    m_rect = rc;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindow(
        "TutorialWindowClass",
        m_windowName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_rect.right - m_rect.left,
        m_rect.bottom - m_rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hWnd) {
        MessageBox(nullptr, "CreateWindow failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR CreateWindow()");
        return E_FAIL;
    }

    // Muestra y actualiza la ventana
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Obtiene dimensiones del área cliente (donde se dibuja el juego)
    GetClientRect(m_hWnd, &m_rect);
    m_width = m_rect.right - m_rect.left;
    m_height = m_rect.bottom - m_rect.top;

    return S_OK;
}

/**
 * @brief Actualiza la lógica de la ventana.
 *
 * @note Actualmente vacío, pero en un motor real aquí podría:
 *  - Comprobar si la ventana fue redimensionada.
 *  - Procesar eventos de entrada adicionales.
 */
void Window::update() {
}

/**
 * @brief Renderiza elementos propios de la ventana.
 *
 * @note En este motor, la ventana no tiene contenido propio que renderizar.
 *       Todo el dibujo lo gestiona DirectX en el área cliente.
 */
void Window::render() {
}

/**
 * @brief Libera recursos asociados a la ventana.
 *
 * @note
 * Actualmente vacío porque la API Win32 libera automáticamente
 * los recursos al cerrar la aplicación, pero se podría:
 *  - Desregistrar la clase de ventana.
 *  - Liberar handles y recursos manualmente.
 */
void Window::destroy() {
}
