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
     * @brief Constructor por defecto de la aplicaci�n base.
     */
    BaseApp() = default;
    /**
     * @brief Destructor por defecto de la aplicaci�n base.
     */
    ~BaseApp() = default;

    /**
     * @brief Ejecuta el ciclo principal de la aplicaci�n.
     * @param hInstance Instancia de la aplicaci�n.
     * @param nCmdShow Par�metro de visualizaci�n de la ventana.
     * @return C�digo de salida de la aplicaci�n.
     */
    int run(HINSTANCE hInstance, int nCmdShow);

private:
    // Ciclo de vida de la aplicaci�n
    /**
     * @brief Inicializa la aplicaci�n y sus recursos.
     * @param hInstance Instancia de la aplicaci�n.
     * @param nCmdShow Par�metro de visualizaci�n de la ventana.
     * @return Resultado de la inicializaci�n.
     */
    HRESULT init(HINSTANCE hInstance, int nCmdShow);
    /**
     * @brief Actualiza el estado de la aplicaci�n.
     */
    void update();
    /**
     * @brief Renderiza la escena.
     */
    void render();
    /**
     * @brief Libera los recursos de la aplicaci�n.
     */
    void destroy();

    /**
     * @brief Procedimiento de ventana est�tico.
     * @param hWnd Handle de la ventana.
     * @param message Mensaje recibido.
     * @param wParam Par�metro adicional.
     * @param lParam Par�metro adicional.
     * @return Resultado del procesamiento del mensaje.
     */
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // Componentes de la aplicaci�n
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

    // Buffers de c�mara
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

    // Matrices de transformaci�n y estado
    XMMATRIX g_World;
    XMMATRIX g_PlaneWorld;
    XMMATRIX g_View;
    XMMATRIX g_Projection;
    XMFLOAT4 g_vMeshColor;
    XMFLOAT4 g_LightPos;

    XMFLOAT3 g_ModelRotation;
    float g_ModelScale;

    // Geometr�a
    MeshComponent cubeMesh;
    //MeshComponent planeMesh;

    // Estructuras de Constant Buffers
    CBNeverChanges cbNeverChanges;
    CBChangeOnResize cbChangesOnResize;
    //CBChangesEveryFrame cbPlane;
    CBChangesEveryFrame cb;
    CBChangesEveryFrame cbShadow;
};