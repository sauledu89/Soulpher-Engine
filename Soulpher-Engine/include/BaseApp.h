#pragma once
#include "Prerequisites.h"
#include "Window.h"

/**
 * @file BaseApp.h
 * @class BaseApp
 * @brief Clase base para una aplicación de motor gráfico con DirectX.
 *
 * Define la estructura fundamental de una aplicación basada en un motor gráfico.
 * Gestiona el ciclo de vida completo de la aplicación:
 * - Inicialización de recursos.
 * - Actualización por frame.
 * - Renderizado del contenido.
 * - Liberación de recursos.
 *
 * También contiene la lógica para iniciar la ventana y ejecutar el bucle principal.
 *
 * @note Esta clase sirve como punto de partida para motores gráficos modulares.
 * Permite derivar subclases específicas como editores, juegos, o simuladores.
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
     * @brief Inicializa los componentes de la aplicación.
     *
     * Este método debe ser sobrescrito o extendido para inicializar:
     * - El dispositivo gráfico (Direct3D 11).
     * - Recursos como texturas, buffers y shaders.
     * - Subsistemas como audio, física o input.
     *
     * @return HRESULT Devuelve S_OK si la inicialización fue exitosa.
     *
     * @note En un motor real, este método es vital para preparar la escena o cargar el nivel.
     */
    HRESULT init();

    /**
     * @brief Actualiza la lógica de la aplicación una vez por frame.
     *
     * Aquí se deben manejar todos los cálculos relacionados con:
     * - Movimiento de objetos.
     * - Lógica del juego.
     * - Animaciones.
     * - Actualización del estado interno.
     *
     * @note Este método representa el "Update()" típico de un game loop.
     */
    void update();

    /**
     * @brief Renderiza un frame de la aplicación.
     *
     * Este método se encarga de:
     * - Limpiar el back buffer.
     * - Configurar el pipeline de renderizado.
     * - Dibujar la escena.
     * - Presentar el frame a la pantalla.
     *
     * @note En un motor completo, se podría dividir entre renderizado 3D, UI, y postprocesado.
     */
    void render();

    /**
     * @brief Libera todos los recursos usados por la aplicación.
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
     * @brief Ejecuta el bucle principal de la aplicación.
     *
     * Inicializa la ventana principal y entra en el bucle de mensajes de Windows.
     * Este método representa el punto de entrada del motor desde WinMain.
     *
     * @param hInstance Instancia del proceso actual.
     * @param hPrevInstance Instancia anterior (obsoleta en Win32, usualmente NULL).
     * @param lpCmdLine Argumentos recibidos desde línea de comandos.
     * @param nCmdShow Modo de visualización inicial de la ventana.
     * @param wndproc Función de callback que procesa mensajes de Windows (WM_*).
     * @return int Código de salida de la aplicación.
     *
     * @note Este método combina la arquitectura Win32 con la lógica del motor.
     * Ideal para crear editores gráficos o juegos de escritorio.
     */
    int run(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow,
        WNDPROC wndproc);

private:
    /**
     * @brief Ventana principal asociada a la aplicación.
     *
     * Controla la creación, visualización y eventos de la ventana Win32.
     * Se utiliza internamente para establecer el contexto gráfico.
     */
    Window m_window;
};
