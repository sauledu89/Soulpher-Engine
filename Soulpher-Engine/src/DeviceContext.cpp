/**
 * @file DeviceContext.cpp
 * @brief Implementación de la clase DeviceContext.
 *
 * Esta clase encapsula el contexto del dispositivo (`ID3D11DeviceContext`),
 * que es la responsable de ejecutar comandos de renderizado, como:
 * - Limpiar buffers
 * - Dibujar
 * - Establecer viewports
 *
 * Todos los comandos enviados al pipeline gráfico pasan por aquí.
 */

#include "DeviceContext.h"

 /**
  * @brief Asigna uno o más viewports al pipeline gráfico.
  *
  * El viewport define en qué parte de la pantalla (o textura) se dibuja.
  * Generalmente se usa uno solo, pero también permite varios (ej: split screen).
  *
  * @param NumViewports Número de viewports a establecer.
  * @param pViewports Arreglo de estructuras D3D11_VIEWPORT.
  */
void DeviceContext::RSSetViewports(unsigned int NumViewports,
	const D3D11_VIEWPORT* pViewports) {

	if (NumViewports > 0 && pViewports != nullptr) {
		m_deviceContext->RSSetViewports(NumViewports, pViewports);
	}
	else {
		ERROR("DeviceContext", "RSSetViewports", "pViewports is nullptr or NumViewports is 0");
	}
}

/**
 * @brief Limpia el depth-stencil view (Z-buffer y stencil buffer).
 *
 * Esto es fundamental antes de dibujar una nueva escena. Si no se limpia,
 * las comparaciones de profundidad podrían generar errores visuales.
 *
 * @param pDepthStencilView Vista del buffer de profundidad a limpiar.
 * @param ClearFlags Indicadores de qué limpiar (profundidad, stencil, o ambos).
 * @param Depth Valor de profundidad a establecer (normalmente 1.0f).
 * @param Stencil Valor de stencil (usualmente 0).
 */
void DeviceContext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
	unsigned int ClearFlags,
	float Depth,
	UINT8 Stencil) {

	if (!m_deviceContext || !pDepthStencilView) {
		ERROR("DeviceContext", "ClearDepthStencilView", "Device context or depth stencil view is null.");
		return;
	}

	m_deviceContext->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
}

/**
 * @brief Limpia el render target view (RTV), es decir, el color de fondo.
 *
 * Este método establece un color base en toda la pantalla o textura
 * antes de comenzar a dibujar objetos encima.
 *
 * @param pRenderTargetView Vista del render target (backbuffer).
 * @param ColorRGBA Color a utilizar para limpiar (array de 4 floats).
 */
void DeviceContext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView,
	const float ColorRGBA[4]) {

	if (!m_deviceContext || !pRenderTargetView) {
		ERROR("DeviceContext", "ClearRenderTargetView", "Device context or render target view is null.");
		return;
	}

	m_deviceContext->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}
