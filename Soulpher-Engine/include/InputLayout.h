#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file InputLayout.h
 * @brief Encapsula la definición del formato de los datos de vértice para el Input Assembler de Direct3D 11.
 *
 * @details
 * El **Input Layout** describe cómo se interpretan los datos contenidos en un Vertex Buffer
 * y cómo se envían al Vertex Shader.
 *
 * Un `InputLayout` se crea a partir de:
 * - Una lista de descriptores `D3D11_INPUT_ELEMENT_DESC` que define cada atributo de vértice
 *   (posición, normal, color, coordenadas UV…).
 * - El bytecode compilado del Vertex Shader, ya que este determina qué atributos se esperan.
 *
 * @note Para estudiantes:
 * - Si el Input Layout no coincide con la estructura de vértices usada o con las entradas declaradas en el Vertex Shader, **el render fallará**.
 * - Este es uno de los pasos clave en la configuración inicial del pipeline gráfico.
 * - Cambiar de Input Layout puede ser costoso en rendimiento, así que agrupa el render de objetos con el mismo formato de vértice.
 */
class InputLayout {
public:
    InputLayout() = default;  ///< Constructor por defecto.
    ~InputLayout() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa el Input Layout.
     * @param device Dispositivo de Direct3D usado para la creación.
     * @param Layout Vector de descriptores de elementos de entrada (`D3D11_INPUT_ELEMENT_DESC`).
     * @param VertexShaderData Bytecode del Vertex Shader asociado (compilado con HLSL).
     * @return `S_OK` si la operación fue exitosa, código de error en caso contrario.
     *
     * @note
     * - `Layout` debe coincidir exactamente con las entradas declaradas en el Vertex Shader.
     * - Cada descriptor en `Layout` define un atributo: nombre semántico, formato, offset, etc.
     */
    HRESULT init(Device& device,
        std::vector<D3D11_INPUT_ELEMENT_DESC>& Layout,
        ID3DBlob* VertexShaderData);

    /**
     * @brief Actualiza el estado interno del Input Layout (si es necesario).
     *
     * @note Actualmente no implementado, pero podría usarse para regenerar el layout si cambian los datos de vértice.
     */
    void update();

    /**
     * @brief Asigna este Input Layout al pipeline de renderizado.
     * @param deviceContext Contexto del dispositivo donde se aplicará.
     *
     * @note Esto debe hacerse antes de emitir draw calls, para que el Input Assembler interprete los vértices correctamente.
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
