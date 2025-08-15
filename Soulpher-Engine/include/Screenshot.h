#pragma once
#include "Prerequisites.h"
#include "UserInterface.h"

class Window;
class SwapChain;
class Texture;

/**
 * @class Screenshot
 * @brief Gestiona la captura de pantallas del contenido renderizado.
 *
 * @details
 * Esta clase permite capturar el contenido del **back buffer** y guardarlo como una imagen.
 * También ofrece una interfaz gráfica (UI) para que el usuario pueda
 * realizar capturas directamente desde el motor.
 *
 * 🔹 **Conceptos clave para estudiantes de videojuegos:**
 * - El **back buffer** es la imagen que se está renderizando en la GPU antes de mostrarse en pantalla.
 * - Capturarlo permite guardar "frames" exactos del renderizado.
 * - El proceso suele implicar copiar datos desde la GPU a la CPU y luego guardarlos en un formato de imagen.
 *
 * @note
 * En motores de juegos, la captura de pantalla puede usarse para depuración, marketing, o generar
 * imágenes de referencia para comparar mejoras gráficas.
 */
class Screenshot {
public:
    Screenshot() = default;  ///< Constructor por defecto.
    ~Screenshot() = default; ///< Destructor por defecto.

    /**
     * @brief Captura una imagen del contenido actual del back buffer.
     *
     * @param window Ventana activa desde la cual se captura la imagen.
     * @param swapChain Cadena de intercambio asociada (contiene el back buffer).
     * @param backBuffer Textura que representa el contenido actual renderizado.
     *
     * @note
     * El back buffer normalmente es una textura de la GPU; para guardarla como imagen,
     * debe copiarse a una textura accesible por la CPU.
     */
    void captureScreenshot(Window window, SwapChain swapChain, Texture& backBuffer);

    /**
     * @brief Dibuja la interfaz de usuario para la captura de pantalla.
     *
     * @param window Ventana activa.
     * @param swapChain Cadena de intercambio asociada.
     * @param backBuffer Textura actual que puede mostrarse o capturarse.
     *
     * @details
     * Este método renderiza botones y opciones en la UI para que el usuario
     * pueda capturar imágenes sin necesidad de comandos externos.
     */
    void ui(Window window, SwapChain swapChain, Texture& backBuffer);
};
