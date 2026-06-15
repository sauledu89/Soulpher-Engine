#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file BlendState.h
 * @brief Encapsulaci�n de un estado de blending en Direct3D 11.
 *
 * @details
 * El blending en gr�ficos 3D es el proceso de combinar el color resultante de un
 * fragmento (pixel) que se est� dibujando con el color ya presente en el render target.
 *
 * Esta clase:
 * - Crea un `ID3D11BlendState` a partir de configuraciones predefinidas.
 * - Aplica el estado al `DeviceContext` activo.
 * - Permite activar/desactivar efectos como transparencia alfa, aditivos, etc.
 *
 * @note Para estudiantes:
 * - El blending es esencial para efectos como **vidrios, humo, part�culas y UI semitransparente**.
 * - Cambiar el estado de blending en cada draw call puede afectar el rendimiento.
 * - Es recomendable agrupar los objetos con el mismo estado para minimizar cambios de estado.
 *
 * @note [GameDev] El orden de render con blending es critico: los objetos transparentes
 * DEBEN dibujarse despues de los opacos y de back-to-front (lejano antes que cercano).
 * Si se dibujan desordenados, el blending produce artefactos visuales (transparencias
 * cortadas o mal mezcladas). Esta es la razon por la que RenderScene separa
 * opaqueObjects de transparentObjects.
 * En Unreal Engine esto se maneja automaticamente con el TranslucencySortPolicy.
 * El blending aditivo (Additive) es el mas comun para particulas de fuego/luz ya que
 * suma energia al buffer: nunca oscurece, solo aclara.
 */
class BlendState {
public:
    /** @brief Constructor por defecto. */
    BlendState() = default;

    /** @brief Destructor por defecto. */
    ~BlendState() = default;

    /**
     * @brief Inicializa el estado de blending en la GPU.
     * @param device Dispositivo de Direct3D para la creaci�n del estado.
     * @return HRESULT que indica �xito (`S_OK`) o el tipo de error.
     *
     * @note Este m�todo suele configurarse con una descripci�n (`D3D11_BLEND_DESC`)
     *       que define el tipo de mezcla.
     */
    HRESULT init(Device& device);

    /**
     * @brief Actualiza par�metros internos (actualmente sin implementaci�n).
     *
     * @note Aqu� podr�a reconfigurarse el estado si se quisiera cambiar el tipo de blending en runtime.
     */
    void update() {};

    /**
     * @brief Aplica el estado de blending al contexto de render.
     * @param deviceContext Contexto de dispositivo sobre el que aplicar el estado.
     * @param blendFactor Array de 4 floats (RGBA) que modulan la mezcla.
     * @param sampleMask M�scara de muestreo de p�xeles (por defecto todos habilitados).
     * @param reset Si es `true`, restablece el estado por defecto del pipeline.
     *
     * @note
     * - `blendFactor` puede usarse para efectos como *fade in/out*.
     * - `sampleMask` rara vez se modifica, normalmente se deja como `0xffffffff`.
     * - Cambiar de blending habilitado a deshabilitado se hace normalmente con `reset = true`.
     */
    void render(DeviceContext& deviceContext,
        float* blendFactor = nullptr,
        unsigned int sampleMask = 0xffffffff,
        bool reset = false);

    /**
     * @brief Libera los recursos asociados al estado de blending.
     *
     * @note Siempre llamar antes de destruir el dispositivo para evitar fugas de memoria GPU.
     */
    void destroy();

private:
    ID3D11BlendState* m_blendState = nullptr; ///< Puntero al objeto de estado de blending de Direct3D.
};
