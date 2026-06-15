#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file DepthStencilState.h
 * @brief Encapsula la configuración del estado de profundidad en Direct3D 11.
 *
 * @details
 * Controla la prueba de profundidad (Depth Test) del pipeline gráfico:
 * determina si un píxel se dibuja comparando su valor Z con el almacenado en el Depth Buffer.
 *
 * @note [GameDev] En el Deferred Renderer se necesitan TRES estados distintos:
 * - **Shadow pass**: depth enabled, write ON, LESS → escribe el shadow map.
 * - **Lighting pass (transparent)**: depth enabled, write OFF (ZERO), LESS_EQUAL → lee pero no
 *   sobreescribe el depth de los opacos (evita z-fighting entre opacos y transparentes).
 * - **Lighting pass (disabled)**: depth disabled → el fullscreen quad no compite con el depth
 *   del G-buffer (siempre queremos que el quad de iluminación "gane").
 * Esto requiere crear tres instancias de DepthStencilState con distintos parámetros.
 * El Stencil Buffer (no expuesto en esta interfaz simplificada) tiene usos creativos:
 * Doom 2016 lo usa para Mega Texture streaming; juegos de terror lo usan para portales y espejos.
 */
class DepthStencilState {
public:
    /** @brief Constructor por defecto. */
    DepthStencilState() = default;

    /** @brief Destructor por defecto. */
    ~DepthStencilState() = default;

    /**
     * @brief Inicializa el estado de profundidad.
     * @param device      Dispositivo Direct3D para crear el estado.
     * @param enableDepth `true` para habilitar la prueba de profundidad.
     * @param writeMask   Máscara de escritura: `D3D11_DEPTH_WRITE_MASK_ALL` escribe profundidad,
     *                    `D3D11_DEPTH_WRITE_MASK_ZERO` la lee sin modificarla.
     * @param compareFunc Función de comparación (`D3D11_COMPARISON_LESS` por defecto).
     * @return `S_OK` si la inicialización fue exitosa; código de error en caso contrario.
     *
     * @note Usa `writeMask = D3D11_DEPTH_WRITE_MASK_ZERO` para el lighting pass deferred:
     * lee el depth del G-buffer sin sobreescribirlo con el fullscreen quad.
     */
    HRESULT init(Device& device,
                 bool                  enableDepth = true,
                 D3D11_DEPTH_WRITE_MASK writeMask  = D3D11_DEPTH_WRITE_MASK_ALL,
                 D3D11_COMPARISON_FUNC  compareFunc = D3D11_COMPARISON_LESS);

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
