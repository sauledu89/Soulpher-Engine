/**
 * @file EditorViewportPass.h
 * @brief Render target offscreen con color + depth listo para ImGui o post-proceso.
 *
 * @details
 * Crea un par de texturas (color + depth) como destino de renderizado alternativo
 * al back-buffer. El resultado puede mostrarse como imagen en ImGui (`getSRV()`)
 * o pasar a un pass posterior.
 *
 * El orden de los campos privados es crítico: DeferredRenderer.cpp accede a ellos
 * via reinterpret_cast (EditorViewportPassAccess) para extraer el RTV y DSV nativos.
 * No reordenar ni cambiar los tipos sin actualizar DeferredRenderer.cpp.
 *
 * @note [GameDev] Esto es lo que Unity llama "RenderTexture" y Unreal "SceneRenderTarget".
 * Es el building block de cualquier efecto post-proceso: render → textura → quad → pantalla.
 *
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"

class Device;
class DeviceContext;

/**
 * @class EditorViewportPass
 * @brief Offscreen render target: textura de color (RTV+SRV) + depth buffer (DSV).
 */
class EditorViewportPass {
public:
    EditorViewportPass()  = default;
    ~EditorViewportPass() = default;

    /**
     * @brief Crea las texturas de color y depth para el viewport.
     * @param device Dispositivo Direct3D 11.
     * @param width  Ancho en píxeles.
     * @param height Alto en píxeles.
     * @return S_OK si todos los recursos se crearon correctamente.
     */
    HRESULT init(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Recrea los recursos con nuevas dimensiones.
     * @return S_OK si ok.
     */
    HRESULT resize(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Limpia el RTV y DSV con el color dado. No enlaza el render target.
     * @param deviceContext Contexto Direct3D 11.
     * @param clearColor    Color RGBA con el que limpiar.
     */
    void begin(DeviceContext& deviceContext, const float clearColor[4]);

    /**
     * @brief Configura el D3D11 viewport al tamaño del target.
     * @param deviceContext Contexto Direct3D 11.
     */
    void setViewport(DeviceContext& deviceContext);

    /**
     * @brief Limpia solo el depth buffer (sin tocar el color).
     * @param deviceContext Contexto Direct3D 11.
     */
    void clearDepth(DeviceContext& deviceContext);

    /** @brief True si el RTV y DSV están creados y listos. */
    bool isValid() const { return m_rtv.get() != nullptr && m_dsv.m_depthStencilView != nullptr; }

    /** @brief Libera todos los recursos GPU. */
    void destroy();

    /** @brief SRV de la textura de color para leer en shaders o ImGui. */
    ID3D11ShaderResourceView* getSRV()    const { return m_colorSRV.m_textureFromImg; }

    unsigned int getWidth()  const { return m_width; }
    unsigned int getHeight() const { return m_height; }

private:
    // ¡ORDEN CRÍTICO! EditorViewportPassAccess en DeferredRenderer.cpp depende de este layout.
    Texture          m_colorTexture;   ///< Textura R8G8B8A8 de color (RTV + SRV bind flags).
    Texture          m_colorSRV;       ///< Alias SRV sobre m_colorTexture (para leer en shaders).
    RenderTargetView m_rtv;            ///< Vista de render target sobre m_colorTexture.
    Texture          m_depthTexture;   ///< Textura D24_UNORM_S8 de profundidad.
    DepthStencilView m_dsv;            ///< Vista de depth stencil sobre m_depthTexture.
    unsigned int     m_width  = 1;     ///< Ancho del viewport en píxeles.
    unsigned int     m_height = 1;     ///< Alto del viewport en píxeles.
};
