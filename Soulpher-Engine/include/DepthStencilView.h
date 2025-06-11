#pragma once
#include "Prerequisites.h"

/// @file DepthStencilView.h
/// @brief Clase que administra una vista de profundidad y stencil (DepthStencilView) para DirectX 11.
///
/// La DepthStencilView permite aplicar t�cnicas como z-buffering (comparaci�n de profundidad)
/// y operaciones de m�scara con stencil, ambas fundamentales en el pipeline de gr�ficos 3D.
///
/// @note Esta clase encapsula el uso de ID3D11DepthStencilView, permitiendo mayor control
/// y modularidad en motores gr�ficos personalizados.

// Forward declarations para evitar dependencias circulares
class Device;
class DeviceContext;
class Texture;

/**
 * @class DepthStencilView
 * @brief Encapsula una vista de profundidad y stencil para Direct3D 11.
 *
 * Esta clase permite gestionar una Depth Stencil View asociada a una textura.
 * Su uso es esencial en el proceso de renderizado para:
 * - Determinar qu� fragmentos son visibles (test de profundidad).
 * - Aplicar t�cnicas avanzadas como stencil shadows, outlines, y m�scaras.
 *
 * @note Se utiliza com�nmente junto con RenderTargetView para renderizado completo.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/d3d11/
 */
class DepthStencilView {
public:
    /**
     * @brief Constructor por defecto.
     */
    DepthStencilView() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~DepthStencilView() = default;

    /**
     * @brief Inicializa la vista de profundidad y stencil a partir de una textura existente.
     *
     * Crea una interfaz ID3D11DepthStencilView con el formato y recursos provistos.
     *
     * @param device Referencia al dispositivo Direct3D 11.
     * @param depthStencil Textura que funcionar� como buffer de profundidad y stencil.
     * @param format Formato DXGI a utilizar (por ejemplo: DXGI_FORMAT_D24_UNORM_S8_UINT).
     * @return HRESULT C�digo de �xito o error del sistema DirectX.
     *
     * @note Este paso es crucial para activar pruebas de profundidad en la escena.
     */
    HRESULT init(Device& device, Texture& depthStencil, DXGI_FORMAT format);

    /**
     * @brief M�todo de actualizaci�n por cuadro.
     *
     * Por defecto no realiza ninguna acci�n. Puede extenderse para operaciones din�micas
     * como cambios en el comportamiento de stencil o reemplazo de buffers.
     */
    void update();

    /**
     * @brief Activa la DepthStencilView y limpia el buffer de profundidad y stencil.
     *
     * Este m�todo se llama generalmente al inicio de cada frame para limpiar el z-buffer
     * y preparar el renderizado correcto de geometr�a 3D.
     *
     * @param deviceContext Contexto del dispositivo (pipeline de comandos de renderizado).
     *
     * @note Asegura que el buffer est� limpio antes del render. Si no se llama,
     * los fragmentos anteriores pueden interferir con la visualizaci�n actual.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera los recursos asociados a la vista.
     *
     * Llama a `Release()` sobre la interfaz COM si existe, y asegura que el puntero
     * quede en estado nulo para evitar accesos inv�lidos.
     */
    void destroy();

public:
    /**
     * @brief Puntero a la interfaz nativa ID3D11DepthStencilView.
     *
     * Es el recurso que DirectX utiliza internamente para aplicar pruebas
     * de profundidad y operaciones de stencil.
     *
     * @note No debe manipularse directamente fuera de esta clase.
     */
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
};
