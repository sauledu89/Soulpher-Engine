#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file InputLayout.h
 * @brief Encapsula la definici�n del formato de los datos de v�rtice para el Input Assembler de Direct3D 11.
 *
 * @details
 * El **Input Layout** describe c�mo se interpretan los datos contenidos en un Vertex Buffer
 * y c�mo se env�an al Vertex Shader.
 *
 * Un `InputLayout` se crea a partir de:
 * - Una lista de descriptores `D3D11_INPUT_ELEMENT_DESC` que define cada atributo de v�rtice
 *   (posici�n, normal, color, coordenadas UV�).
 * - El bytecode compilado del Vertex Shader, ya que este determina qu� atributos se esperan.
 *
 * @note Para estudiantes:
 * - Si el Input Layout no coincide con la estructura de v�rtices usada o con las entradas declaradas en el Vertex Shader, **el render fallar�**.
 * - Este es uno de los pasos clave en la configuraci�n inicial del pipeline gr�fico.
 * - Cambiar de Input Layout puede ser costoso en rendimiento, as� que agrupa el render de objetos con el mismo formato de v�rtice.
 *
 * @note [GameDev] El Input Layout es el "contrato" entre la CPU (SimpleVertex en C++) y
 * el Vertex Shader (input struct en HLSL con semantics POSITION, NORMAL, TANGENT, etc.).
 * Si los semantics, formatos o strides no coinciden, la GPU interpreta los bytes mal y
 * produce geometria corrupta o crashes silenciosos.
 * En DX12 y Vulkan esto se llama "Vertex Input State" y es parte del Pipeline State Object
 * (PSO). En DX12, cambiar el vertex format requiere crear un nuevo PSO completo.
 * Una optimizacion comun es usar un solo vertex format universal (como SimpleVertex aqui)
 * para todos los objetos y evitar cambiar el Input Layout entre draw calls.
 */
class InputLayout {
public:
    InputLayout() = default;  ///< Constructor por defecto.
    ~InputLayout() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa el Input Layout.
     * @param device Dispositivo de Direct3D usado para la creaci�n.
     * @param Layout Vector de descriptores de elementos de entrada (`D3D11_INPUT_ELEMENT_DESC`).
     * @param VertexShaderData Bytecode del Vertex Shader asociado (compilado con HLSL).
     * @return `S_OK` si la operaci�n fue exitosa, c�digo de error en caso contrario.
     *
     * @note
     * - `Layout` debe coincidir exactamente con las entradas declaradas en el Vertex Shader.
     * - Cada descriptor en `Layout` define un atributo: nombre sem�ntico, formato, offset, etc.
     */
    HRESULT init(Device& device,
        std::vector<D3D11_INPUT_ELEMENT_DESC>& Layout,
        ID3DBlob* VertexShaderData);

    /**
     * @brief Actualiza el estado interno del Input Layout (si es necesario).
     *
     * @note Actualmente no implementado, pero podr�a usarse para regenerar el layout si cambian los datos de v�rtice.
     */
    void update();

    /**
     * @brief Asigna este Input Layout al pipeline de renderizado.
     * @param deviceContext Contexto del dispositivo donde se aplicar�.
     *
     * @note Esto debe hacerse antes de emitir draw calls, para que el Input Assembler interprete los v�rtices correctamente.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera los recursos asociados al Input Layout.
     *
     * @note Siempre llamar antes de cerrar el dispositivo o recrear el layout para evitar fugas de memoria.
     */
    void destroy();

public:
    ID3D11InputLayout* m_inputLayout = nullptr; ///< Puntero a la interfaz de Input Layout de Direct3D 11.
};
