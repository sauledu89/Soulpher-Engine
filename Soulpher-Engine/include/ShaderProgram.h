/**
 * @file ShaderProgram.h
 * @brief Encapsula un programa de shaders HLSL (Vertex + Pixel Shader) y su Input Layout.
 *
 * @details
 * ShaderProgram agrupa los dos shaders minimos para un draw call en D3D11:
 *  - Vertex Shader (VS): transforma vertices de espacio objeto a clip space (MVP).
 *  - Pixel Shader (PS): calcula el color final de cada fragmento (Lambert, Blinn-Phong, PBR).
 *  - Input Layout: descriptor que mapea los campos de SimpleVertex a los semantics HLSL
 *    (POSITION, NORMAL, TANGENT, BINORMAL, TEXCOORD).
 *
 * La compilacion HLSL ocurre en tiempo de ejecucion via D3DCompileFromFile. En produccion
 * se pre-compilarian los shaders (.cso) para evitar el costo al inicio y detectar errores
 * HLSL en build time, no en runtime.
 *
 * @note [GameDev] En Unreal Engine los shaders se compilan off-line a multiples targets
 * (DX11, DX12, Vulkan, Metal) y se cachean en disco (el "Shader Cook"). Puede generar
 * gigabytes de shaders compilados. Los tiempos de compilacion de shaders son uno de los
 * principales bottlenecks de iteration en juegos AAA con miles de variantes de materiales.
 * Unity llama a este proceso "Shader Variant Compilation" y tambien cachea los resultados.
 *
 * @see ForwardRenderer, InputLayout, Device, Soulpher-Engine.fx
 */

#pragma once
#include "Prerequisites.h"
#include "InputLayout.h"
#include "EngineUtilities/Utilities/LayoutBuilder.h"

class Device;
class DeviceContext;

/**
 * @class ShaderProgram
 * @brief Encapsula un programa de shaders (Vertex y Pixel) y su Input Layout.
 *
 * @details
 * Un **ShaderProgram** combina la lógica gráfica programada en HLSL
 * con la configuración del **Input Layout** que define cómo llegan
 * los datos de los vértices a la GPU.
 *
 * 🔹 **Conceptos clave para estudiantes**:
 * - **Vertex Shader (VS)**: Procesa cada vértice y lo transforma de espacio local a pantalla.
 * - **Pixel Shader (PS)**: Calcula el color final de cada píxel.
 * - **Input Layout**: Define la estructura de los datos de los vértices (posición, UV, normales, etc.).
 * - Este sistema carga, compila y enlaza ambos shaders al **pipeline gráfico** de Direct3D 11.
 *
 * @note ⚡ Parte del pipeline gráfico de Soulpher-Engine.
 */
class ShaderProgram {
public:
    ShaderProgram() = default;  ///< Constructor por defecto.
    ~ShaderProgram() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa el programa de shaders a partir de un archivo y un layout de entrada.
     * @param device Dispositivo Direct3D para la creación de recursos.
     * @param fileName Nombre del archivo HLSL que contiene los shaders.
     * @param Layout Descriptores del formato de vértices (Input Layout).
     * @return HRESULT con el estado de la operación.
     *
     * @note
     * Este método compila y crea el Vertex y Pixel Shader, además del Input Layout.
     */
    HRESULT init(Device& device,
        const std::string& fileName,
        std::vector<D3D11_INPUT_ELEMENT_DESC> Layout);

    /** @brief Overload que acepta un LayoutBuilder fluent (delega al overload con vector). */
    HRESULT init(Device& device, const std::string& fileName, const LayoutBuilder& builder) {
        return init(device, fileName, builder.Get());
    }

    /** @brief Actualiza el estado del programa de shaders (actualmente no realiza cambios). */
    void update();

    /**
     * @brief Aplica el programa de shaders al pipeline.
     * @param deviceContext Contexto de dispositivo donde se usará.
     *
     * @details
     * Configura en la GPU el Vertex Shader, Pixel Shader y el Input Layout asociados.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Aplica un tipo específico de shader al pipeline.
     * @param deviceContext Contexto de dispositivo donde se usará.
     * @param type Tipo de shader a aplicar (VERTEX_SHADER o PIXEL_SHADER).
     */
    void render(DeviceContext& deviceContext, ShaderType type);

    /** @brief Libera los recursos asociados a los shaders e Input Layout. */
    void destroy();

    /**
     * @brief Crea el Input Layout del programa.
     * @param device Dispositivo Direct3D.
     * @param Layout Descriptores del formato de vértices.
     * @return HRESULT con el estado de la operación.
     */
    HRESULT CreateInputLayout(Device& device, std::vector<D3D11_INPUT_ELEMENT_DESC> Layout);

    /**
     * @brief Crea un shader del tipo especificado usando el archivo del programa.
     * @param device Dispositivo Direct3D.
     * @param type Tipo de shader a crear.
     * @return HRESULT con el estado de la operación.
     */
    HRESULT CreateShader(Device& device, ShaderType type);

    /**
     * @brief Crea un shader del tipo especificado desde un archivo dado.
     * @param device Dispositivo Direct3D.
     * @param type Tipo de shader a crear.
     * @param fileName Ruta del archivo HLSL.
     * @return HRESULT con el estado de la operación.
     */
    HRESULT CreateShader(Device& device, ShaderType type, const std::string& fileName);

    /**
     * @brief Compila un shader desde archivo HLSL.
     * @param szFileName Ruta del archivo HLSL.
     * @param szEntryPoint Punto de entrada del shader (ej. "VSMain").
     * @param szShaderModel Modelo de shader (ej. "vs_5_0" para vertex shader).
     * @param ppBlobOut Salida con el bytecode compilado.
     * @return HRESULT con el estado de la operación.
     *
     * @note
     * La compilación traduce el código HLSL a un formato binario
     * que Direct3D puede cargar en la GPU.
     */
    HRESULT CompileShaderFromFile(char* szFileName,
        LPCSTR szEntryPoint,
        LPCSTR szShaderModel,
        ID3DBlob** ppBlobOut);

public:
    ID3D11VertexShader* m_VertexShader = nullptr; ///< Shader de vértices.
    ID3D11PixelShader* m_PixelShader = nullptr;   ///< Shader de píxeles.
    InputLayout m_inputLayout;                    ///< Input Layout asociado.
private:
    std::string m_shaderFileName;                 ///< Nombre del archivo del shader.
    ID3DBlob* m_vertexShaderData = nullptr;       ///< Bytecode del Vertex Shader.
    ID3DBlob* m_pixelShaderData = nullptr;        ///< Bytecode del Pixel Shader.
};
