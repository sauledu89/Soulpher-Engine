/**
 * @file ShaderProgram.cpp
 * @brief Implementación de la clase ShaderProgram para compilar, crear y gestionar shaders en Direct3D 11.
 *
 * @details
 * Esta clase permite:
 * - Compilar shaders desde archivos HLSL.
 * - Crear Vertex Shader, Pixel Shader y el Input Layout correspondiente.
 * - Configurar el pipeline gráfico con los shaders compilados.
 * - Destruir y liberar los recursos asociados.
 */

#include "ShaderProgram.h"
#include "Device.h"
#include "DeviceContext.h"

HRESULT
ShaderProgram::init(Device& device,
    const std::string& fileName,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout) {
    /**
     * @brief Inicializa el programa de shaders con un Vertex Shader, Pixel Shader y su Input Layout.
     * @param device Referencia al dispositivo Direct3D 11.
     * @param fileName Ruta del archivo HLSL.
     * @param Layout Descripción de los elementos de entrada (Input Layout).
     * @return HRESULT S_OK si se inicializó correctamente, o código de error en caso contrario.
     */
    if (!device.m_device) {
        ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }
    if (fileName.empty()) {
        ERROR("ShaderProgram", "init", "File name is empty.");
        return E_INVALIDARG;
    }
    if (Layout.empty()) {
        ERROR("ShaderProgram", "init", "Input layout is empty.");
        return E_INVALIDARG;
    }
    m_shaderFileName = fileName;

    // Crear Vertex Shader
    HRESULT hr = CreateShader(device, ShaderType::VERTEX_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create vertex shader.");
        return hr;
    }

    // Crear Input Layout
    hr = CreateInputLayout(device, Layout);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create input layout.");
        return hr;
    }

    // Crear Pixel Shader
    hr = CreateShader(device, ShaderType::PIXEL_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create pixel shader.");
        return hr;
    }

    return hr;
}

HRESULT
ShaderProgram::CreateInputLayout(Device& device,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout) {
    /**
     * @brief Crea el Input Layout usando los datos del Vertex Shader.
     * @param device Referencia al dispositivo Direct3D 11.
     * @param Layout Descripción de la estructura de vértices.
     * @return HRESULT S_OK si se creó correctamente, o código de error en caso contrario.
     */
    if (!m_vertexShaderData) {
        ERROR("ShaderProgram", "CreateInputLayout", "Vertex shader data is null.");
        return E_POINTER;
    }
    if (!device.m_device) {
        ERROR("ShaderProgram", "CreateInputLayout", "Device is null.");
        return E_POINTER;
    }
    if (Layout.empty()) {
        ERROR("ShaderProgram", "CreateInputLayout", "Input layout is empty.");
        return E_INVALIDARG;
    }

    HRESULT hr = m_inputLayout.init(device, Layout, m_vertexShaderData);
    SAFE_RELEASE(m_vertexShaderData);

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateInputLayout", "Failed to create input layout.");
        return hr;
    }

    return hr;
}

HRESULT
ShaderProgram::CreateShader(Device& device, ShaderType type) {
    /**
     * @brief Compila y crea un shader de tipo Vertex o Pixel.
     * @param device Referencia al dispositivo Direct3D 11.
     * @param type Tipo de shader (VERTEX_SHADER o PIXEL_SHADER).
     * @return HRESULT S_OK si se creó correctamente, o código de error en caso contrario.
     */
    if (!device.m_device) {
        ERROR("ShaderProgram", "CreateShader", "Device is null.");
        return E_POINTER;
    }
    if (m_shaderFileName.empty()) {
        ERROR("ShaderProgram", "CreateShader", "Shader file name is empty.");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    ID3DBlob* shaderData = nullptr;

    const char* shaderEntryPoint = (type == ShaderType::PIXEL_SHADER) ? "PS" : "VS";
    const char* shaderModel = (type == ShaderType::PIXEL_SHADER) ? "ps_4_0" : "vs_4_0";

    // Compilar shader desde archivo
    hr = CompileShaderFromFile(m_shaderFileName.data(),
        shaderEntryPoint,
        shaderModel,
        &shaderData);

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader",
            "Failed to compile shader from file: %s", m_shaderFileName.c_str());
        return hr;
    }

    // Crear objeto shader
    if (type == PIXEL_SHADER) {
        hr = device.CreatePixelShader(shaderData->GetBufferPointer(),
            shaderData->GetBufferSize(),
            nullptr,
            &m_PixelShader);
    }
    else {
        hr = device.CreateVertexShader(shaderData->GetBufferPointer(),
            shaderData->GetBufferSize(),
            nullptr,
            &m_VertexShader);
    }

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader",
            "Failed to create shader object from compiled data.");
        shaderData->Release();
        return hr;
    }

    // Guardar datos compilados
    if (type == PIXEL_SHADER) {
        SAFE_RELEASE(m_pixelShaderData);
        m_pixelShaderData = shaderData;
    }
    else {
        SAFE_RELEASE(m_vertexShaderData);
        m_vertexShaderData = shaderData;
    }

    return S_OK;
}

HRESULT
ShaderProgram::CreateShader(Device& device, ShaderType type, const std::string& fileName) {
    /**
     * @brief Crea un shader especificando un archivo HLSL diferente al inicial.
     * @param device Referencia al dispositivo Direct3D 11.
     * @param type Tipo de shader.
     * @param fileName Ruta del archivo HLSL.
     * @return HRESULT S_OK si se creó correctamente, o código de error.
     */
    if (!device.m_device) {
        ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }
    if (fileName.empty()) {
        ERROR("ShaderProgram", "init", "File name is empty.");
        return E_INVALIDARG;
    }
    m_shaderFileName = fileName;
    CreateShader(device, type);

    return S_OK;
}

HRESULT
ShaderProgram::CompileShaderFromFile(char* szFileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    ID3DBlob** ppBlobOut) {
    /**
     * @brief Compila un shader desde un archivo HLSL.
     * @param szFileName Ruta del archivo HLSL.
     * @param szEntryPoint Punto de entrada (función principal) del shader.
     * @param szShaderModel Modelo de shader a usar (ej. vs_4_0, ps_4_0).
     * @param ppBlobOut Puntero para recibir el bytecode compilado.
     * @return HRESULT S_OK si se compila correctamente, o código de error.
     */
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile(szFileName,
        nullptr,
        nullptr,
        szEntryPoint,
        szShaderModel,
        dwShaderFlags,
        0,
        nullptr,
        ppBlobOut,
        &pErrorBlob,
        nullptr);

    if (FAILED(hr)) {
        if (pErrorBlob) {
            ERROR("ShaderProgram", "CompileShaderFromFile",
                "Failed to compile shader from file: %s. Error: %s",
                szFileName, static_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        else {
            ERROR("ShaderProgram", "CompileShaderFromFile",
                "Failed to compile shader from file: %s. No error message available.",
                szFileName);
        }
        return hr;
    }

    SAFE_RELEASE(pErrorBlob)
        return S_OK;
}

void
ShaderProgram::update() {
    /** @brief Método vacío, reservado para futura lógica de actualización de shaders. */
}

void
ShaderProgram::render(DeviceContext& deviceContext) {
    /**
     * @brief Activa el Vertex Shader, Pixel Shader e Input Layout en el pipeline.
     * @param deviceContext Contexto de dispositivo para emitir comandos de renderizado.
     */
    if (!m_VertexShader || !m_PixelShader || !m_inputLayout.m_inputLayout) {
        ERROR("ShaderProgram", "render", "Shaders or InputLayout not initialized");
        return;
    }

    m_inputLayout.render(deviceContext);
    deviceContext.m_deviceContext->VSSetShader(m_VertexShader, nullptr, 0);
    deviceContext.m_deviceContext->PSSetShader(m_PixelShader, nullptr, 0);
}

void
ShaderProgram::render(DeviceContext& deviceContext, ShaderType type) {
    /**
     * @brief Activa únicamente un shader específico.
     * @param deviceContext Contexto de dispositivo.
     * @param type Tipo de shader a activar.
     */
    if (!deviceContext.m_deviceContext) {
        ERROR("RenderTargetView", "render", "DeviceContext is nullptr.");
        return;
    }
    switch (type) {
    case VERTEX_SHADER:
        deviceContext.m_deviceContext->VSSetShader(m_VertexShader, nullptr, 0);
        break;
    case PIXEL_SHADER:
        deviceContext.m_deviceContext->PSSetShader(m_PixelShader, nullptr, 0);
        break;
    default:
        break;
    }
}

void
ShaderProgram::destroy() {
    /** @brief Libera todos los recursos asociados al programa de shaders. */
    SAFE_RELEASE(m_VertexShader);
    m_inputLayout.destroy();
    SAFE_RELEASE(m_PixelShader);
    SAFE_RELEASE(m_vertexShaderData);
    SAFE_RELEASE(m_pixelShaderData);
}
