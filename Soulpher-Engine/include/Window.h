#pragma once
#include "Prerequisites.h"

/**
 * @file Window.h
 * @class Window
 * @brief Maneja la creación y control de la ventana principal del motor gráfico.
 */
class Window {
public:
	Window() = default;
	~Window() = default;

	HRESULT init(HINSTANCE hInstance, int nCmdShow, WNDPROC wndproc);
	void update();
	void render();
	void destroy();

	HWND m_hWnd = nullptr;
	unsigned int m_width;
	unsigned int m_height;

private:
	HINSTANCE m_hInst = nullptr;
	RECT m_rect;
	std::string m_windowName = "Soulpher Engine";
};
