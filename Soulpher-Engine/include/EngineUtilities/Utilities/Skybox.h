/**
 * @file Skybox.h
 * @brief Clase base para el skybox de la escena.
 *
 * @details
 * Interfaz mínima requerida por DeferredRenderer para renderizar el skybox durante
 * el skybox pass. La implementación completa se añadirá cuando el profesor lo vea en clase.
 *
 * @note [GameDev] Un skybox es un cubo o esfera enorme centrado en la cámara que simula
 * el fondo del cielo/ambiente. Se dibuja DESPUÉS de los opacos con depth write OFF y
 * depth func LESS_EQUAL para que siempre quede "detrás" de la geometría (z=1 tras la
 * división perspectiva). En engines AAA se usa un "sky dome" con scattering atmosférico.
 *
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"

class DeviceContext;

/**
 * @class Skybox
 * @brief Renderizable de fondo: se dibuja en el skybox pass del DeferredRenderer.
 */
class Skybox {
public:
    Skybox()  = default;
    ~Skybox() = default;

    /** @brief Dibuja el skybox al render target activo. */
    void render(DeviceContext& deviceContext) {}

    /** @brief Libera recursos GPU del skybox. */
    void destroy() {}
};
