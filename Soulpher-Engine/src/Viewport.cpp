/**
 * @file Viewport.cpp
 * @brief Implementación de la clase Viewport.
 */

#include "Viewport.h"
#include "Window.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el viewport usando los parámetros de una ventana.
  *
  * Este método usa el tamaño actual de la ventana para establecer el área de dibujo.
  * El viewport es la región donde se proyectará el render en pantalla (como una "ventana virtual").
  *
  * @param window Referencia a una ventana ya creada.
  * @return HRESULT S_OK si la operación fue exitosa; error si hay dimensiones inválidas o punteros nulos.
  */
HRESULT Viewport::init(const Window& window) {
	if (!window.m_hWnd) {
		ERROR("Viewport", "init", "Window handle (m_hWnd) is nullptr");
		return E_POINTER;
	}
	if (window.m_width == 0 || window.m_height == 0) {
		ERROR("Viewport", "init", "Window dimensions are zero.");
		return E_INVALIDARG;
	}

	// Asignamos dimensiones del viewport según la ventana
	m_viewport.Width = static_cast<float>(window.m_width);
	m_viewport.Height = static_cast<float>(window.m_height);
	m_viewport.MinDepth = 0.0f;   // Profundidad mínima (z-buffer)
	m_viewport.MaxDepth = 1.0f;   // Profundidad máxima
	m_viewport.TopLeftX = 0;      // Coordenada X inicial
	m_viewport.TopLeftY = 0;      // Coordenada Y inicial

	return S_OK;
}

/**
 * @brief Inicializa el viewport manualmente con dimensiones específicas.
 *
 * Útil para cuando no usamos una ventana, como en editores o herramientas off-screen.
 *
 * @param width Ancho del viewport.
 * @param height Alto del viewport.
 * @return HRESULT S_OK si las dimensiones son válidas, error si son cero.
 */
HRESULT Viewport::init(unsigned int width, unsigned int height) {
	if (width == 0 || height == 0) {
		ERROR("Viewport", "init", "Window dimensions are zero.");
		return E_INVALIDARG;
	}

	// Establecemos manualmente el área visible de dibujo
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	return S_OK;
}

/**
 * @brief Activa este viewport en el pipeline gráfico.
 *
 * Esto indica a Direct3D que todo el dibujo posterior debe hacerse dentro del área definida.
 *
 * @param deviceContext Contexto del dispositivo en el que se aplicará este viewport.
 */
void Viewport::render(DeviceContext& deviceContext) {
	if (!deviceContext.m_deviceContext) {
		ERROR("Viewport", "render", "Device context is not set.");
		return;
	}

	// Enviamos la estructura m_viewport al rasterizador
	deviceContext.RSSetViewports(1, &m_viewport);
}
