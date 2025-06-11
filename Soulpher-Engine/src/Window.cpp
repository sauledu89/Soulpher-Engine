/**
 * @file Window.cpp
 * @brief Implementaci�n de la clase Window.
 */

#include "Window.h"

 /**
  * @brief Inicializa la ventana principal de la aplicaci�n.
  *
  * Este m�todo registra una clase de ventana de Win32 y crea una ventana visible
  * con un tama�o predefinido. Tambi�n guarda los datos esenciales como el `HWND` y dimensiones.
  *
  * @param hInstance Instancia de la aplicaci�n (provista por WinMain).
  * @param nCmdShow Par�metro que define c�mo se mostrar� la ventana (normal, minimizada, etc.).
  * @param wndproc Funci�n que manejar� los mensajes de Windows (eventos).
  * @return HRESULT S_OK si fue exitosa; E_FAIL si algo falla.
  */
HRESULT Window::init(HINSTANCE hInstance, int nCmdShow, WNDPROC wndproc) {
    m_hInst = hInstance;

    // 1. Configuramos la clase de ventana
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;  // Redibujar si se redimensiona
    wcex.lpfnWndProc = wndproc;            // Funci�n que recibe eventos (mouse, teclado, etc.)
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInst;
    wcex.hIcon = LoadIcon(m_hInst, (LPCTSTR)IDI_TUTORIAL1); // �cono de la ventana
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);             // Cursor est�ndar
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);        // Fondo blanco
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = "TutorialWindowClass";             // Nombre interno
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1); // �cono peque�o

    // 2. Registramos la clase de ventana con el sistema operativo
    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, "RegisterClassEx failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR RegisterClassEx");
        return E_FAIL;
    }

    // 3. Definimos el tama�o interno del cliente (�rea �til de la ventana)
    RECT rc = { 0, 0, 1200, 1010 };
    m_rect = rc;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);  // Ajusta para bordes y barra de t�tulo

    // 4. Creamos la ventana en pantalla
    m_hWnd = CreateWindow(
        "TutorialWindowClass",        // Clase registrada
        m_windowName.c_str(),         // T�tulo de la ventana
        WS_OVERLAPPEDWINDOW,          // Estilo cl�sico de ventana con bordes
        CW_USEDEFAULT, CW_USEDEFAULT, // Posici�n en pantalla
        rc.right - rc.left,           // Ancho
        rc.bottom - rc.top,           // Alto
        nullptr, nullptr, hInstance, nullptr);

    // 5. Validaci�n
    if (!m_hWnd) {
        MessageBox(nullptr, "CreateWindow failed!", "Error", MB_OK);
        ERROR("Window", "init", "CHECK FOR CreateWindow()");
        return E_FAIL;
    }

    // 6. Mostramos la ventana y la actualizamos
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // 7. Obtenemos tama�o real del �rea del cliente
    GetClientRect(m_hWnd, &m_rect);
    m_width = m_rect.right - m_rect.left;
    m_height = m_rect.bottom - m_rect.top;

    return S_OK;
}

/**
 * @brief M�todo de actualizaci�n de la ventana.
 *
 * Actualmente no hace nada, pero se puede usar para detectar eventos del sistema.
 */
void Window::update() {
    // Placeholder por si se quiere agregar l�gica de manejo de eventos personalizados.
}

/**
 * @brief M�todo de renderizado de la ventana.
 *
 * Normalmente no se usa directamente ya que DirectX se encarga del render.
 */
void Window::render() {
    // Aqu� podr�as dibujar UI con GDI si no est�s usando DirectX.
}

/**
 * @brief Libera recursos asociados con la ventana.
 *
 * Actualmente no hace nada porque WinMain se encarga del ciclo de vida,
 * pero es buena pr�ctica tenerlo definido.
 */
void Window::destroy() {
    // Future-proof: �til si quieres destruir o recrear la ventana.
}
