#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @file RasterizerState.h
 * @brief Estado del rasterizador configurable para DX11.
 * @ingroup rendering
 */

/**
 * @class RasterizerState
 * @brief Encapsula `ID3D11RasterizerState` con configuración de fill, cull y clip.
 *
 * @details
 * A diferencia de la clase `Rasterizer` (parámetros fijos), esta clase expone
 * configuración por instancia para cubrir todos los passes del deferred pipeline:
 * - **Shadow pass**: `CULL_FRONT` (evita shadow acne en caras internas), `depthClip=true`.
 * - **G-buffer geometry**: `CULL_BACK` (default), `FILL_SOLID`.
 * - **Fullscreen lighting quad**: `CULL_NONE` (triángulo cubre toda la pantalla).
 *
 * Provee dos sobrecargas de `init()`:
 *  1. Sin parámetros (usa defaults seguros: Solid + BackCull + DepthClip ON).
 *  2. Completa: fill, cull, frontCCW (winding) y depthClip.
 *
 * @note [GameDev] `FrontCounterClockwise` define qué cara de un triángulo es "frontal".
 * En DX11 el default es CW (clockwise = frontal). Modelos exportados de Blender suelen
 * usar CCW, así que ajustar este flag evita tener que dar vuelta los índices. En Unreal
 * Engine el winding depende del eje forward (Y-forward vs Z-forward); cambiar de motor
 * requiere ajustar este flag o flippear los índices del mesh.
 */
class RasterizerState {
public:
    RasterizerState()  = default;
    ~RasterizerState() = default;

    /**
     * @brief Inicializa con valores seguros: FILL_SOLID, CULL_BACK, depthClip=true.
     * @param device Dispositivo Direct3D 11.
     * @return S_OK si ok.
     */
    HRESULT init(Device& device);

    /**
     * @brief Inicializa el estado con parámetros completos.
     * @param device    Dispositivo Direct3D 11.
     * @param fillMode  `D3D11_FILL_SOLID` o `D3D11_FILL_WIREFRAME`.
     * @param cullMode  `D3D11_CULL_NONE`, `D3D11_CULL_FRONT` o `D3D11_CULL_BACK`.
     * @param frontCCW  `true` = caras CCW son frontales (convención OpenGL/Blender).
     * @param depthClip `true` = recortar geometría fuera del near/far clip (recomendado).
     * @return S_OK si ok.
     */
    HRESULT init(Device& device,
                 D3D11_FILL_MODE fillMode,
                 D3D11_CULL_MODE cullMode,
                 bool            frontCCW,
                 bool            depthClip);

    /** @brief Aplica el estado al rasterizador del contexto. */
    void render(DeviceContext& deviceContext);

    /** @brief Reservado para actualización dinámica de parámetros. */
    void update();

    /** @brief Libera el recurso COM. */
    void destroy();

private:
    ID3D11RasterizerState* m_rasterizerState = nullptr; ///< Estado DX11 interno.
};
