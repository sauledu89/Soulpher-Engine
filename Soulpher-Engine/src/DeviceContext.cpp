#include "DeviceContext.h"

void 
DeviceContext::RSSetViewports(unsigned int NumViewports, 
															const D3D11_VIEWPORT* pViewports) {
	if (NumViewports > 0 && pViewports != nullptr) {
		ERROR("DeviceContext", "RSSetViewports", "pViewports is nullptr");
	}
	// Set the Viewport
	m_deviceContext->RSSetViewports(NumViewports, pViewports);
}

void 
DeviceContext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, 
																		 unsigned int ClearFlags, 
																		 float Depth, 
																		 UINT8 Stencil) {
}

void 
DeviceContext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, 
																		 const float ColorRGBA[4]) {

}
