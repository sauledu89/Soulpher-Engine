/**
 * @file UserInterface.cpp
 * @brief Inicializacion, actualizacion y render de la UI (ImGui) de Soulpher-Engine.
 *
 * @details
 * UserInterface usa Dear ImGui en su rama "docking" (ImGuiConfigFlags_DockingEnable)
 * para crear un editor de motor con paneles flotantes y anclables.
 * Cada frame el ciclo es: init una vez -> update() -> render() -> destroy al cerrar.
 *
 * Paneles implementados:
 *  - Outliner: lista jerarquica de actores en escena.
 *  - Inspector: propiedades de transformacion del actor seleccionado (vec3Control).
 *  - Light Panel: direccion y color de la luz + shadow bias.
 *  - Renderer Window: preview del framebuffer como SRV en una ventana ImGui.
 *  - Output: consola de mensajes de debug.
 *  - MenuBar: archivo, captura de pantalla, cambio de tema.
 *
 * @note [GameDev] ImGui usa el patron "Immediate Mode": la UI no tiene estado persistente,
 * se reconstruye completamente cada frame desde cero. Contrario a sistemas "Retained Mode"
 * (WPF, Qt, HTML) donde el arbol de widgets persiste y solo se actualizan los cambios.
 * Immediate Mode es ideal para herramientas de debug/editor porque es simple de implementar
 * y no requiere manejar eventos de UI — simplemente preguntas "isButtonPressed?" y actuas.
 * Unreal Engine usa Slate (Retained Mode) para su editor y UMG para UI en juego.
 * Dear ImGui es el estandar de facto para debug UI en motores custom.
 *
 * @see UserInterface.h, BaseApp, Window, SwapChain
 */

#include "UserInterface.h"
#include "imgui_internal.h"
#include "Screenshot.h"
#include "Window.h"
#include "SwapChain.h"
#include "Texture.h"
#include "MeshComponent.h"
#include "ECS\\Actor.h"

    UserInterface::UserInterface() {}
UserInterface::~UserInterface() {}

void UserInterface::init(void* window, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Entrada y docking
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // <<< Clave para poder arrastrar desde cualquier parte de la ventana
    io.ConfigWindowsMoveFromTitleBarOnly = false;

    // Fuente pixel art — se verifica existencia antes de llamar a ImGui
    // para evitar la IM_ASSERT interna de AddFontFromFileTTF
    {
        ImFontConfig cfg;
        cfg.OversampleH = cfg.OversampleV = 1;
        cfg.PixelSnapH  = true;

        FILE* probe = nullptr;
        fopen_s(&probe, "Minecraftia-Regular.ttf", "rb");
        if (probe) {
            fclose(probe);
            m_mainFont = io.Fonts->AddFontFromFileTTF("Minecraftia-Regular.ttf", 16.0f, &cfg);
        }
        if (!m_mainFont) {
            m_mainFont = io.Fonts->AddFontDefault();
        }
    }

    ImGui::StyleColorsDark();
    CyberpunkStyle();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, deviceContext);

    toolTipData();
    selectedActorIndex = 0;

    m_imguiInitialized = true;
}

void UserInterface::update() {
    // Frame ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Dockspace “global” para un layout más predecible (opcional pero recomendable)
    ImGui::DockSpaceOverViewport(NULL, ImGuiDockNodeFlags_PassthruCentralNode);

    // Siempre visibles
    ToolBar();
    closeApp();
}

void UserInterface::render() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void UserInterface::destroy()
{
    if (!m_imguiInitialized || ImGui::GetCurrentContext() == nullptr)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_imguiInitialized = false;
}

void UserInterface::vec3Control(const std::string& label, float* values, float resetValue, float columnWidth) {
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) values[0] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) values[1] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) values[2] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);

    ImGui::PopID();
}

void UserInterface::menuBar(Window window, SwapChain swapChain, Texture& backBuffer) {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New", "Ctrl+N");
            ImGui::MenuItem("Open", "Ctrl+O");
            ImGui::MenuItem("Save", "Ctrl+S");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Copy", "Ctrl+C");
            ImGui::MenuItem("Paste", "Ctrl+V");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Capture screenshot"))
        {
            // Screenshot sc; sc.captureScreenshot(window, swapChain, backBuffer);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UserInterface::Renderer(Window window, ID3D11ShaderResourceView* renderTexture) {
    bool Stage = true;
    // No forzamos pos cada frame para no “clavar” la ventana
    ImGui::Begin("Renderer", &Stage);
    ImTextureID texId = (ImTextureID)renderTexture;
    ImGui::Image(texId, ImVec2(window.m_width / 2.0f, window.m_height / 2.0f));
    ImGui::End();
}

void UserInterface::inspectorGeneral(EU::TSharedPointer<Actor> actor) {
    ImGui::Begin("Inspector");

    bool isStatic = false;
    ImGui::Checkbox("##Static", &isStatic);
    ImGui::SameLine();

    // Buffer local para editar el nombre — se aplica al actor al confirmar
    char nameBuf[128] = {};
    strncpy_s(nameBuf, actor->getName().c_str(), sizeof(nameBuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.6f);
    if (ImGui::InputText("##ObjectName", nameBuf, sizeof(nameBuf)))
        actor->getName() = nameBuf;
    ImGui::SameLine();

    if (ImGui::Button("Icon")) {
        // icon action
    }

    ImGui::Separator();

    const char* tags[] = { "Untagged", "Player", "Enemy", "Environment" };
    static int currentTag = 0;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
    ImGui::Combo("Tag", &currentTag, tags, IM_ARRAYSIZE(tags));
    ImGui::SameLine();

    const char* layers[] = { "Default", "TransparentFX", "Ignore Raycast", "Water", "UI" };
    static int currentLayer = 0;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
    ImGui::Combo("Layer", &currentLayer, layers, IM_ARRAYSIZE(layers));

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspectorContainer(actor);
    }
    ImGui::End();
}

void UserInterface::inspectorContainer(EU::TSharedPointer<Actor> actor) {
    vec3Control("Position", const_cast<float*>(actor->getComponent<Transform>()->getPosition().data()));
    vec3Control("Rotation", const_cast<float*>(actor->getComponent<Transform>()->getRotation().data()));
    vec3Control("Scale", const_cast<float*>(actor->getComponent<Transform>()->getScale().data()));
}

void UserInterface::output() {
    bool Stage = true;
    ImGui::Begin("Output", &Stage);
    ImGui::End();
}

void UserInterface::darkStyle() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.04f, 0.04f, 0.04f, 0.04f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);

    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        colors[i].x += 0.015f; colors[i].y += 0.025f; colors[i].z += 0.020f;
    }
}

void UserInterface::greyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.PopupRounding = 3;
    style.WindowPadding = ImVec2(4, 4);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 2);
    style.ScrollbarSize = 18;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 2;
    style.WindowRounding = 3;
    style.ChildRounding = 3;
    style.FrameRounding = 3;
    style.ScrollbarRounding = 2;
    style.GrabRounding = 3;
    style.TabBorderSize = 2;
    style.TabRounding = 3;

    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void UserInterface::NeonRedStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // Paleta base
    const ImVec4 bg0 = ImVec4(0.06f, 0.07f, 0.09f, 1.00f); // fondo principal
    const ImVec4 bg1 = ImVec4(0.09f, 0.10f, 0.13f, 1.00f); // fondo secundario
    const ImVec4 bg2 = ImVec4(0.12f, 0.14f, 0.18f, 1.00f); // widgets
    const ImVec4 line = ImVec4(0.25f, 0.26f, 0.30f, 1.00f); // bordes

    // Neón rojo
    const ImVec4 neonR = ImVec4(1.00f, 0.16f, 0.25f, 1.00f);
    const ImVec4 neonRHover = ImVec4(1.00f, 0.26f, 0.35f, 1.00f);
    const ImVec4 neonRAct = ImVec4(1.00f, 0.36f, 0.45f, 1.00f);
    const ImVec4 neonRSoft = ImVec4(1.00f, 0.16f, 0.25f, 0.25f);

    // Curvas / métrica
    style.Alpha = 1.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    // Fondos
    c[ImGuiCol_WindowBg] = bg0;
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = bg1;

    // Bordes / separadores
    c[ImGuiCol_Border] = line;
    c[ImGuiCol_Separator] = line;
    c[ImGuiCol_SeparatorHovered] = neonRHover;
    c[ImGuiCol_SeparatorActive] = neonRAct;

    // Títulos / menús
    c[ImGuiCol_TitleBg] = bg1;
    c[ImGuiCol_TitleBgActive] = bg2;
    c[ImGuiCol_TitleBgCollapsed] = bg1;
    c[ImGuiCol_MenuBarBg] = bg1;

    // Controles
    c[ImGuiCol_FrameBg] = bg2;
    c[ImGuiCol_FrameBgHovered] = neonRSoft;
    c[ImGuiCol_FrameBgActive] = neonRSoft;
    c[ImGuiCol_SliderGrab] = neonR;
    c[ImGuiCol_SliderGrabActive] = neonRAct;
    c[ImGuiCol_CheckMark] = neonR;

    // Botones
    c[ImGuiCol_Button] = neonR;
    c[ImGuiCol_ButtonHovered] = neonRHover;
    c[ImGuiCol_ButtonActive] = neonRAct;

    // Headers (TreeNode/CollapsingHeader)
    c[ImGuiCol_Header] = neonRSoft;
    c[ImGuiCol_HeaderHovered] = neonRHover;
    c[ImGuiCol_HeaderActive] = neonRAct;

    // Tabs
    c[ImGuiCol_Tab] = bg2;
    c[ImGuiCol_TabHovered] = neonRHover;
    c[ImGuiCol_TabActive] = neonRAct;
    c[ImGuiCol_TabUnfocused] = bg2;
    c[ImGuiCol_TabUnfocusedActive] = bg2;

    // Scrollbar
    c[ImGuiCol_ScrollbarBg] = bg1;
    c[ImGuiCol_ScrollbarGrab] = line;
    c[ImGuiCol_ScrollbarGrabHovered] = neonRHover;
    c[ImGuiCol_ScrollbarGrabActive] = neonRAct;

    // Docking / selección
    c[ImGuiCol_DockingPreview] = neonRSoft;
    c[ImGuiCol_TextSelectedBg] = neonRSoft;

    // Texto
    c[ImGuiCol_Text] = ImVec4(0.95f, 0.97f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.62f, 0.70f, 1.00f);

    // Multi-viewport: evita esquinas redondeadas en ventanas flotantes
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}


void UserInterface::CyberpunkStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // ── Fondos ───────────────────────────────────────────────────────────────
    const ImVec4 bg0  = { 0.02f, 0.03f, 0.06f, 1.00f }; // #050810  fondo principal
    const ImVec4 bg1  = { 0.04f, 0.06f, 0.10f, 1.00f }; // #0A0F1A  paneles / popups
    const ImVec4 bg2  = { 0.06f, 0.09f, 0.15f, 1.00f }; // #0F1726  widgets / frames

    // ── Neón cian (acento primario) ───────────────────────────────────────────
    const ImVec4 cyan     = { 0.00f, 0.90f, 1.00f, 1.00f }; // #00E5FF
    const ImVec4 cyanHov  = { 0.20f, 0.95f, 1.00f, 1.00f }; // #33F3FF
    const ImVec4 cyanAct  = { 0.40f, 1.00f, 1.00f, 1.00f }; // #66FFFF
    const ImVec4 cyanFill = { 0.00f, 0.90f, 1.00f, 0.18f }; // cian semitransparente

    // ── Magenta neón (acento secundario – botones, activos) ───────────────────
    const ImVec4 mag     = { 1.00f, 0.18f, 0.47f, 1.00f }; // #FF2F78
    const ImVec4 magHov  = { 1.00f, 0.30f, 0.55f, 1.00f }; // #FF4D8C
    const ImVec4 magAct  = { 1.00f, 0.40f, 0.62f, 1.00f }; // #FF669E
    const ImVec4 magFill = { 1.00f, 0.18f, 0.47f, 0.18f }; // magenta semitransparente

    // ── Borde metálico ────────────────────────────────────────────────────────
    const ImVec4 edge = { 0.10f, 0.20f, 0.30f, 1.00f }; // acero azul oscuro

    // ── Métricas: esquinas casi angulares para look tech ─────────────────────
    style.Alpha             = 1.0f;
    style.WindowRounding    = 3.0f;
    style.FrameRounding     = 2.0f;
    style.GrabRounding      = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding       = 2.0f;
    style.PopupRounding     = 2.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.WindowPadding     = { 8.0f, 6.0f };
    style.FramePadding      = { 6.0f, 4.0f };
    style.ItemSpacing       = { 6.0f, 4.0f };
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    // ── Texto ─────────────────────────────────────────────────────────────────
    c[ImGuiCol_Text]         = { 0.88f, 0.97f, 1.00f, 1.00f }; // blanco cian
    c[ImGuiCol_TextDisabled] = { 0.35f, 0.50f, 0.60f, 1.00f }; // gris apagado

    // ── Fondos de ventanas ────────────────────────────────────────────────────
    c[ImGuiCol_WindowBg]   = bg0;
    c[ImGuiCol_ChildBg]    = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_PopupBg]    = bg1;

    // ── Bordes ────────────────────────────────────────────────────────────────
    c[ImGuiCol_Border]       = edge;
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };

    // ── Frames (inputs, combos, sliders) ─────────────────────────────────────
    c[ImGuiCol_FrameBg]        = bg2;
    c[ImGuiCol_FrameBgHovered] = cyanFill;
    c[ImGuiCol_FrameBgActive]  = cyanFill;

    // ── Barras de título ──────────────────────────────────────────────────────
    c[ImGuiCol_TitleBg]          = bg1;
    c[ImGuiCol_TitleBgActive]    = { 0.03f, 0.12f, 0.18f, 1.00f }; // cian muy oscuro
    c[ImGuiCol_TitleBgCollapsed] = bg0;
    c[ImGuiCol_MenuBarBg]        = bg1;

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    c[ImGuiCol_ScrollbarBg]          = bg1;
    c[ImGuiCol_ScrollbarGrab]        = edge;
    c[ImGuiCol_ScrollbarGrabHovered] = cyan;
    c[ImGuiCol_ScrollbarGrabActive]  = cyanAct;

    // ── Controles interactivos ────────────────────────────────────────────────
    c[ImGuiCol_CheckMark]        = cyan;
    c[ImGuiCol_SliderGrab]       = cyan;
    c[ImGuiCol_SliderGrabActive] = cyanAct;

    // ── Botones: magenta para máximo contraste ────────────────────────────────
    c[ImGuiCol_Button]        = mag;
    c[ImGuiCol_ButtonHovered] = magHov;
    c[ImGuiCol_ButtonActive]  = magAct;

    // ── Headers (CollapsingHeader, TreeNode, Selectable) ─────────────────────
    c[ImGuiCol_Header]        = cyanFill;
    c[ImGuiCol_HeaderHovered] = { 0.00f, 0.90f, 1.00f, 0.30f };
    c[ImGuiCol_HeaderActive]  = cyan;

    // ── Separadores ───────────────────────────────────────────────────────────
    c[ImGuiCol_Separator]        = edge;
    c[ImGuiCol_SeparatorHovered] = cyanHov;
    c[ImGuiCol_SeparatorActive]  = cyanAct;

    // ── Resize grip ───────────────────────────────────────────────────────────
    c[ImGuiCol_ResizeGrip]        = cyanFill;
    c[ImGuiCol_ResizeGripHovered] = cyan;
    c[ImGuiCol_ResizeGripActive]  = cyanAct;

    // ── Tabs ──────────────────────────────────────────────────────────────────
    c[ImGuiCol_Tab]                = bg2;
    c[ImGuiCol_TabHovered]         = { 0.00f, 0.90f, 1.00f, 0.25f };
    c[ImGuiCol_TabActive]          = { 0.03f, 0.20f, 0.28f, 1.00f };
    c[ImGuiCol_TabUnfocused]       = bg1;
    c[ImGuiCol_TabUnfocusedActive] = bg2;

    // ── Docking ───────────────────────────────────────────────────────────────
    c[ImGuiCol_DockingPreview] = cyanFill;
    c[ImGuiCol_DockingEmptyBg] = bg0;

    // ── Plots ─────────────────────────────────────────────────────────────────
    c[ImGuiCol_PlotLines]             = cyan;
    c[ImGuiCol_PlotLinesHovered]      = mag;
    c[ImGuiCol_PlotHistogram]         = cyan;
    c[ImGuiCol_PlotHistogramHovered]  = mag;

    // ── Selección / nav ───────────────────────────────────────────────────────
    c[ImGuiCol_TextSelectedBg]          = cyanFill;
    c[ImGuiCol_DragDropTarget]          = mag;
    c[ImGuiCol_NavHighlight]            = cyan;
    c[ImGuiCol_NavWindowingHighlight]   = cyan;
    c[ImGuiCol_NavWindowingDimBg]       = { 0.00f, 0.00f, 0.00f, 0.60f };
    c[ImGuiCol_ModalWindowDimBg]        = { 0.00f, 0.00f, 0.00f, 0.70f };

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void UserInterface::visualStudioStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    const ImVec4 discord_purple = ImVec4(0.447f, 0.227f, 0.635f, 1.000f);
    const ImVec4 discord_darker_purple = ImVec4(0.337f, 0.157f, 0.486f, 1.000f);
    const ImVec4 discord_light_gray = ImVec4(0.741f, 0.765f, 0.780f, 1.000f);
    const ImVec4 discord_darker_gray = ImVec4(0.169f, 0.188f, 0.204f, 1.000f);
    const ImVec4 discord_blue = ImVec4(0.192f, 0.545f, 0.906f, 1.000f);
    const ImVec4 discord_green = ImVec4(0.129f, 0.694f, 0.403f, 1.000f);
    const ImVec4 discord_light_blue = ImVec4(0.29f, 0.56f, 0.89f, 1.00f);
    const ImVec4 discord_dark_gray = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);

    colors[ImGuiCol_Text] = discord_light_gray;
    colors[ImGuiCol_TextDisabled] = discord_darker_gray;
    colors[ImGuiCol_WindowBg] = discord_purple;
    colors[ImGuiCol_ChildBg] = discord_purple;
    colors[ImGuiCol_PopupBg] = discord_purple;
    colors[ImGuiCol_Border] = discord_darker_gray;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg] = discord_darker_purple;
    colors[ImGuiCol_FrameBgHovered] = discord_blue;
    colors[ImGuiCol_FrameBgActive] = discord_blue;
    colors[ImGuiCol_TitleBg] = discord_darker_purple;
    colors[ImGuiCol_TitleBgActive] = discord_blue;
    colors[ImGuiCol_TitleBgCollapsed] = discord_darker_gray;
    colors[ImGuiCol_MenuBarBg] = discord_darker_purple;
    colors[ImGuiCol_ScrollbarBg] = discord_darker_purple;
    colors[ImGuiCol_ScrollbarGrab] = discord_light_gray;
    colors[ImGuiCol_ScrollbarGrabHovered] = discord_darker_gray;
    colors[ImGuiCol_ScrollbarGrabActive] = discord_blue;
    colors[ImGuiCol_CheckMark] = discord_green;
    colors[ImGuiCol_SliderGrab] = discord_green;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Button] = discord_green;
    colors[ImGuiCol_ButtonHovered] = discord_blue;
    colors[ImGuiCol_ButtonActive] = ImVec4(0.05f, 0.11f, 0.19f, 1.00f);
    colors[ImGuiCol_Header] = discord_green;
    colors[ImGuiCol_HeaderHovered] = discord_blue;
    colors[ImGuiCol_HeaderActive] = ImVec4(0.05f, 0.11f, 0.19f, 1.00f);
    colors[ImGuiCol_Separator] = discord_darker_gray;
    colors[ImGuiCol_SeparatorHovered] = discord_blue;
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.05f, 0.11f, 0.19f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = discord_light_gray;
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_Tab] = discord_dark_gray;
    colors[ImGuiCol_TabHovered] = ImVec4(0.05f, 0.11f, 0.19f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.05f, 0.11f, 0.19f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = discord_darker_gray;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = discord_dark_gray;
    colors[ImGuiCol_PlotLines] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = discord_light_blue;
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = discord_light_blue;
    colors[ImGuiCol_TextSelectedBg] = discord_light_blue;
    colors[ImGuiCol_DragDropTarget] = discord_light_blue;
    colors[ImGuiCol_NavHighlight] = discord_light_blue;
    colors[ImGuiCol_NavWindowingHighlight] = discord_light_blue;
    colors[ImGuiCol_NavWindowingDimBg] = discord_darker_gray;
    colors[ImGuiCol_ModalWindowDimBg] = discord_darker_gray;
}

void UserInterface::ToolTip(std::string icon, std::string tip) {
    ImGui::SameLine();
    ImGui::Text("%s", icon.c_str());
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tip.c_str());
    }
}

void UserInterface::ToolTip(std::string tip) {
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tip.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void UserInterface::toolTipData() {
    m_tooltips.push_back(
        "You can change the current GameObject that is active to be used in the Inspector.\n"
        "\nNOTE:\n* WIP – some things might not work correctly.\n");
    m_tooltips.push_back(
        "You can change the drawing state of the GameObject by activating or deactivating the checkbox.\n"
        "\nNOTE:\n* WIP – some things might not work correctly.\n");
}

void UserInterface::ToolBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            if (ImGui::MenuItem("Exit")) {
                show_exit_popup = true;
                ImGui::OpenPopup("Exit?");
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Options");
            ImGui::MenuItem("Settings");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UserInterface::closeApp() {
    if (show_exit_popup) {
        ImGui::OpenPopup("Exit?");
        show_exit_popup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Estas a punto de salir de la aplicacion.\nEstas seguro?\n\n");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            exit(0);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void UserInterface::RenderFullScreenTransparentWindow() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Importantísimo: que no “coma” el ratón ni tape ventanas
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoInputs;

    ImGui::Begin("FullScreenTransparentWindow", nullptr, window_flags);
    ImGui::End();
}

void UserInterface::lightPanel(float* lightDir, float* lightColor, float& shadowBias) {
    ImGui::Begin("Lighting");

    // --- Luz direccional ---
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Direction");
        ImGui::SameLine();
        ImGui::TextDisabled("(se normaliza)");

        bool changed = ImGui::DragFloat3("##LightDir", lightDir, 0.01f, -1.0f, 1.0f, "%.3f");
        if (changed) {
            // Mantener normalizado para que la iluminación no cambie de intensidad
            float len = sqrtf(lightDir[0]*lightDir[0] + lightDir[1]*lightDir[1] + lightDir[2]*lightDir[2]);
            if (len > 1e-5f) {
                lightDir[0] /= len;
                lightDir[1] /= len;
                lightDir[2] /= len;
            }
        }

        // Preset buttons para dirección de luz
        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Cenital"))  { lightDir[0]=0.0f; lightDir[1]=-1.0f; lightDir[2]=0.0f; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Diagonal")) { lightDir[0]=0.3f; lightDir[1]=-1.0f; lightDir[2]=0.5f;
            float l = sqrtf(0.09f+1.0f+0.25f); lightDir[0]/=l; lightDir[1]/=l; lightDir[2]/=l; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Lateral"))  { lightDir[0]=1.0f; lightDir[1]=-0.3f; lightDir[2]=0.0f;
            float l = sqrtf(1.0f+0.09f); lightDir[0]/=l; lightDir[1]/=l; lightDir[2]/=l; }

        ImGui::Separator();

        ImGui::Text("Color");
        ImGui::ColorEdit3("##LightColor", lightColor,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
    }

    // --- UI ---
    if (ImGui::CollapsingHeader("Interface")) {
        float scale = ImGui::GetIO().FontGlobalScale;
        if (ImGui::SliderFloat("Font Scale", &scale, 0.5f, 2.5f, "%.1fx"))
            ImGui::GetIO().FontGlobalScale = scale;
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##fs")) ImGui::GetIO().FontGlobalScale = 1.0f;
    }

    // --- Sombras ---
    if (ImGui::CollapsingHeader("Shadow Map", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Bias");
        ImGui::SameLine();
        ImGui::TextDisabled("(+acne  -peter pan)");
        ImGui::SliderFloat("##ShadowBias", &shadowBias, 0.0f, 0.02f, "%.5f");

        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) shadowBias = 0.003f;
    }

    ImGui::End();
}

void UserInterface::outliner(const std::vector<EU::TSharedPointer<Actor>>& actors) {
    ImGui::Begin("Hierarchy");

    static ImGuiTextFilter filter;
    filter.Draw("##Search", ImGui::GetContentRegionAvailWidth());
    ImGui::Separator();

    for (int i = 0; i < (int)actors.size(); ++i) {
        const auto& actor = actors[i];
        if (actor.isNull()) continue;

        const std::string& name = const_cast<EU::TSharedPointer<Actor>&>(actor)->getName();
        if (!filter.PassFilter(name.c_str())) continue;

        bool selected = (selectedActorIndex == i);

        // Icono simple según si proyecta o no sombra
        const char* icon = actor->canCastShadow() ? "[*] " : "[ ] ";
        std::string label = icon + name + "##" + std::to_string(i);

        if (ImGui::Selectable(label.c_str(), selected)) {
            selectedActorIndex = i;
        }

        // Tooltip con info rápida
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Cast shadow: %s", actor->canCastShadow() ? "si" : "no");
            ImGui::EndTooltip();
        }
    }

    ImGui::End();
}