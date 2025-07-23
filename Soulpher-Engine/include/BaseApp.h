#pragma once
#include "Prerequisites.h"
#include "Window.h"
#include "Device.h"
#include "SwapChain.h"
#include "DeviceContext.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"
#include "Viewport.h"
#include "InputLayout.h"
#include "ShaderProgram.h"
#include "Buffer.h"
#include "MeshComponent.h"
#include "BlendState.h"
#include "UserInterface.h"
#include "ModelLoader.h"
#include "DepthStencilState.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class BaseApp {
public:
    /**
     * @brief Constructor por defecto de la aplicación base.
     */
    BaseApp() = default;
    /**
     * @brief Destructor por defecto de la aplicación base.
     */
    ~BaseApp() = default;

    /**
     * @brief Ejecuta el ciclo principal de la aplicación.
     * @param hInstance Instancia de la aplicación.
     * @param nCmdShow Parámetro de visualización de la ventana.
     * @return Código de salida de la aplicación.
     */
    int run(HINSTANCE hInstance, int nCmdShow);

private:
    // Ciclo de vida de la aplicación
    /**
     * @brief Inicializa la aplicación y sus recursos.
     * @param hInstance Instancia de la aplicación.
     * @param nCmdShow Parámetro de visualización de la ventana.
     * @return Resultado de la inicialización.
     */
    HRESULT init(HINSTANCE hInstance, int nCmdShow);
    /**
     * @brief Actualiza el estado de la aplicación.
     */
    void update();
    /**
     * @brief Renderiza la escena.
     */
    void render();
    /**
     * @brief Libera los recursos de la aplicación.
     */
    void destroy();

    /**
     * @brief Procedimiento de ventana estático.
     * @param hWnd Handle de la ventana.
     * @param message Mensaje recibido.
     * @param wParam Parámetro adicional.
     * @param lParam Parámetro adicional.
     * @return Resultado del procesamiento del mensaje.
     */
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // Componentes de la aplicación
    Window g_window;
    Device g_device;
    SwapChain g_swapChain;
    DeviceContext g_deviceContext;
    Texture g_backBuffer;
    RenderTargetView g_renderTargetView;
    Texture g_depthStencil;
    DepthStencilView g_depthStencilView;
    Viewport g_viewport;
    ShaderProgram g_shaderProgram;
    ShaderProgram g_shaderShadow;
    BlendState g_shadowBlendState;
    DepthStencilState g_shadowDepthStencilState;
    UserInterface g_userInterface;
    ModelLoader g_modelLoader;

    // Buffers de cámara
    Buffer m_neverChanges;
    Buffer m_changeOnResize;

    // Buffers del cubo
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    Buffer m_changeEveryFrame;

    // Buffers de la sombra del cubo
    Buffer m_constShadow;

    // Buffers del plano
    Buffer m_planeVertexBuffer;
    Buffer m_planeIndexBuffer;
    Buffer m_constPlane;

    // Recursos de DirectX
    ID3D11ShaderResourceView* g_pTextureRV = nullptr;
    ID3D11SamplerState* g_pSamplerLinear = nullptr;
    ID3D11DepthStencilState* g_pShadowDepthStencilState = nullptr;

    // Matrices de transformación y estado
    XMMATRIX g_World;
    XMMATRIX g_PlaneWorld;
    XMMATRIX g_View;
    XMMATRIX g_Projection;
    XMFLOAT4 g_vMeshColor;
    XMFLOAT4 g_LightPos;

    XMFLOAT3 g_ModelRotation;
    float g_ModelScale;

    // Geometría
    MeshComponent cubeMesh;
    //MeshComponent planeMesh;

    // Estructuras de Constant Buffers
    CBNeverChanges cbNeverChanges;
    CBChangeOnResize cbChangesOnResize;
    //CBChangesEveryFrame cbPlane;
    CBChangesEveryFrame cb;
    CBChangesEveryFrame cbShadow;
};