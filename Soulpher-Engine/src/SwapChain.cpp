/**
 * @file SwapChain.cpp
 * @brief Creación y gestión de la swap chain, el dispositivo Direct3D y el back buffer.
 *
 * @details
 * Esta implementación inicializa el dispositivo de renderizado (`ID3D11Device`), el contexto (`ID3D11DeviceContext`)
 * y la cadena de intercambio (`IDXGISwapChain`). También se encarga de extraer y almacenar el back buffer
 * como textura para su posterior uso en el pipeline de renderizado.
 */

#include "SwapChain.h"
#include "Device.h"
#include "DeviceContext.h"
#include "Texture.h"
#include "Window.h"

 /**
  * @brief Inicializa el dispositivo, contexto y swap chain.
  *
  * @param device Referencia al objeto Device que contendrá el dispositivo Direct3D creado.
  * @param deviceContext Referencia al objeto DeviceContext que contendrá el contexto de dispositivo.
  * @param backBuffer Referencia a la textura donde se almacenará el back buffer.
  * @param window Información y manejador de la ventana.
  * @return HRESULT Código de éxito o error (E_POINTER, E_INVALIDARG, etc.).
  *
  * @details
  * - Crea el dispositivo Direct3D y su contexto.
  * - Configura la descripción de la swap chain.
  * - Obtiene el `IDXGIFactory` y crea la swap chain asociada a la ventana.
  * - Extrae el back buffer como `ID3D11Texture2D` y lo asigna al `Texture` recibido.
  */
HRESULT
SwapChain::init(Device& device,
    DeviceContext& deviceContext,
    Texture& backBuffer,
    Window window)
{
    if (!window.m_hWnd) {
        ERROR("SwapChain", "init", "Invalid window handle. (m_hWnd is nullptr)");
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    // Flags de creación del dispositivo
    unsigned int createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Tipos de driver a probar
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    const unsigned int numDriverTypes = ARRAYSIZE(driverTypes);

    // Niveles de características a soportar
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    const unsigned int numFeatureLevels = ARRAYSIZE(featureLevels);

    // Intentar crear el dispositivo con cada tipo de driver
    bool created = false;
    for (unsigned int i = 0; i < numDriverTypes; ++i) {
        m_driverType = driverTypes[i];
        hr = D3D11CreateDevice(
            nullptr,
            m_driverType,
            nullptr,
            createDeviceFlags,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &device.m_device,
            &m_featureLevel,
            &deviceContext.m_deviceContext);

        if (SUCCEEDED(hr)) {
            MESSAGE("SwapChain", "init", "Device created successfully.");
            created = true;
            break;
        }
    }
    if (!created) {
        ERROR("SwapChain", "init",
            ("Failed to create D3D11 device. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Sin MSAA para depuración
    m_sampleCount = 1;
    m_qualityLevels = 0;

    // Descripción de la swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = window.m_width;
    sd.BufferDesc.Height = window.m_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window.m_hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    // Obtener la factory de DXGI
    hr = device.m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&m_dxgiDevice);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init",
            ("Failed to query IDXGIDevice. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    hr = m_dxgiDevice->GetAdapter(&m_dxgiAdapter);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init",
            ("Failed to get IDXGIAdapter. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    hr = m_dxgiAdapter->GetParent(__uuidof(IDXGIFactory),
        reinterpret_cast<void**>(&m_dxgiFactory));
    if (FAILED(hr)) {
        ERROR("SwapChain", "init",
            ("Failed to get IDXGIFactory. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Crear la swap chain
    hr = m_dxgiFactory->CreateSwapChain(device.m_device, &sd, &m_swapChain);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init",
            ("Failed to create swap chain. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Obtener el back buffer
    ID3D11Texture2D* bb = nullptr;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init",
            ("Failed to get back buffer. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Reemplazar si ya había una textura previa
    if (backBuffer.m_texture) {
        backBuffer.m_texture->Release();
        backBuffer.m_texture = nullptr;
    }
    backBuffer.m_texture = bb; // se liberará en destroy() de Texture

    return S_OK;
}

/**
 * @brief Libera todos los recursos asociados a la swap chain y objetos DXGI.
 */
void
SwapChain::destroy() {
    if (m_swapChain) { SAFE_RELEASE(m_swapChain); }
    if (m_dxgiDevice) { SAFE_RELEASE(m_dxgiDevice); }
    if (m_dxgiAdapter) { SAFE_RELEASE(m_dxgiAdapter); }
    if (m_dxgiFactory) { SAFE_RELEASE(m_dxgiFactory); }
}

/**
 * @brief Presenta el back buffer en pantalla.
 *
 * @details Llama a `IDXGISwapChain::Present` para mostrar la imagen renderizada.
 * En caso de error, registra el código HRESULT.
 */
void
SwapChain::present() {
    if (m_swapChain) {
        HRESULT hr = m_swapChain->Present(0, 0);
        if (FAILED(hr)) {
            ERROR("SwapChain", "present",
                ("Failed to present swap chain. HRESULT: " + std::to_string(hr)).c_str());
        }
    }
    else {
        ERROR("SwapChain", "present", "Swap chain is not initialized.");
    }
}
