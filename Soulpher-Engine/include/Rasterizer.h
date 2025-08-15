#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class Rasterizer
 * @brief Administra el estado de rasterización en Direct3D 11.
 *
 * @details
 * En Direct3D, el rasterizador se encarga de convertir los vértices procesados por el pipeline
 * en fragmentos/píxeles listos para la etapa de pixel shader.
 * Mediante el **Rasterizer State** podemos configurar:
 * - **Cull Mode**: Qué caras de los polígonos se descartan (Back, Front o None).
 * - **Fill Mode**: Cómo se dibujan las primitivas (sólidas o en wireframe).
 * - Parámetros adicionales como profundidad de sesgo (Depth Bias) o recorte de viewports.
 *
 * Esta clase encapsula:
 *  - Creación de `ID3D11RasterizerState`.
 *  - Configuración del estado.
 *  - Asignación al pipeline.
 *  - Liberación de recursos.
 *
 * @note Entender el estado de rasterización es clave para optimizar rendimiento y depurar geometría
 *       (por ejemplo, usando Wireframe para ver la topología de malla).
 */
class Rasterizer {
public:
    /** @brief Constructor por defecto. */
    Rasterizer() = default;

    /** @brief Destructor por defecto. */
    ~Rasterizer() = default;

    /**
     * @brief Inicializa el estado de rasterización.
     * @param device Dispositivo Direct3D donde se creará el estado.
     * @return HRESULT que indica éxito o error de la operación.
     *
     * @note Aquí es donde defines el `D3D11_RASTERIZER_DESC` con parámetros como:
     *  - `FillMode` → D3D11_FILL_SOLID o D3D11_FILL_WIREFRAME.
     *  - `CullMode` → D3D11_CULL_BACK, D3D11_CULL_FRONT o D3D11_CULL_NONE.
     *  - `FrontCounterClockwise` → Orientación de las caras.
     */
    HRESULT init(Device device);

    /**
     * @brief Actualiza parámetros internos si es necesario.
     *
     * @note Útil si queremos cambiar dinámicamente el FillMode o CullMode sin recrear todo el estado.
     */
    void update();

    /**
     * @brief Aplica el estado de rasterización al contexto de dispositivo.
     * @param deviceContext Contexto de dispositivo donde aplicar el estado.
     *
     * @note Se suele llamar una vez por frame antes de dibujar las mallas.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera los recursos asociados.
     *
     * @warning Siempre llamar antes de destruir el objeto para evitar fugas de memoria.
     */
    void destroy();

private:
    /**
     * @brief Puntero al estado de rasterización de Direct3D 11.
     *
     * @note Este puntero debe ser liberado con `SAFE_RELEASE` para evitar memory leaks.
     */
    ID3D11RasterizerState* m_rasterizerState = nullptr;
};
