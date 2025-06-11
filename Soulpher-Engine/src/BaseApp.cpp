/**
 * @file BaseApp.cpp
 * @brief Implementación de la clase BaseApp.
 *
 * Esta clase sirve como punto de partida para cualquier aplicación que utilice el motor gráfico.
 * Define los métodos principales del ciclo de vida de una app: inicialización, actualización,
 * renderizado, destrucción y ejecución (`run`). Actualmente todos los métodos son placeholders
 * y deben ser sobreescritos por una clase derivada que implemente la lógica específica.
 */

#include "BaseApp.h"

 /**
  * @brief Inicializa la aplicación base.
  *
  * Este método debería sobreescribirse en la clase derivada para preparar los subsistemas
  * necesarios como gráficos, lógica, recursos, ventanas, etc.
  *
  * @return HRESULT Código de éxito o error (actualmente retorna E_NOTIMPL).
  */
HRESULT BaseApp::init() {
    return E_NOTIMPL;
}

/**
 * @brief Actualiza la lógica del juego o simulación.
 *
 * En una app real, este método debe manejar la lógica por frame, como entrada del usuario,
 * simulaciones físicas, IA, etc.
 */
void BaseApp::update() {
}

/**
 * @brief Llama a las rutinas de renderizado por cuadro.
 *
 * Este método debería contener todas las llamadas a render, como establecer viewports,
 * limpiar buffers, dibujar mallas y presentar el frame.
 */
void BaseApp::render() {
}

/**
 * @brief Libera los recursos de la aplicación.
 *
 * Este método se ejecuta antes de cerrar la app, y debe liberar memoria,
 * recursos gráficos y cualquier subsistema activo.
 */
void BaseApp::destroy() {
}

/**
 * @brief Ejecuta el ciclo principal de la aplicación.
 *
 * Esta función se encarga de crear la ventana, iniciar el loop de mensajes y coordinar
 * la ejecución del motor. En esta versión es un placeholder y siempre retorna 0.
 *
 * @param hInstance Instancia de la aplicación.
 * @param hPrevInstance (Obsoleto) Instancia anterior.
 * @param lpCmdLine Argumentos de línea de comandos.
 * @param nCmdShow Modo de visualización de la ventana.
 * @param wndproc Función de ventana para manejar mensajes del sistema.
 * @return int Código de salida de la aplicación.
 */
int BaseApp::run(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow,
    WNDPROC wndproc) {
    return 0;
}
