/**
 * @file ShaderProgram.cpp
 * @brief Implementaci�n de la clase ShaderProgram para compilar, crear y gestionar shaders en Direct3D 11.
 *
 * @details
 * Esta clase permite:
 * - Compilar shaders desde archivos HLSL.
 * - Crear Vertex Shader, Pixel Shader y el Input Layout correspondiente.
 * - Configurar el pipeline gr�fico con los shaders compilados.
 * - Destruir y liberar los recursos asociados.
 *
 * @note [GameDev] CompileShaderFromFile usa D3DX11CompileFromFile con shader model
 * vs_4_0 / ps_4_0 (Feature Level 10.0). Para usar caracteristicas DX11 completas
 * (geometry shaders, compute shaders, stream output) se usaria vs_5_0 / ps_5_0.
 * El ID3DBlob del VS se guarda en m_vertexShaderData porque CreateInputLayout lo
 * necesita para validar los semantics contra el HLSL. Una vez creado el IL, el blob
 * puede liberarse (CreateInputLayout ya lo hizo al terminar).
 * En produccion se usaria D3DCompileFromFile (D3DCompiler.h moderno, no D3DX11) y
 * los resultados se cachean a disco para evitar recompilacion en cada arranque.
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
     * @param Layout Descripci�n de los elementos de entrada (Input Layout).
     * @return HRESULT S_OK si se inicializ� correctamente, o c�digo de error en caso contrario.
     */
    if (!device.m_device) {
        LOG_ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }
    if (fileName.empty()) {
        LOG_ERROR("ShaderProgram", "init", "File name is empty.");
        return E_INVALIDARG;
    }
    if (Layout.empty()) {
        LOG_ERROR("ShaderProgram", "init", "Input layout is empty.");
        return E_INVALIDARG;
    }
    m_shaderFileName = fileName;

    // Crear Vertex Shader
    HRESULT hr = CreateShader(device, ShaderType::VERTEX_SHADER);
    if (FAILED(hr)) {
        LOG_ERROR("ShaderProgram", "init", "Failed to create vertex shader.");
        return hr;
    }

    // Crear Input Layout
    hr = CreateInputLayout(device, Layout);
    if (FAILED(hr)) {
        LOG_ERROR("ShaderProgram", "init", "Failed to create input layout.");
        return hr;
    }

    // Crear Pixel Shader
    hr = CreateShader(device, ShaderType::PIXEL_SHADER);
    if (FAILED(hr)) {
        LOG_ERROR("ShaderProgram", "init", "Failed to create pixel shader.");
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
     * @param Layout Descripci�n de la estructura de v�rtices.
     * @return HRESULT S_OK si se cre� correctamente, o c�digo de error en caso contrario.
     */
    if (!m_vertexShaderData) {
        LOG_ERROR("ShaderProgram", "CreateInputLayout", "Vertex shader data is null.");
        return E_POINTER;
    }
    if (!device.m_device) {
        LOG_ERROR("ShaderProgram", "CreateInputLayout", "Device is null.");
        return E_POINTER;
    }
    if (Layout.empty()) {
        LOG_ERROR("ShaderProgram", "CreateInputLayout", "Input layout is empty.");
        return E_INVALIDARG;
    }

    HRESULT hr = m_inputLayout.init(device, Layout, m_vertexShaderData);
    SAFE_RELEASE(m_vertexShaderData);

    if (FAILED(hr)) {
        LOG_ERROR("ShaderProgram", "CreateInputLayout", "Failed to create input layout.");
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
     * @return HRESULT S_OK si se cre� correctamente, o c�digo de error en caso contrario.
     */
    if (!device.m_device) {
        LOG_ERROR("ShaderProgram", "CreateShader", "Device is null.");
        return E_POINTER;
    }
    if (m_shaderFileName.empty()) {
        LOG_ERROR("ShaderProgram", "CreateShader", "Shader file name is empty.");
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
        LOG_ERROR("ShaderProgram", "CreateShader",
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
        LOG_ERROR("ShaderProgram", "CreateShader",
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
     * @return HRESULT S_OK si se cre� correctamente, o c�digo de error.
     */
    if (!device.m_device) {
        LOG_ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }
    if (fileName.empty()) {
        LOG_ERROR("ShaderProgram", "init", "File name is empty.");
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
     * @param szEntryPoint Punto de entrada (funci�n principal) del shader.
     * @param szShaderModel Modelo de shader a usar (ej. vs_4_0, ps_4_0).
     * @param ppBlobOut Puntero para recibir el bytecode compilado.
     * @return HRESULT S_OK si se compila correctamente, o c�digo de error.
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
            LOG_ERROR("ShaderProgram", "CompileShaderFromFile",
                "Failed to compile shader from file: %s. Error: %s",
                szFileName, static_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        else {
            LOG_ERROR("ShaderProgram", "CompileShaderFromFile",
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
    /** @brief M�todo vac�o, reservado para futura l�gica de actualizaci�n de shaders. */
}

void
ShaderProgram::render(DeviceContext& deviceContext) {
    /**
     * @brief Activa el Vertex Shader, Pixel Shader e Input Layout en el pipeline.
     * @param deviceContext Contexto de dispositivo para emitir comandos de renderizado.
     */
    if (!m_VertexShader || !m_PixelShader || !m_inputLayout.m_inputLayout) {
        LOG_ERROR("ShaderProgram", "render", "Shaders or InputLayout not initialized");
        return;
    }

    m_inputLayout.render(deviceContext);
    deviceContext.m_deviceContext->VSSetShader(m_VertexShader, nullptr, 0);
    deviceContext.m_deviceContext->PSSetShader(m_PixelShader, nullptr, 0);
}

void
ShaderProgram::render(DeviceContext& deviceContext, ShaderType type) {
    /**
     * @brief Activa �nicamente un shader espec�fico.
     * @param deviceContext Contexto de dispositivo.
     * @param type Tipo de shader a activar.
     */
    if (!deviceContext.m_deviceContext) {
        LOG_ERROR("RenderTargetView", "render", "DeviceContext is nullptr.");
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
