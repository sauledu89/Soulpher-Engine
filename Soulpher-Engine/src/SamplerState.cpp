/**
 * @file SamplerState.cpp
 * @brief Implementación de la creación y uso de estados de muestreo de texturas en Direct3D 11.
 *
 * @details
 * Un **Sampler State** define cómo el motor gráfico lee (muestrea) los píxeles de una textura
 * cuando se renderiza. Controla aspectos como:
 *  - Filtros (linear, point, anisotrópico).
 *  - Dirección de repetición (wrap, clamp, mirror).
 *  - Nivel de detalle (LOD).
 *
 * @note
 * En videojuegos, un buen control de `SamplerState` es importante para:
 *  - Evitar texturas pixeladas (usar filtrado lineal o anisotrópico).
 *  - Controlar repeticiones en mallas grandes (wrap vs clamp).
 *  - Mejorar la calidad visual sin sacrificar demasiado rendimiento.
 */

#include "SamplerState.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el estado de muestreo de texturas.
  *
  * @param device  Referencia al dispositivo Direct3D 11.
  * @return HRESULT Devuelve `S_OK` si se crea correctamente, o un código de error si falla.
  *
  * @details
  * En este caso se crea un sampler con:
  *  - **Filtrado lineal** en minificación, magnificación y mipmaps (`MIN_MAG_MIP_LINEAR`).
  *  - **Dirección wrap** en U, V y W (la textura se repite indefinidamente).
  *  - Sin comparación de profundidad (`COMPARISON_NEVER`).
  *  - Rango de LOD desde 0 hasta el máximo permitido.
  *
  * @note
  * Este es un sampler estándar muy usado para texturas 2D comunes en videojuegos.
  */
HRESULT SamplerState::init(Device& device) {
    if (!device.m_device) {
        ERROR("SamplerState", "init", "Device is nullptr");
        return E_POINTER;
    }

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = device.CreateSamplerState(&sampDesc, &m_sampler);
    if (FAILED(hr)) {
        ERROR("SamplerState", "init", "Failed to create SamplerState");
        return hr;
    }

    return S_OK;
}

/**
 * @brief Actualiza el estado de muestreo.
 *
 * @note
 * En este caso no hay lógica dinámica, pero en un motor más complejo podría
 * cambiarse el filtro o el modo de repetición en tiempo real.
 */
void SamplerState::update() {
    // No hay lógica de actualización para un sampler en este caso.
}

/**
 * @brief Asigna el sampler al pipeline de renderizado en el **Pixel Shader**.
 *
 * @param deviceContext  Contexto del dispositivo.
 * @param StartSlot      Primer slot donde se asignará el sampler.
 * @param NumSamplers    Número de samplers a asignar.
 *
 * @note
 * Esto permite que el shader aplique el filtrado y la repetición configurada
 * al acceder a las texturas.
 */
void SamplerState::render(DeviceContext& deviceContext,
    unsigned int StartSlot,
    unsigned int NumSamplers) {
    if (!m_sampler) {
        ERROR("SamplerState", "render", "SamplerState is nullptr");
        return;
    }

    deviceContext.PSSetSamplers(StartSlot, NumSamplers, &m_sampler);
}

/**
 * @brief Libera el recurso asociado al sampler.
 *
 * @note
 * Es importante destruir el sampler para evitar **fugas de memoria** (memory leaks)
 * al cerrar el juego o al recargar recursos.
 */
void SamplerState::destroy() {
    if (m_sampler) {
        SAFE_RELEASE(m_sampler);
    }
}
