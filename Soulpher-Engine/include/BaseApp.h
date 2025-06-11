#pragma once
#include "Prerequisites.h"
#include "Window.h"

/**
 * @file BaseApp.h
 * @class BaseApp
 * @brief Clase base para una aplicaci�n de motor gr�fico con DirectX.
 *
 * Define la estructura fundamental de una aplicaci�n basada en un motor gr�fico.
 * Gestiona el ciclo de vida completo de la aplicaci�n:
 * - Inicializaci�n de recursos.
 * - Actualizaci�n por frame.
 * - Renderizado del contenido.
 * - Liberaci�n de recursos.
 *
 * Tambi�n contiene la l�gica para iniciar la ventana y ejecutar el bucle principal.
 *
 * @note Esta clase sirve como punto de partida para motores gr�ficos modulares.
 * Permite derivar subclases espec�ficas como editores, juegos, o simuladores.
 */
class BaseApp {
public:
    /**
     * @brief Constructor por defecto.
     */
    BaseApp() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~BaseApp() = default;

    /**
     * @brief Inicializa los componentes de la aplicaci�n.
     *
     * Este m�todo debe ser sobrescrito o extendido para inicializar:
     * - El dispositivo gr�fico (Direct3D 11).
     * - Recursos como texturas, buffers y shaders.
     * - Subsistemas como audio, f�sica o input.
     *
     * @return HRESULT Devuelve S_OK si la inicializaci�n fue exitosa.
     *
     * @note En un motor real, este m�todo es vital para preparar la escena o cargar el nivel.
     */
    HRESULT init();

    /**
     * @brief Actualiza la l�gica de la aplicaci�n una vez por frame.
     *
     * Aqu� se deben manejar todos los c�lculos relacionados con:
     * - Movimiento de objetos.
     * - L�gica del juego.
     * - Animaciones.
     * - Actualizaci�n del estado interno.
     *
     * @note Este m�todo representa el "Update()" t�pico de un game loop.
     */
    void update();

    /**
     * @brief Renderiza un frame de la aplicaci�n.
     *
     * Este m�todo se encarga de:
     * - Limpiar el back buffer.
     * - Configurar el pipeline de renderizado.
     * - Dibujar la escena.
     * - Presentar el frame a la pantalla.
     *
     * @note En un motor completo, se podr�a dividir entre renderizado 3D, UI, y postprocesado.
     */
    void render();

    /**
     * @brief Libera todos los recursos usados por la aplicaci�n.
     *
     * Debe llamarse al final del ciclo de vida para liberar:
     * - Buffers de GPU.
     * - Recursos de textura y shaders.
     * - Memoria del sistema asociada.
     *
     * @note Fundamental para evitar memory leaks y asegurar cierres limpios.
     */
    void destroy();

    /**
     * @brief Ejecuta el bucle principal de la aplicaci�n.
     *
     * Inicializa la ventana principal y entra en el bucle de mensajes de Windows.
     * Este m�todo representa el punto de entrada del motor desde WinMain.
     *
     * @param hInstance Instancia del proceso actual.
     * @param hPrevInstance Instancia anterior (obsoleta en Win32, usualmente NULL).
     * @param lpCmdLine Argumentos recibidos desde l�nea de comandos.
     * @param nCmdShow Modo de visualizaci�n inicial de la ventana.
     * @param wndproc Funci�n de callback que procesa mensajes de Windows (WM_*).
     * @return int C�digo de salida de la aplicaci�n.
     *
     * @note Este m�todo combina la arquitectura Win32 con la l�gica del motor.
     * Ideal para crear editores gr�ficos o juegos de escritorio.
     */
    int run(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow,
        WNDPROC wndproc);

private:
    /**
     * @brief Ventana principal asociada a la aplicaci�n.
     *
     * Controla la creaci�n, visualizaci�n y eventos de la ventana Win32.
     * Se utiliza internamente para establecer el contexto gr�fico.
     */
    Window m_window;
};
