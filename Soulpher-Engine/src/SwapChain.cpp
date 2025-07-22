/**
 * @file SwapChain.cpp
 * @brief Implementación de la clase SwapChain.
 *
 * Esta clase administra la cadena de intercambio (SwapChain) que permite
 * mostrar en pantalla los fotogramas renderizados por el motor gráfico.
 * Además, se encarga de configurar el multisampling (MSAA), crear el
 * dispositivo de render y extraer el back buffer para usarlo como render target.
 */

#include "SwapChain.h"
#include "Device.h"
#include "DeviceContext.h"
#include "Texture.h"
#include "Window.h"

 /**
  * @brief Inicializa el swap chain y los dispositivos necesarios para DirectX 11.
  *
  * Este método crea el `Device`, `DeviceContext` y la `SwapChain`, verificando si
  * hay soporte para MSAA. También obtiene el backBuffer para poder renderizar sobre él.
  *
  * @param device Referencia al objeto Device donde se almacenará el dispositivo creado.
  * @param deviceContext Referencia al contexto del dispositivo.
  * @param backBuffer Textura que representará el buffer de dibujo.
  * @param window Ventana sobre la que se dibujará (se necesita el HWND).
  * @return HRESULT indicando éxito o fallo.
  */
HRESULT SwapChain::init(Device& device,
    DeviceContext& deviceContext,
    Texture& backBuffer,
    Window window) {

    // Verificamos que la ventana esté inicializada correctamente.
    if (!window.m_hWnd) {
        ERROR("SwapChain", "init", "Invalid window handle. (m_hWnd is nullptr)");
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    unsigned int createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;  // Activamos modo debug si está definido.
#endif

    // Tipos de driver que probaremos (hardware, software, referencia).
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    unsigned int numDriverTypes = ARRAYSIZE(driverTypes);

    // Niveles de características que intentaremos usar (DX11, DX10.1, DX10).
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    unsigned int numFeatureLevels = ARRAYSIZE(featureLevels);

    // Intentamos crear el dispositivo probando diferentes tipos de driver.
    for (unsigned int i = 0; i < numDriverTypes; i++) {
        hr = D3D11CreateDevice(nullptr, driverTypes[i], nullptr, createDeviceFlags,
            featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
            &device.m_device, &m_featureLevel, &deviceContext.m_deviceContext);

        if (SUCCEEDED(hr)) {
            MESSAGE("SwapChain", "init", "Device created successfully.");
            break;
        }
    }

    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to create D3D11 device. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Verificamos el soporte de MSAA (4x).
    m_sampleCount = 4;
    hr = device.m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,
        m_sampleCount, &m_qualityLevels);

    if (FAILED(hr) || m_qualityLevels == 0) {
        ERROR("SwapChain", "init", ("MSAA not supported or invalid quality level. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Configuramos la descripción del SwapChain.
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
    sd.SampleDesc.Count = m_sampleCount;
    sd.SampleDesc.Quality = m_qualityLevels - 1;

    // Obtenemos la interfaz DXGI del dispositivo.
    hr = device.m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&m_dxgiDevice);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to query IDXGIDevice. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Obtenemos el adaptador y la fábrica para crear el SwapChain.
    hr = m_dxgiDevice->GetAdapter(&m_dxgiAdapter);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to get IDXGIAdapter. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    hr = m_dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&m_dxgiFactory));
    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to get IDXGIFactory. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Creamos el SwapChain con toda la configuración anterior.
    hr = m_dxgiFactory->CreateSwapChain(device.m_device, &sd, &m_swapChain);
    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to create swap chain. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Obtenemos el buffer de dibujo (backBuffer) como textura.
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        ERROR("SwapChain", "init", ("Failed to get back buffer. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Libera todos los recursos utilizados por el SwapChain.
 *
 * Incluye el swap chain y las interfaces DXGI utilizadas para crearlo.
 */
void SwapChain::destroy() {
    SAFE_RELEASE(m_swapChain);
    SAFE_RELEASE(m_dxgiDevice);
    SAFE_RELEASE(m_dxgiAdapter);
    SAFE_RELEASE(m_dxgiFactory);
}

/**
 * @brief Envía el buffer actual a la pantalla.
 *
 * Es el paso final del ciclo de renderizado. Si no se llama a `present()`,
 * no se verá nada en la ventana.
 */
void SwapChain::present() {
    if (m_swapChain) {
        HRESULT hr = m_swapChain->Present(0, 0);
        if (FAILED(hr)) {
            ERROR("SwapChain", "present", ("Failed to present swap chain. HRESULT: " + std::to_string(hr)).c_str());
        }
    }
    else {
        ERROR("SwapChain", "present", "Swap chain is not initialized.");
    }
}