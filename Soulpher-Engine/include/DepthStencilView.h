#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class Texture;

/**
 * @file DepthStencilView.h
 * @brief Encapsula la creaci�n y gesti�n de vistas de profundidad/stencil en Direct3D 11.
 *
 * @details
 * Un **Depth Stencil View (DSV)** es el enlace entre el Depth/Stencil Buffer y el pipeline de render.
 * Se utiliza para:
 * - **Pruebas de profundidad (Depth Test):** Evitar que se dibujen p�xeles que est�n detr�s de otros.
 * - **Pruebas de stencil (Stencil Test):** Controlar qu� partes de la pantalla se pueden modificar.
 *
 * Esta clase se encarga de:
 * - Crear el DSV a partir de una textura de profundidad/stencil.
 * - Enlazarlo al pipeline gr�fico.
 * - Liberar recursos cuando ya no se necesite.
 *
 * @note Para estudiantes:
 * - El DSV es obligatorio en casi todo renderizado 3D con Z-buffering.
 * - Sin un DSV v�lido, el motor no podr� realizar pruebas de profundidad.
 * - El formato (`DXGI_FORMAT`) debe ser compatible con Depth/Stencil (por ejemplo `DXGI_FORMAT_D24_UNORM_S8_UINT`).
 */
class DepthStencilView {
public:
    DepthStencilView() = default;  ///< Constructor por defecto.
    ~DepthStencilView() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa la vista de profundidad/stencil a partir de una textura.
     * @param device Referencia al dispositivo Direct3D.
     * @param depthStencil Textura que actuar� como buffer de profundidad/stencil.
     * @param format Formato de p�xel del DSV (por ejemplo, `DXGI_FORMAT_D24_UNORM_S8_UINT`).
     * @return `S_OK` si la operaci�n fue exitosa, c�digo de error en caso contrario.
     *
     * @note
     * - La textura `depthStencil` debe haberse creado con la bandera `D3D11_BIND_DEPTH_STENCIL`.
     * - El formato debe coincidir con el utilizado en la textura.
     */
    HRESULT init(Device& device, Texture& depthStencil, DXGI_FORMAT format);

    /**
     * @brief Actualiza el estado interno del DSV.
     *
     * @note Actualmente este m�todo no realiza cambios, pero podr�a usarse
     *       para recrear la vista si cambian dimensiones o formato.
     */
    void update();

    /**
     * @brief Enlaza la vista de profundidad/stencil al pipeline gr�fico.
     * @param deviceContext Contexto del dispositivo.
     *
     * @note Esto debe hacerse antes de emitir draw calls para que
     *       las pruebas de profundidad y stencil est�n activas.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera los recursos asociados al DSV.
     *
     * @note Llamar antes de cerrar el dispositivo o cambiar el buffer de profundidad.
     */
    void destroy();

public:
    ID3D11DepthStencilView* m_depthStencilView = nullptr; ///< Puntero a la vista DSV de Direct3D.
};
