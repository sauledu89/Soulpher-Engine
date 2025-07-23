#pragma once
#include "Prerequisites.h"
#include "InputLayout.h"

class Device;
class DeviceContext;

/**
 * @class ShaderProgram
 * @brief Gestiona los shaders de vértices y píxeles.
 *
 * Esta clase es responsable de cargar, compilar e inicializar los programas de sombreado
 * (vertex y pixel shaders) que se utilizarán en el pipeline de renderizado.
 */
class
    ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram() = default;

    HRESULT
        init(Device& device,
            const std::string& fileName,
            std::vector<D3D11_INPUT_ELEMENT_DESC> Layout);

    void
        update();

    void
        render(DeviceContext& deviceContext);

    void
        render(DeviceContext& deviceContext, ShaderType type);

    void
        destroy();

    HRESULT
        CreateInputLayout(Device& device, std::vector<D3D11_INPUT_ELEMENT_DESC> Layout);

    HRESULT
        CreateShader(Device& device, ShaderType type);

    HRESULT
        CreateShader(Device& device, ShaderType type, const std::string& fileName);

    HRESULT
        CompileShaderFromFile(char* szFileName,
            LPCSTR szEntryPoint,
            LPCSTR szShaderModel,
            ID3DBlob** ppBlobOut);

public:
    ID3D11VertexShader* m_VertexShader = nullptr;
    ID3D11PixelShader* m_PixelShader = nullptr;
    InputLayout m_inputLayout;
private:
    std::string m_shaderFileName;
    ID3DBlob* m_vertexShaderData = nullptr;
    ID3DBlob* m_pixelShaderData = nullptr;
};