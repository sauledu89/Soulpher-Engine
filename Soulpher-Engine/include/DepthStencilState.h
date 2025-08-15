#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file DepthStencilState.h
 * @brief Encapsula la configuración del estado de profundidad y stencil en Direct3D 11.
 *
 * @details
 * Controla dos pruebas fundamentales en el pipeline gráfico:
 * - **Depth Test (Prueba de profundidad):** Determina si un píxel debe dibujarse
 *   comparando su valor de profundidad (Z) con el ya almacenado en el Depth Buffer.
 * - **Stencil Test (Prueba de máscara):** Permite dibujar sólo en ciertas regiones
 *   de la pantalla mediante una máscara (Stencil Buffer).
 *
 * @note Para estudiantes:
 * - El **Depth Test** es esencial para el renderizado 3D correcto: evita que objetos lejanos
 *   aparezcan sobre objetos cercanos.
 * - El **Stencil Test** se usa para efectos como espejos, portales, siluetas y máscaras.
 * - Cambiar este estado demasiado seguido puede impactar el rendimiento.
 */
class DepthStencilState {
public:
    /** @brief Constructor por defecto. */
    DepthStencilState() = default;

    /** @brief Destructor por defecto. */
    ~DepthStencilState() = default;

    /**
     * @brief Inicializa el estado de profundidad y stencil.
     * @param device Dispositivo de Direct3D para crear el estado.
     * @param enableDepth `true` para habilitar la prueba de profundidad.
     * @param enableStencil `true` para habilitar la prueba de stencil.
     * @return `S_OK` si la inicialización fue exitosa; código de error en caso contrario.
     *
     * @note
     * - Con `enableDepth = false`, todos los píxeles se dibujan sin comparar su Z.
     * - Con `enableStencil = true`, se habilita el uso de la máscara de stencil.
     */
    HRESULT init(Device& device, bool enableDepth = true, bool enableStencil = false);

    /**
     * @brief Actualiza parámetros internos.
     *
     * @note Actualmente no implementado, pero podría usarse para cambiar dinámicamente
     *       la función de comparación o las operaciones de stencil.
     */
    void update();

    /**
     * @brief Aplica el estado de profundidad y stencil al contexto de render.
     * @param deviceContext Contexto del dispositivo para operaciones gráficas.
     * @param stencilRef Valor de referencia usado en la prueba de stencil.
     * @param reset Si es `true`, restablece el estado por defecto del pipeline.
     *
     * @note
     * - `stencilRef` se compara con el valor del Stencil Buffer según la función configurada.
     * - En la mayoría de los casos, para un render 3D básico, `stencilRef` se deja en 0.
     */
    void render(DeviceContext& deviceContext, unsigned int stencilRef = 0, bool reset = false);

    /**
     * @brief Libera los recursos asociados al estado de profundidad y stencil.
     *
     * @note Siempre llamar antes de destruir el dispositivo para evitar fugas de memoria GPU.
     */
    void destroy();

private:
    ID3D11DepthStencilState* m_depthStencilState = nullptr; ///< Puntero al estado de profundidad/stencil de Direct3D.
};
