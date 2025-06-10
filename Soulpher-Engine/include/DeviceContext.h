#pragma once
#include "Prerequisites.h"

class 
DeviceContext {
public:
	DeviceContext()  = default;
	~DeviceContext() = default;

	void
	init();
	
	void 
	update();
	
	void 
	render();
	
	void 
	destroy();

	void 
	RSSetViewports(unsigned int NumViewports, const D3D11_VIEWPORT* pViewports);

	void 
	ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
												unsigned int ClearFlags,
												float Depth,
												UINT8 Stencil);

	void 
	ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, 
												const float ColorRGBA[4]);

	void 
	OMSetRenderTargets(unsigned int NumViews,
										 ID3D11RenderTargetView* const* ppRenderTargetViews,
										 ID3D11DepthStencilView* pDepthStencilView);
public:
	ID3D11DeviceContext* m_deviceContext = nullptr;
};