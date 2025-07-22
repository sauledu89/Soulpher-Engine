#include "ShaderProgram.h"
#include "Device.h"
#include "DeviceContext.h"
#include "ShaderType.h" // Asegúrate de que este archivo contenga el enum ShaderType

HRESULT ShaderProgram::init(Device& device,
    const std::string& fileName,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout)
{
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

    HRESULT hr = CreateShader(device, ShaderType::VERTEX_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create vertex shader.");
        return hr;
    }

    hr = CreateInputLayout(device, Layout);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create input layout.");
        return hr;
    }

    hr = CreateShader(device, ShaderType::PIXEL_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Failed to create pixel shader.");
        return hr;
    }

    return hr;
}

HRESULT ShaderProgram::CreateInputLayout(Device& device,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout)
{
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

HRESULT ShaderProgram::CreateShader(Device& device, ShaderType type)
{
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

    hr = CompileShaderFromFile(m_shaderFileName.c_str(), shaderEntryPoint, shaderModel, &shaderData);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader",
            "Failed to compile shader from file: %s", m_shaderFileName.c_str());
        return hr;
    }

    if (type == ShaderType::PIXEL_SHADER) {
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

    if (type == ShaderType::PIXEL_SHADER) {
        SAFE_RELEASE(m_pixelShaderData);
        m_pixelShaderData = shaderData;
    }
    else {
        SAFE_RELEASE(m_vertexShaderData);
        m_vertexShaderData = shaderData;
    }

    return S_OK;
}

HRESULT ShaderProgram::CreateShader(Device& device, ShaderType type, const std::string& fileName)
{
    if (!device.m_device) {
        ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }
    if (fileName.empty()) {
        ERROR("ShaderProgram", "init", "File name is empty.");
        return E_INVALIDARG;
    }

    m_shaderFileName = fileName;
    return CreateShader(device, type);
}

HRESULT ShaderProgram::CompileShaderFromFile(const char* szFileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(DEBUG) || defined(_DEBUG)
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;
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

    SAFE_RELEASE(pErrorBlob);
    return S_OK;
}

void ShaderProgram::update() {}

void ShaderProgram::render(DeviceContext& deviceContext)
{
    if (!m_VertexShader || !m_PixelShader || !m_inputLayout.m_inputLayout) {
        ERROR("ShaderProgram", "render", "Shaders or InputLayout not initialized");
        return;
    }

    m_inputLayout.render(deviceContext);
    deviceContext.VSSetShader(m_VertexShader, nullptr, 0);
    deviceContext.PSSetShader(m_PixelShader, nullptr, 0);
}

void ShaderProgram::render(DeviceContext& deviceContext, ShaderType type)
{
    switch (type) {
    case ShaderType::VERTEX_SHADER:
        deviceContext.VSSetShader(m_VertexShader, nullptr, 0);
        break;
    case ShaderType::PIXEL_SHADER:
        deviceContext.PSSetShader(m_PixelShader, nullptr, 0);
        break;
    default:
        break;
    }
}

void ShaderProgram::destroy()
{
    SAFE_RELEASE(m_VertexShader);
    m_inputLayout.destroy();
    SAFE_RELEASE(m_PixelShader);
    SAFE_RELEASE(m_vertexShaderData);
    SAFE_RELEASE(m_pixelShaderData);
}
