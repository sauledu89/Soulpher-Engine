/**
 * @file Viewport.cpp
 * @brief Implementaci�n de la clase Viewport.
 */

#include "Viewport.h"
#include "Window.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el viewport usando los par�metros de una ventana.
  *
  * Este m�todo usa el tama�o actual de la ventana para establecer el �rea de dibujo.
  * El viewport es la regi�n donde se proyectar� el render en pantalla (como una "ventana virtual").
  *
  * @param window Referencia a una ventana ya creada.
  * @return HRESULT S_OK si la operaci�n fue exitosa; error si hay dimensiones inv�lidas o punteros nulos.
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

	// Asignamos dimensiones del viewport seg�n la ventana
	m_viewport.Width = static_cast<float>(window.m_width);
	m_viewport.Height = static_cast<float>(window.m_height);
	m_viewport.MinDepth = 0.0f;   // Profundidad m�nima (z-buffer)
	m_viewport.MaxDepth = 1.0f;   // Profundidad m�xima
	m_viewport.TopLeftX = 0;      // Coordenada X inicial
	m_viewport.TopLeftY = 0;      // Coordenada Y inicial

	return S_OK;
}

/**
 * @brief Inicializa el viewport manualmente con dimensiones espec�ficas.
 *
 * �til para cuando no usamos una ventana, como en editores o herramientas off-screen.
 *
 * @param width Ancho del viewport.
 * @param height Alto del viewport.
 * @return HRESULT S_OK si las dimensiones son v�lidas, error si son cero.
 */
HRESULT Viewport::init(unsigned int width, unsigned int height) {
	if (width == 0 || height == 0) {
		ERROR("Viewport", "init", "Window dimensions are zero.");
		return E_INVALIDARG;
	}

	// Establecemos manualmente el �rea visible de dibujo
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	return S_OK;
}

/**
 * @brief Activa este viewport en el pipeline gr�fico.
 *
 * Esto indica a Direct3D que todo el dibujo posterior debe hacerse dentro del �rea definida.
 *
 * @param deviceContext Contexto del dispositivo en el que se aplicar� este viewport.
 */
void Viewport::render(DeviceContext& deviceContext) {
	if (!deviceContext.m_deviceContext) {
		ERROR("Viewport", "render", "Device context is not set.");
		return;
	}

	// Enviamos la estructura m_viewport al rasterizador
	deviceContext.RSSetViewports(1, &m_viewport);
}
