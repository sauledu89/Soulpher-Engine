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

void DeviceContext::IASetInputLayout(ID3D11InputLayout* pInputLayout) {
	if (!m_deviceContext || !pInputLayout) {
		ERROR("DeviceContext", "IASetInputLayout", "Null input layout or context.");
		return;
	}
	m_deviceContext->IASetInputLayout(pInputLayout);
}

void DeviceContext::VSSetShader(ID3D11VertexShader* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
	if (!m_deviceContext || !pShader) {
		ERROR("DeviceContext", "VSSetShader", "Null vertex shader or context.");
		return;
	}
	m_deviceContext->VSSetShader(pShader, ppClassInstances, NumClassInstances);
}

void DeviceContext::PSSetShader(ID3D11PixelShader* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
	if (!m_deviceContext || !pShader) {
		ERROR("DeviceContext", "PSSetShader", "Null pixel shader or context.");
		return;
	}
	m_deviceContext->PSSetShader(pShader, ppClassInstances, NumClassInstances);
}

void DeviceContext::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask) {
	if (!m_deviceContext) {
		ERROR("DeviceContext", "OMSetBlendState", "Null device context.");
		return;
	}
	m_deviceContext->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}

void DeviceContext::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef) {
	if (!m_deviceContext) {
		ERROR("DeviceContext", "OMSetDepthStencilState", "Null device context.");
		return;
	}
	m_deviceContext->OMSetDepthStencilState(pDepthStencilState, StencilRef);
}

void DeviceContext::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
	if (!m_deviceContext) {
		ERROR("DeviceContext", "VSSetConstantBuffers", "Null device context.");
		return;
	}
	m_deviceContext->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

void DeviceContext::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
	if (!m_deviceContext) {
		ERROR("DeviceContext", "PSSetConstantBuffers", "Null device context.");
		return;
	}
	m_deviceContext->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

void DeviceContext::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox,
	const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) {
	if (!m_deviceContext || !pDstResource || !pSrcData) {
		ERROR("DeviceContext", "UpdateSubresource", "Null argument.");
		return;
	}
	m_deviceContext->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

void DeviceContext::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset) {
	if (!m_deviceContext || !pIndexBuffer) {
		ERROR("DeviceContext", "IASetIndexBuffer", "Null device context or index buffer.");
		return;
	}
	m_deviceContext->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}

void DeviceContext::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets) {
	if (!m_deviceContext || !ppVertexBuffers) {
		ERROR("DeviceContext", "IASetVertexBuffers", "Null device context or vertex buffer.");
		return;
	}
	m_deviceContext->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}
