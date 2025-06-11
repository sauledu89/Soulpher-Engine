#include "Prerequisites.h"
#include "Window.h"
#include "Device.h"
#include "SwapChain.h"
#include "DeviceContext.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"
#include "Viewport.h"

// Customs
Window g_window;
Device g_device;
SwapChain g_swapChain;
DeviceContext g_deviceContext;
Texture g_backBuffer;
RenderTargetView g_renderTargetView;
Texture g_depthStencil;
DepthStencilView g_depthStencilView;
Viewport viewport;

//--------------------------------------------------------------------------------------
// Variables Globales
//--------------------------------------------------------------------------------------
//HINSTANCE                           g_hInst = NULL;
//HWND                                g_hWnd = NULL;
//D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
//D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
//ID3D11Device*                       g_device.m_device = NULL;
//ID3D11DeviceContext*                g_deviceContext.m_deviceContext = NULL;
//IDXGISwapChain*                     g_pSwapChain = NULL;
//ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
//ID3D11Texture2D*                    g_pDepthStencil = NULL;
//ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader* g_pVertexShader = NULL;
ID3D11PixelShader* g_pPixelShader = NULL;
ID3D11InputLayout* g_pVertexLayout = NULL;
ID3D11Buffer* g_pVertexBuffer = NULL;
ID3D11Buffer* g_pIndexBuffer = NULL;
ID3D11Buffer* g_pCBNeverChanges = NULL;
ID3D11Buffer* g_pCBChangeOnResize = NULL;
ID3D11Buffer* g_pCBChangesEveryFrame = NULL;

// Variable global para el constant buffer de la luz puntual
ID3D11Buffer* g_pCBPointLight = NULL;
ID3D11ShaderResourceView* g_pTextureRV = NULL;
ID3D11SamplerState* g_pSamplerLinear = NULL;
XMMATRIX                            g_World;         // Para el cubo
XMMATRIX                            g_PlaneWorld;    // Para el plano
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);

//----- Variables agregadas para el plano y sombras -----//
ID3D11Buffer* g_pPlaneVertexBuffer = NULL;
ID3D11Buffer* g_pPlaneIndexBuffer = NULL;
UINT                                g_planeIndexCount = 0;
ID3D11PixelShader* g_pShadowPixelShader = NULL;
ID3D11BlendState* g_pShadowBlendState = NULL;
ID3D11DepthStencilState* g_pShadowDepthStencilState = NULL;
XMFLOAT4                            g_LightPos(2.0f, 4.0f, -2.0f, 1.0f); // Posición de la luz

//--------------------------------------------------------------------------------------
// Declaraciones adelantadas
//--------------------------------------------------------------------------------------
//HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateScene();
void RenderScene();

//--------------------------------------------------------------------------------------
// Punto de entrada del programa. Inicializa todo y entra en el bucle de mensajes.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(g_window.init(hInstance, nCmdShow, WndProc)))
        return 0;

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // Bucle principal de mensajes
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Actualiza la lógica de la escena
            UpdateScene();
            // Renderiza la escena
            RenderScene();
        }
    }

    CleanupDevice();

    return (int)msg.wParam;
}

//--------------------------------------------------------------------------------------
// Registro de la clase y creación de la ventana
//--------------------------------------------------------------------------------------
//HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
//{
//  // Registro de la clase
//  WNDCLASSEX wcex;
//  wcex.cbSize = sizeof(WNDCLASSEX);
//  wcex.style = CS_HREDRAW | CS_VREDRAW;
//  wcex.lpfnWndProc = WndProc;
//  wcex.cbClsExtra = 0;
//  wcex.cbWndExtra = 0;
//  wcex.hInstance = hInstance;
//  wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
//  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
//  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//  wcex.lpszMenuName = NULL;
//  wcex.lpszClassName = "TutorialWindowClass";
//  wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
//  if (!RegisterClassEx(&wcex))
//    return E_FAIL;
//
//  // Creación de la ventana
//  g_hInst = hInstance;
//  RECT rc = { 0, 0, 640, 480 };
//  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
//  g_hWnd = CreateWindow("TutorialWindowClass", "Direct3D 11 Tutorial 7 - Sombras Planas", WS_OVERLAPPEDWINDOW,
//    CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
//    NULL);
//  if (!g_hWnd)
//    return E_FAIL;
//
//  ShowWindow(g_hWnd, nCmdShow);
//
//  return S_OK;
//}

//--------------------------------------------------------------------------------------
// Función auxiliar para compilar shaders con D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(char* szFileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
    if (FAILED(hr))
    {
        if (pErrorBlob != NULL)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        if (pErrorBlob) pErrorBlob->Release();
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Creación del dispositivo Direct3D y swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    hr = g_swapChain.init(g_device, g_deviceContext, g_backBuffer, g_window);

    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize SwpaChian. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    hr = g_renderTargetView.init(g_device, g_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);

    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize RenderTargetView. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Crear textura de depth stencil.
    hr = g_depthStencil.init(g_device,
        g_window.m_width,
        g_window.m_height,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_BIND_DEPTH_STENCIL,
        4,
        0);

    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize DepthStencil. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Crear el depth stencil view
    hr = g_depthStencilView.init(g_device,
        g_depthStencil,
        DXGI_FORMAT_D24_UNORM_S8_UINT);

    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize DepthStencilView. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Crear el viewport
    hr = viewport.init(g_window);

    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize Viewport. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Compilar y crear el vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile("Soulpher-Engine.fx", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr))
    {
        MessageBox(NULL,
            "El archivo FX no se pudo compilar. Ejecuta el ejecutable desde el directorio que contiene el archivo FX.", "Error", MB_OK);
        return hr;
    }

    hr = g_device.CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Definir el layout de entrada
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    hr = g_device.CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    g_deviceContext.m_deviceContext->IASetInputLayout(g_pVertexLayout);

    // Compilar y crear el pixel shader (normal)
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile("Soulpher-Engine.fx", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr))
    {
        MessageBox(NULL,
            "El archivo FX no se pudo compilar. Ejecuta el ejecutable desde el directorio que contiene el archivo FX.", "Error", MB_OK);
        return hr;
    }

    hr = g_device.CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Crear vertex buffer y index buffer para el cubo
    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = g_device.CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    // Configurar el vertex buffer para el cubo
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_deviceContext.m_deviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Crear index buffer para el cubo
    WORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_device.CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
    if (FAILED(hr))
        return hr;

    g_deviceContext.m_deviceContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Establecer topología primitiva
    g_deviceContext.m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Crear los constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_device.CreateBuffer(&bd, NULL, &g_pCBNeverChanges);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_device.CreateBuffer(&bd, NULL, &g_pCBChangeOnResize);
    if (FAILED(hr))
        return hr;

    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_device.CreateBuffer(&bd, NULL, &g_pCBChangesEveryFrame);
    if (FAILED(hr))
        return hr;

    // Cargar la textura
    hr = D3DX11CreateShaderResourceViewFromFile(g_device.m_device, "seafloor.dds", NULL, NULL, &g_pTextureRV, NULL);
    if (FAILED(hr))
        return hr;

    // Crear el sampler state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_device.CreateSamplerState(&sampDesc, &g_pSamplerLinear);
    if (FAILED(hr))
        return hr;

    // Inicializar las matrices de mundo, vista y proyección
    // --- Para el cubo, se añade una traslación hacia arriba (2 unidades) antes de la rotación ---
    // Nota: Esto hará que el cubo se ubique por encima del plano.
    XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -6.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = XMMatrixLookAtLH(Eye, At, Up);

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose(g_View);
    g_deviceContext.m_deviceContext->UpdateSubresource(g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);

    g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, g_window.m_width / (FLOAT)g_window.m_height, 0.01f, 100.0f);

    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);
    g_deviceContext.m_deviceContext->UpdateSubresource(g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0);

    //------- CREACIÓN DE GEOMETRÍA DEL PLANO (suelo) -------//
    // Se amplían las dimensiones del plano para que sea más visible.
    SimpleVertex planeVertices[] =
    {
        { XMFLOAT3(-20.0f, 0.0f, -20.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(20.0f, 0.0f, -20.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(20.0f, 0.0f,  20.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-20.0f, 0.0f,  20.0f), XMFLOAT2(0.0f, 1.0f) },
    };

    WORD planeIndices[] =
    {
        0, 2, 1,
        0, 3, 2
    };

    g_planeIndexCount = 6;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = planeVertices;
    hr = g_device.CreateBuffer(&bd, &InitData, &g_pPlaneVertexBuffer);
    if (FAILED(hr))
        return hr;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * g_planeIndexCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = planeIndices;
    hr = g_device.CreateBuffer(&bd, &InitData, &g_pPlaneIndexBuffer);
    if (FAILED(hr))
        return hr;

    //------- COMPILAR SHADER DE SOMBRA -------//
    ID3DBlob* pShadowPSBlob = NULL;
    hr = CompileShaderFromFile("Soulpher-Engine.fx", "ShadowPS", "ps_4_0", &pShadowPSBlob);
    if (FAILED(hr))
    {
        MessageBox(NULL,
            "Error al compilar el ShadowPS.", "Error", MB_OK);
        return hr;
    }
    hr = g_device.CreatePixelShader(pShadowPSBlob->GetBufferPointer(), pShadowPSBlob->GetBufferSize(), NULL, &g_pShadowPixelShader);
    pShadowPSBlob->Release();
    if (FAILED(hr))
        return hr;

    //------- CREAR ESTADOS DE BLENDING Y DEPTH STENCIL PARA LAS SOMBRAS -------//
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = g_device.CreateBlendState(&blendDesc, &g_pShadowBlendState);
    if (FAILED(hr))
        return hr;

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    ZeroMemory(&dsDesc, sizeof(dsDesc));
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Deshabilitar escritura en depth
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    hr = g_device.CreateDepthStencilState(&dsDesc, &g_pShadowDepthStencilState);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Liberar los objetos creados
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if (g_deviceContext.m_deviceContext) g_deviceContext.m_deviceContext->ClearState();

    if (g_pShadowBlendState) g_pShadowBlendState->Release();
    if (g_pShadowDepthStencilState) g_pShadowDepthStencilState->Release();
    if (g_pShadowPixelShader) g_pShadowPixelShader->Release();
    if (g_pPlaneVertexBuffer) g_pPlaneVertexBuffer->Release();
    if (g_pPlaneIndexBuffer) g_pPlaneIndexBuffer->Release();
    if (g_pSamplerLinear) g_pSamplerLinear->Release();
    if (g_pTextureRV) g_pTextureRV->Release();
    if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
    if (g_pCBChangeOnResize) g_pCBChangeOnResize->Release();
    if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    //if (g_pDepthStencil) g_pDepthStencil->Release();
    g_depthStencil.destroy();
    //if (g_pDepthStencilView) g_pDepthStencilView->Release();
    g_depthStencilView.destroy();
    g_renderTargetView.destroy();
    //if (g_pRenderTargetView) g_pRenderTargetView->Release();
    //g_backBuffer.destroy();
    g_swapChain.destroy();
    //if (g_pSwapChain) g_pSwapChain->Release();
    if (g_deviceContext.m_deviceContext) g_deviceContext.m_deviceContext->Release();
    if (g_device.m_device) g_device.m_device->Release();
}

//--------------------------------------------------------------------------------------
// Función de mensaje de la ventana
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Renderizar un frame
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Función UpdateScene: actualiza transformaciones, tiempo y demás variables dinámicas.
//--------------------------------------------------------------------------------------
void UpdateScene()
{
    // Actualizar tiempo (mismo que antes)
    static float t = 0.0f;
    if (g_swapChain.m_driverType == D3D_DRIVER_TYPE_REFERENCE)
    {
        t += (float)XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;
        t = (dwTimeCur - dwTimeStart) / 1000.0f;
    }

    // --- Transformación del cubo ---
    // Parámetros del cubo:
    float cubePosX = 0.0f, cubePosY = 2.0f, cubePosZ = 0.0f;  // Ubicado 2 unidades arriba
    float cubeScale = 1.0f;                                    // Escala uniforme
    float cubeAngleX = 0.0f, cubeAngleY = t, cubeAngleZ = 0.0f;  // Rotación dinámica en Y

    // Crear las matrices individuales
    XMMATRIX cubeScaleMat = XMMatrixScaling(cubeScale, cubeScale, cubeScale);
    XMMATRIX cubeRotMat = XMMatrixRotationX(cubeAngleX) *
        XMMatrixRotationY(cubeAngleY) *
        XMMatrixRotationZ(cubeAngleZ);
    XMMATRIX cubeTransMat = XMMatrixTranslation(cubePosX, cubePosY, cubePosZ);

    // Combinar: primero escala, luego rota y por último traslada
    g_World = cubeTransMat * cubeRotMat * cubeScaleMat;

    // Actualizar el color animado del cubo
    g_vMeshColor.x = (sinf(t * 1.0f) + 1.0f) * 0.5f;
    g_vMeshColor.y = (cosf(t * 3.0f) + 1.0f) * 0.5f;
    g_vMeshColor.z = (sinf(t * 5.0f) + 1.0f) * 0.5f;

    // --- Transformación del plano ---
    // Parámetros para el plano:
    float planePosX = 0.0f, planePosY = -5.0f, planePosZ = 0.0f;
    // Aunque los vértices ya definen un plano extenso (-20 a 20), aquí puedes ajustar el escalado adicional
    float planeScaleFactor = 1.0f; // Puedes modificar este factor para agrandar o reducir el plano
    float planeAngleX = 0.0f, planeAngleY = 0.0f, planeAngleZ = 0.0f; // Sin rotación por defecto

    XMMATRIX planeScaleMat = XMMatrixScaling(planeScaleFactor, planeScaleFactor, planeScaleFactor);
    XMMATRIX planeRotMat = XMMatrixRotationX(planeAngleX) *
        XMMatrixRotationY(planeAngleY) *
        XMMatrixRotationZ(planeAngleZ);
    XMMATRIX planeTransMat = XMMatrixTranslation(planePosX, planePosY, planePosZ);

    // Combinar transformaciones para el plano
    g_PlaneWorld = planeTransMat * planeRotMat * planeScaleMat;
}

//--------------------------------------------------------------------------------------
// Función RenderScene: limpia buffers y dibuja la escena (plano, cubo y sombra).
//--------------------------------------------------------------------------------------
void RenderScene()
{
    // Limpiar el back buffer y el depth buffer
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
    g_renderTargetView.render(g_deviceContext, g_depthStencilView, 1, ClearColor);
    g_depthStencilView.render(g_deviceContext);

    // Set Viewport
    viewport.render(g_deviceContext);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;

    //------------- Renderizar el plano (suelo) -------------//
    g_deviceContext.m_deviceContext->IASetVertexBuffers(0, 1, &g_pPlaneVertexBuffer, &stride, &offset);
    g_deviceContext.m_deviceContext->IASetIndexBuffer(g_pPlaneIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    XMMATRIX planeWorld = XMMatrixIdentity();
    // Renderizar el plano (suelo)
    // Renderizar el plano (suelo)
    CBChangesEveryFrame cbPlane;
    cbPlane.mWorld = XMMatrixTranspose(g_PlaneWorld);
    cbPlane.vMeshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    g_deviceContext.m_deviceContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cbPlane, 0, 0);
    // ... Continuar con el renderizado (setear shaders, constantes, etc.)

    g_deviceContext.m_deviceContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_deviceContext.m_deviceContext->PSSetShader(g_pPixelShader, NULL, 0);
    g_deviceContext.m_deviceContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_deviceContext.m_deviceContext->PSSetShaderResources(0, 1, &g_pTextureRV);
    g_deviceContext.m_deviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_deviceContext.m_deviceContext->DrawIndexed(g_planeIndexCount, 0, 0);

    //------------- Renderizar el cubo (normal) -------------//
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose(g_World);
    cb.vMeshColor = g_vMeshColor;
    g_deviceContext.m_deviceContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0);
    g_deviceContext.m_deviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_deviceContext.m_deviceContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_deviceContext.m_deviceContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
    g_deviceContext.m_deviceContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_deviceContext.m_deviceContext->PSSetShader(g_pPixelShader, NULL, 0);
    g_deviceContext.m_deviceContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
    g_deviceContext.m_deviceContext->PSSetShaderResources(0, 1, &g_pTextureRV);
    g_deviceContext.m_deviceContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_deviceContext.m_deviceContext->DrawIndexed(36, 0, 0);

    //------------- Renderizar la sombra del cubo -------------//
    float dot = g_LightPos.y;
    XMMATRIX shadowMatrix = XMMATRIX(
        dot, -g_LightPos.x, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -g_LightPos.z, dot, 0.0f,
        0.0f, -1.0f, 0.0f, dot
    );
    XMMATRIX shadowWorld = g_World * shadowMatrix;
    CBChangesEveryFrame cbShadow;
    cbShadow.mWorld = XMMatrixTranspose(shadowWorld);
    cbShadow.vMeshColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
    g_deviceContext.m_deviceContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &cbShadow, 0, 0);
    g_deviceContext.m_deviceContext->PSSetShader(g_pShadowPixelShader, NULL, 0);
    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    g_deviceContext.m_deviceContext->OMSetBlendState(g_pShadowBlendState, blendFactor, 0xffffffff);
    g_deviceContext.m_deviceContext->OMSetDepthStencilState(g_pShadowDepthStencilState, 0);
    g_deviceContext.m_deviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_deviceContext.m_deviceContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_deviceContext.m_deviceContext->DrawIndexed(36, 0, 0);
    g_deviceContext.m_deviceContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);
    g_deviceContext.m_deviceContext->OMSetDepthStencilState(NULL, 0);
    g_deviceContext.m_deviceContext->PSSetShader(g_pPixelShader, NULL, 0);

    // Presentar el back buffer al front buffer
    g_swapChain.present();
}