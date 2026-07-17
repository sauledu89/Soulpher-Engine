/**
 * @file SamplerState.cpp
 * @brief Implementaci�n de la creaci�n y uso de estados de muestreo de texturas en Direct3D 11.
 *
 * @details
 * Un **Sampler State** define c�mo el motor gr�fico lee (muestrea) los p�xeles de una textura
 * cuando se renderiza. Controla aspectos como:
 *  - Filtros (linear, point, anisotr�pico).
 *  - Direcci�n de repetici�n (wrap, clamp, mirror).
 *  - Nivel de detalle (LOD).
 *
 * @note
 * En videojuegos, un buen control de `SamplerState` es importante para:
 *  - Evitar texturas pixeladas (usar filtrado lineal o anisotr�pico).
 *  - Controlar repeticiones en mallas grandes (wrap vs clamp).
 *  - Mejorar la calidad visual sin sacrificar demasiado rendimiento.
 *
 * @note [GameDev] Este sampler usa MIN_MAG_MIP_LINEAR (trilinear) + WRAP para texturas
 * de color. Para el shadow map se necesita un sampler de COMPARACION (D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
 * + D3D11_COMPARISON_LESS_EQUAL) que permite usar SampleCmpLevelZero() en HLSL.
 * En Soulpher-Engine el shadow map usa txShadow.Sample() en vez de SampleCmp, lo que
 * significa que la comparacion la hace el shader en HLSL (ComputeShadow). Una extension
 * natural seria crear un segundo SamplerState de comparacion para PCF hardware-accelerated.
 */

#include "SamplerState.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el estado de muestreo de texturas.
  *
  * @param device  Referencia al dispositivo Direct3D 11.
  * @return HRESULT Devuelve `S_OK` si se crea correctamente, o un c�digo de error si falla.
  *
  * @details
  * En este caso se crea un sampler con:
  *  - **Filtrado lineal** en minificaci�n, magnificaci�n y mipmaps (`MIN_MAG_MIP_LINEAR`).
  *  - **Direcci�n wrap** en U, V y W (la textura se repite indefinidamente).
  *  - Sin comparaci�n de profundidad (`COMPARISON_NEVER`).
  *  - Rango de LOD desde 0 hasta el m�ximo permitido.
  *
  * @note
  * Este es un sampler est�ndar muy usado para texturas 2D comunes en videojuegos.
  */
HRESULT SamplerState::init(Device& device) {
    if (!device.m_device) {
        LOG_ERROR("SamplerState", "init", "Device is nullptr");
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
        LOG_ERROR("SamplerState", "init", "Failed to create SamplerState");
        return hr;
    }

    return S_OK;
}

/**
 * @brief Actualiza el estado de muestreo.
 *
 * @note
 * En este caso no hay l�gica din�mica, pero en un motor m�s complejo podr�a
 * cambiarse el filtro o el modo de repetici�n en tiempo real.
 */
void SamplerState::update() {
    // No hay l�gica de actualizaci�n para un sampler en este caso.
}

/**
 * @brief Asigna el sampler al pipeline de renderizado en el **Pixel Shader**.
 *
 * @param deviceContext  Contexto del dispositivo.
 * @param StartSlot      Primer slot donde se asignar� el sampler.
 * @param NumSamplers    N�mero de samplers a asignar.
 *
 * @note
 * Esto permite que el shader aplique el filtrado y la repetici�n configurada
 * al acceder a las texturas.
 */
void SamplerState::render(DeviceContext& deviceContext,
    unsigned int StartSlot,
    unsigned int NumSamplers) {
    if (!m_sampler) {
        LOG_ERROR("SamplerState", "render", "SamplerState is nullptr");
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
