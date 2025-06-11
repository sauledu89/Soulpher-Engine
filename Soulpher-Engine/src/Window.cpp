/**
 * @file Window.cpp
 * @brief Implementación de la clase Window.
 */

#include "Window.h"

 /**
  * @brief Inicializa la ventana principal de la aplicación.
  *
  * Este método registra una clase de ventana de Win32 y crea una ventana visible
  * con un tamaño predefinido. También guarda los datos esenciales como el `HWND` y dimensiones.
  *
  * @param hInstance Instancia de la aplicación (provista por WinMain).
  * @param nCmdShow Parámetro que define cómo se mostrará la ventana (normal, minimizada, etc.).
  * @param wndproc Función que manejará los mensajes de Windows (eventos).
  * @return HRESULT S_OK si fue exitosa; E_FAIL si algo falla.
  */
HRESULT Window::init(HINSTANCE hInstance, int nCmdShow, WNDPROC wndproc) {
    m_hInst = hInstance;

    // 1. Configuramos la clase de ventana
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;  // Redibujar si se redimensiona
    wcex.lpfnWndProc = wndproc;            // Función que recibe eventos (mouse, teclado, etc.)
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInst;
    wcex.hIcon = LoadIcon(m_hInst, (LPCTSTR)IDI_TUTORIAL1); // Ícono de la ventana
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);             // Cursor estándar
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);        // Fondo blanco
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = "TutorialWindowClass";             // Nombre interno
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1); // Ícono pequeño

    // 2. Registramos la clase de ventana con el sistema operativo
    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, "RegisterClassEx failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR RegisterClassEx");
        return E_FAIL;
    }

    // 3. Definimos el tamaño interno del cliente (área útil de la ventana)
    RECT rc = { 0, 0, 1200, 1010 };
    m_rect = rc;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);  // Ajusta para bordes y barra de título

    // 4. Creamos la ventana en pantalla
    m_hWnd = CreateWindow(
        "TutorialWindowClass",        // Clase registrada
        m_windowName.c_str(),         // Título de la ventana
        WS_OVERLAPPEDWINDOW,          // Estilo clásico de ventana con bordes
        CW_USEDEFAULT, CW_USEDEFAULT, // Posición en pantalla
        rc.right - rc.left,           // Ancho
        rc.bottom - rc.top,           // Alto
        nullptr, nullptr, hInstance, nullptr);

    // 5. Validación
    if (!m_hWnd) {
        MessageBox(nullptr, "CreateWindow failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR CreateWindow()");
        return E_FAIL;
    }

    // 6. Mostramos la ventana y la actualizamos
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // 7. Obtenemos tamaño real del área del cliente
    GetClientRect(m_hWnd, &m_rect);
    m_width = m_rect.right - m_rect.left;
    m_height = m_rect.bottom - m_rect.top;

    return S_OK;
}

/**
 * @brief Método de actualización de la ventana.
 *
 * Actualmente no hace nada, pero se puede usar para detectar eventos del sistema.
 */
void Window::update() {
    // Placeholder por si se quiere agregar lógica de manejo de eventos personalizados.
}

/**
 * @brief Método de renderizado de la ventana.
 *
 * Normalmente no se usa directamente ya que DirectX se encarga del render.
 */
void Window::render() {
    // Aquí podrías dibujar UI con GDI si no estás usando DirectX.
}

/**
 * @brief Libera recursos asociados con la ventana.
 *
 * Actualmente no hace nada porque WinMain se encarga del ciclo de vida,
 * pero es buena práctica tenerlo definido.
 */
void Window::destroy() {
    // Future-proof: útil si quieres destruir o recrear la ventana.
}
