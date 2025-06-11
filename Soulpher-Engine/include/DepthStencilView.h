#pragma once
#include "Prerequisites.h"

/// @file DepthStencilView.h
/// @brief Clase que administra una vista de profundidad y stencil (DepthStencilView) para DirectX 11.
///
/// La DepthStencilView permite aplicar técnicas como z-buffering (comparación de profundidad)
/// y operaciones de máscara con stencil, ambas fundamentales en el pipeline de gráficos 3D.
///
/// @note Esta clase encapsula el uso de ID3D11DepthStencilView, permitiendo mayor control
/// y modularidad en motores gráficos personalizados.

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
 * - Determinar qué fragmentos son visibles (test de profundidad).
 * - Aplicar técnicas avanzadas como stencil shadows, outlines, y máscaras.
 *
 * @note Se utiliza comúnmente junto con RenderTargetView para renderizado completo.
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
     * @param depthStencil Textura que funcionará como buffer de profundidad y stencil.
     * @param format Formato DXGI a utilizar (por ejemplo: DXGI_FORMAT_D24_UNORM_S8_UINT).
     * @return HRESULT Código de éxito o error del sistema DirectX.
     *
     * @note Este paso es crucial para activar pruebas de profundidad en la escena.
     */
    HRESULT init(Device& device, Texture& depthStencil, DXGI_FORMAT format);

    /**
     * @brief Método de actualización por cuadro.
     *
     * Por defecto no realiza ninguna acción. Puede extenderse para operaciones dinámicas
     * como cambios en el comportamiento de stencil o reemplazo de buffers.
     */
    void update();

    /**
     * @brief Activa la DepthStencilView y limpia el buffer de profundidad y stencil.
     *
     * Este método se llama generalmente al inicio de cada frame para limpiar el z-buffer
     * y preparar el renderizado correcto de geometría 3D.
     *
     * @param deviceContext Contexto del dispositivo (pipeline de comandos de renderizado).
     *
     * @note Asegura que el buffer esté limpio antes del render. Si no se llama,
     * los fragmentos anteriores pueden interferir con la visualización actual.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera los recursos asociados a la vista.
     *
     * Llama a `Release()` sobre la interfaz COM si existe, y asegura que el puntero
     * quede en estado nulo para evitar accesos inválidos.
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
