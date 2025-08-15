/**
 * @file InputLayout.cpp
 * @brief Creación y aplicación de un Input Layout para el ensamblado de vértices en Direct3D 11.
 *
 * @details
 * Un Input Layout describe cómo están organizados los datos de los vértices en memoria
 * y cómo serán interpretados por el pipeline gráfico durante la etapa de Input Assembler (IA).
 *
 * En desarrollo de videojuegos, este paso es fundamental para que la GPU sepa cómo mapear
 * los datos (posición, color, normales, coordenadas UV, etc.) al formato esperado por el Vertex Shader.
 *
 * Ejemplo:
 * - Si tu malla tiene vértices con posición y color, tu Input Layout debe describir ambas entradas
 *   para que el Vertex Shader las reciba correctamente.
 *
 * @note Este código requiere que el Vertex Shader ya esté compilado para poder crear el Input Layout,
 *       ya que usa su bytecode para validar la compatibilidad del formato.
 */

#include "InputLayout.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el Input Layout.
  *
  * @param device Referencia al objeto Device, necesario para crear recursos en GPU.
  * @param Layout Vector con la descripción de cada elemento del vértice (posición, color, UV, etc.).
  * @param VertexShaderData Bytecode compilado del Vertex Shader, usado para validar el Input Layout.
  *
  * @return HRESULT S_OK si se creó correctamente, o un código de error si falló.
  *
  * @warning El `Layout` debe coincidir exactamente con la estructura de entrada definida en el Vertex Shader.
  * @note Es común definir este layout una sola vez al inicio, y reutilizarlo para todas las mallas que compartan el mismo formato.
  */
HRESULT InputLayout::init(Device& device,
    std::vector<D3D11_INPUT_ELEMENT_DESC>& Layout,
    ID3DBlob* VertexShaderData) {
    if (Layout.empty()) {
        ERROR("InputLayout", "init", "Layout vector is empty.");
        return E_INVALIDARG;
    }
    if (!VertexShaderData) {
        ERROR("InputLayout", "init", "VertexShaderData is nullptr.");
        return E_POINTER;
    }

    HRESULT hr = device.CreateInputLayout(Layout.data(),
        static_cast<unsigned int>(Layout.size()),
        VertexShaderData->GetBufferPointer(),
        VertexShaderData->GetBufferSize(),
        &m_inputLayout);

    if (FAILED(hr)) {
        ERROR("InputLayout", "init",
            ("Failed to create InputLayout. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Método de actualización (actualmente no implementado).
 * @note Se podría usar si se quisiera cambiar el formato de entrada en tiempo de ejecución.
 */
void InputLayout::update() {
    // Método vacío, se puede utilizar en caso de necesitar cambios dinámicos en el layout
}

/**
 * @brief Aplica este Input Layout al pipeline gráfico.
 *
 * @param deviceContext Contexto del dispositivo, usado para establecer el layout activo.
 *
 * @note Este método debe llamarse antes de dibujar cualquier malla que dependa de este formato de vértice.
 */
void InputLayout::render(DeviceContext& deviceContext) {
    if (!m_inputLayout) {
        ERROR("InputLayout", "render", "InputLayout is nullptr");
        return;
    }

    deviceContext.m_deviceContext->IASetInputLayout(m_inputLayout);
}

/**
 * @brief Libera la memoria asociada al Input Layout.
 * @note Siempre debe llamarse en la fase de limpieza del motor para evitar fugas de memoria.
 */
void InputLayout::destroy() {
    SAFE_RELEASE(m_inputLayout);
}
