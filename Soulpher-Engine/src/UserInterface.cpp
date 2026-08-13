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
#include "Device.h"
#include "MeshComponent.h"
#include "ECS\\Actor.h"
#include "ECS\\LightComponent.h"
#include "Rendering\\MaterialInstance.h"
#include "Rendering\\Material.h"

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

    ImGui::StyleColorsLight();
    NESStyle();

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

/**
 * @copydoc UserInterface::vec3Control
 * @details El estado del checkbox de vínculo se guarda con `ImGui::GetStateStorage()`
 * en vez de con un miembro de la clase.
 *
 * @note [GameDev] `ImGuiStorage` es el mecanismo que usa ImGui internamente para que un
 * widget "recuerde" algo entre frames sin que el código que lo llama tenga que declarar
 * una variable `static`/miembro dedicada — el propio `ImGui::PushID(label)` de arriba ya
 * genera un espacio de IDs único por etiqueta, así que `GetID("##LinkXYZ")` produce un
 * ID distinto para "Position", "Rotation" y "Scale" automáticamente, y cada uno guarda
 * su propio booleano sin colisionar. Es el mismo patrón interno que usa ImGui para
 * recordar, por ejemplo, si un `CollapsingHeader` está abierto o cerrado. La alternativa
 * — un `static bool linked` normal dentro de la función — NO serviría aquí: al ser
 * `static`, ese booleano se compartiría entre las 3 llamadas (Position/Rotation/Scale)
 * del mismo frame, así que activar el vínculo en Position lo activaría también en
 * Rotation y Scale.
 */
void UserInterface::vec3Control(const std::string& label, float* values, float resetValue, float columnWidth) {
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    // Estado del vínculo X/Y/Z, guardado en el storage de ImGui bajo el ID ya escopeado por
    // PushID(label) de arriba — así "Position", "Rotation" y "Scale" tienen cada uno su propio
    // toggle independiente sin necesitar un arreglo/mapa aparte en la clase.
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImGuiID linkStorageId = ImGui::GetID("##LinkXYZ");
    bool linked = storage->GetBool(linkStorageId, false);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    bool changedX = false, changedY = false, changedZ = false;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) { values[0] = resetValue; changedX = true; }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f")) changedX = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) { values[1] = resetValue; changedY = true; }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f")) changedY = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) { values[2] = resetValue; changedZ = true; }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f")) changedZ = true;
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    // Vínculo X/Y/Z: checkbox tipo "cadena" (sin texto, para caber en el panel angosto del
    // Inspector) — cuando está activo, el eje que el usuario acaba de tocar se copia a los
    // otros dos, así los tres quedan sincronizados al mismo valor.
    ImGui::SameLine();
    ImGui::Checkbox("##LinkXYZ", &linked);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Vincular X/Y/Z: al editar un eje, los otros dos se igualan");
    }
    storage->SetBool(linkStorageId, linked);

    if (linked) {
        if (changedX) { values[1] = values[0]; values[2] = values[0]; }
        else if (changedY) { values[0] = values[1]; values[2] = values[1]; }
        else if (changedZ) { values[0] = values[2]; values[1] = values[2]; }
    }

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

/**
 * @copydoc UserInterface::inspectorGeneral
 * @details Además del nombre y el `Transform` (vía `inspectorContainer`), muestra una
 * sección "Light" condicional si `actor` tiene un `LightComponent` — editando sus campos
 * (tipo/color/intensidad/range/spotAngle) directamente sobre la referencia que devuelve
 * `getLightData()`, sin copia intermedia: el próximo `LightComponent::resolve()` (llamado
 * por `BaseApp::render()` este mismo frame) ya ve el valor actualizado.
 */
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

    // Seccion Light — solo si el actor tiene un LightComponent (Light Actor).
    EU::TSharedPointer<LightComponent> lightComp = actor->getComponent<LightComponent>();
    if (!lightComp.isNull()) {
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            LightData& light = lightComp->getLightData();

            const char* typeNames[] = { "Directional", "Point", "Spot" };
            int typeIndex = (int)light.type;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.6f);
            if (ImGui::Combo("Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
                light.type = (LightType)typeIndex;
            }

            ImGui::ColorEdit3("Color", &light.color.x,
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);

            ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 50.0f, "%.2f");

            if (light.type == LightType::Point || light.type == LightType::Spot) {
                ImGui::DragFloat("Range", &light.range, 0.1f, 0.1f, 200.0f, "%.2f");
            }
            if (light.type == LightType::Spot) {
                float angleDeg = XMConvertToDegrees(light.spotAngle);
                if (ImGui::DragFloat("Spot Angle", &angleDeg, 0.5f, 1.0f, 89.0f, "%.1f deg")) {
                    light.spotAngle = XMConvertToRadians(angleDeg);
                }
            }
        }
    }

    ImGui::End();
}

void UserInterface::inspectorContainer(EU::TSharedPointer<Actor> actor) {
    vec3Control("Position", const_cast<float*>(actor->getComponent<Transform>()->getPosition().data()));
    vec3Control("Rotation", const_cast<float*>(actor->getComponent<Transform>()->getRotation().data()));
    vec3Control("Scale", const_cast<float*>(actor->getComponent<Transform>()->getScale().data()));
}

void UserInterface::output() {
    static bool showMessage = true;
    static bool showWarning = true;
    static bool showError   = true;

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        LogManager::instance().clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Message", &showMessage);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);
    ImGui::Separator();

    ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const LogEntry& entry : LogManager::instance().entries()) {
        if (entry.level == LogLevel::Message && !showMessage) continue;
        if (entry.level == LogLevel::Warning && !showWarning) continue;
        if (entry.level == LogLevel::Error   && !showError)   continue;

        ImVec4 color;
        switch (entry.level) {
            case LogLevel::Warning: color = ImVec4(1.00f, 0.80f, 0.20f, 1.0f); break;
            case LogLevel::Error:   color = ImVec4(1.00f, 0.35f, 0.35f, 1.0f); break;
            default:                color = ImVec4(0.165f, 0.165f, 0.165f, 1.0f); break; // #2A2A2A — legible sobre fondo gris claro
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(entry.text.c_str());
        ImGui::PopStyleColor();
    }
    // Auto-scroll: solo si el usuario ya estaba al final (no lo forzamos si scrolleo hacia arriba a leer algo).
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

void UserInterface::statsPanel(float fps, float frameTimeMs, unsigned int drawCalls) {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame time: %.2f ms", frameTimeMs);
    ImGui::Text("Draw calls: %u", drawCalls);
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

/**
 * @copydoc UserInterface::NESStyle
 * @details Estructura idéntica a `CyberpunkStyle()`: variables locales con la paleta
 * base (fondos, acento primario, acento secundario, bordes), seguidas de las métricas
 * de la ventana (rounding/padding/border) y luego la asignación explícita de CADA
 * `ImGuiCol_*` relevante — sin dejar ningún color "heredado" del tema base que pudiera
 * quedar mal sobre el fondo claro.
 *
 * @note [GameDev] Diseñar un tema de color no es solo "elegir bonitos colores": hay que
 * garantizar CONTRASTE consistente en cada combinación texto/fondo y estado/estado
 * (reposo vs. hover vs. activo) para que la UI siga siendo legible y predecible. Aquí
 * el criterio fue: rojo Nintendo para el acento "de marca" (headers activos, checkmarks,
 * botones primarios), naranja Zapper para estados de "atención del usuario" (hover,
 * resaltados, separadores), y texto casi negro sobre el gris claro de fondo — la misma
 * lógica de "color primario / color de acento / color de texto con contraste mínimo
 * garantizado" que usan los design systems de UI profesionales (Material Design,
 * Fluent UI de Microsoft, etc.), aplicada aquí a mano en vez de con una herramienta.
 */
void UserInterface::NESStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // ── Fondos: gris claro tipo plástico de la NES original ───────────────────
    const ImVec4 bg0 = { 0.753f, 0.753f, 0.753f, 1.00f }; // #C0C0C0 fondo principal
    const ImVec4 bg1 = { 0.659f, 0.659f, 0.659f, 1.00f }; // #A8A8A8 popups / barras de título
    const ImVec4 bg2 = { 0.847f, 0.847f, 0.847f, 1.00f }; // #D8D8D8 frames/widgets (look "inset")

    // ── Rojo Nintendo (acento primario) ────────────────────────────────────────
    const ImVec4 red     = { 0.902f, 0.000f, 0.071f, 1.00f }; // #E60012
    const ImVec4 redHov  = { 1.000f, 0.200f, 0.267f, 1.00f }; // #FF3344
    const ImVec4 redAct  = { 0.702f, 0.000f, 0.055f, 1.00f }; // #B3000E (más oscuro al presionar)
    const ImVec4 redFill = { 0.902f, 0.000f, 0.071f, 0.25f };

    // ── Naranja Zapper (headers/títulos/separadores + botones secundarios/highlights) ──
    const ImVec4 orange       = { 0.941f, 0.439f, 0.188f, 1.00f }; // #F07030
    const ImVec4 orangeHov    = { 1.000f, 0.549f, 0.314f, 1.00f }; // #FF8C50
    const ImVec4 orangeAct    = { 0.780f, 0.320f, 0.100f, 1.00f }; // #C7521A (más oscuro al presionar)
    const ImVec4 orangeFill   = { 0.941f, 0.439f, 0.188f, 0.30f };
    const ImVec4 orangeHeader = { 0.941f, 0.439f, 0.188f, 0.55f }; // headers en reposo: naranja semi-sólido

    // ── Bordes: bisel oscuro tipo carcasa de consola ───────────────────────────
    const ImVec4 edge = { 0.376f, 0.376f, 0.376f, 1.00f }; // #606060

    // ── Métricas: esquinas casi rectas, look de cartucho ──────────────────────
    style.Alpha             = 1.0f;
    style.WindowRounding    = 2.0f;
    style.FrameRounding     = 1.0f;
    style.GrabRounding      = 1.0f;
    style.ScrollbarRounding = 1.0f;
    style.TabRounding       = 1.0f;
    style.PopupRounding     = 1.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.WindowPadding     = { 8.0f, 6.0f };
    style.FramePadding      = { 6.0f, 4.0f };
    style.ItemSpacing       = { 6.0f, 4.0f };
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    // ── Texto: gris oscuro/negro para contraste sobre el fondo claro ──────────
    c[ImGuiCol_Text]         = { 0.10f, 0.10f, 0.10f, 1.00f }; // #1A1A1A
    c[ImGuiCol_TextDisabled] = { 0.44f, 0.44f, 0.44f, 1.00f }; // #707070

    // ── Fondos de ventanas ────────────────────────────────────────────────────
    c[ImGuiCol_WindowBg] = bg0;
    c[ImGuiCol_ChildBg]  = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_PopupBg]  = bg1;

    // ── Bordes ────────────────────────────────────────────────────────────────
    c[ImGuiCol_Border]       = edge;
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };

    // ── Frames (inputs, combos, sliders) ─────────────────────────────────────
    c[ImGuiCol_FrameBg]        = bg2;
    c[ImGuiCol_FrameBgHovered] = orangeFill;
    c[ImGuiCol_FrameBgActive]  = redFill;

    // ── Barras de título ──────────────────────────────────────────────────────
    c[ImGuiCol_TitleBg]          = bg1;
    c[ImGuiCol_TitleBgActive]    = orange; // naranja Zapper: ventana activa/enfocada
    c[ImGuiCol_TitleBgCollapsed] = bg0;
    c[ImGuiCol_MenuBarBg]        = bg1;

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    c[ImGuiCol_ScrollbarBg]          = bg1;
    c[ImGuiCol_ScrollbarGrab]        = { 0.56f, 0.56f, 0.56f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = orange;
    c[ImGuiCol_ScrollbarGrabActive]  = red;

    // ── Controles interactivos ────────────────────────────────────────────────
    c[ImGuiCol_CheckMark]        = red;
    c[ImGuiCol_SliderGrab]       = red;
    c[ImGuiCol_SliderGrabActive] = orange;

    // ── Botones: rojo primario, naranja al pasar el mouse (highlight) ────────
    c[ImGuiCol_Button]        = red;
    c[ImGuiCol_ButtonHovered] = orangeHov;
    c[ImGuiCol_ButtonActive]  = redAct;

    // ── Headers (CollapsingHeader, TreeNode, Selectable) — naranja Zapper ────
    c[ImGuiCol_Header]        = orangeHeader;
    c[ImGuiCol_HeaderHovered] = orangeHov;
    c[ImGuiCol_HeaderActive]  = orange;

    // ── Separadores de sección — naranja Zapper ──────────────────────────────
    c[ImGuiCol_Separator]        = orange;
    c[ImGuiCol_SeparatorHovered] = orangeHov;
    c[ImGuiCol_SeparatorActive]  = orangeAct;

    // ── Resize grip ───────────────────────────────────────────────────────────
    c[ImGuiCol_ResizeGrip]        = orangeFill;
    c[ImGuiCol_ResizeGripHovered] = orange;
    c[ImGuiCol_ResizeGripActive]  = red;

    // ── Tabs ──────────────────────────────────────────────────────────────────
    c[ImGuiCol_Tab]                = bg2;
    c[ImGuiCol_TabHovered]         = orangeFill;
    c[ImGuiCol_TabActive]          = redFill;
    c[ImGuiCol_TabUnfocused]       = bg1;
    c[ImGuiCol_TabUnfocusedActive] = bg2;

    // ── Docking ───────────────────────────────────────────────────────────────
    c[ImGuiCol_DockingPreview] = orangeFill;
    c[ImGuiCol_DockingEmptyBg] = bg0;

    // ── Plots ─────────────────────────────────────────────────────────────────
    c[ImGuiCol_PlotLines]            = red;
    c[ImGuiCol_PlotLinesHovered]     = orange;
    c[ImGuiCol_PlotHistogram]        = red;
    c[ImGuiCol_PlotHistogramHovered] = orange;

    // ── Selección / nav ───────────────────────────────────────────────────────
    c[ImGuiCol_TextSelectedBg]        = orangeFill;
    c[ImGuiCol_DragDropTarget]        = orange;
    c[ImGuiCol_NavHighlight]          = red;
    c[ImGuiCol_NavWindowingHighlight] = red;
    c[ImGuiCol_NavWindowingDimBg]     = { 0.20f, 0.20f, 0.20f, 0.40f };
    c[ImGuiCol_ModalWindowDimBg]      = { 0.20f, 0.20f, 0.20f, 0.50f };

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

void UserInterface::lightPanel(float& shadowBias, bool& viewShadowMap) {
    ImGui::Begin("Lighting");

    // La direccion/color/intensidad de las luces ahora se editan por Light Actor (seleccionar
    // el actor "Sun" u otro Light Actor en Hierarchy -> seccion "Light" del Inspector) en vez
    // de un unico slider global aqui.
    ImGui::TextDisabled("Direccion/color/intensidad: selecciona un Light Actor en Hierarchy.");

    // --- Sombras ---
    if (ImGui::CollapsingHeader("Shadow Map", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Bias");
        ImGui::SameLine();
        ImGui::TextDisabled("(+acne  -peter pan)");
        ImGui::SliderFloat("##ShadowBias", &shadowBias, 0.0f, 0.02f, "%.5f");

        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) shadowBias = 0.003f;

        ImGui::Checkbox("Ver Shadow Map", &viewShadowMap);
    }

    ImGui::End();
}

void UserInterface::interfacePanel() {
    ImGui::Begin("Interface");

    float scale = ImGui::GetIO().FontGlobalScale;
    if (ImGui::SliderFloat("Font Scale", &scale, 0.5f, 2.5f, "%.1fx"))
        ImGui::GetIO().FontGlobalScale = scale;
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset##fs")) ImGui::GetIO().FontGlobalScale = 1.0f;

    ImGui::End();
}

/**
 * @copydoc UserInterface::outliner
 */
void UserInterface::outliner(const std::vector<EU::TSharedPointer<Actor>>& actors) {
    ImGui::Begin("Hierarchy");

    // "+ Add Light": solo marca la intención (requestedLightType) — no construye ningún
    // Actor aquí (ver nota de "comando diferido" en el @file de UserInterface.h).
    if (ImGui::Button("+ Add Light")) {
        ImGui::OpenPopup("AddLightPopup");
    }
    if (ImGui::BeginPopup("AddLightPopup")) {
        if (ImGui::MenuItem("Directional Light")) requestedLightType = (int)LightType::Directional;
        if (ImGui::MenuItem("Point Light"))        requestedLightType = (int)LightType::Point;
        if (ImGui::MenuItem("Spot Light"))         requestedLightType = (int)LightType::Spot;
        ImGui::EndPopup();
    }

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

        // Menú contextual (clic derecho): igual que "+ Add Light", solo marca el índice —
        // BaseApp::update() hace el duplicado/borrado real al inicio del siguiente frame.
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Duplicate")) requestedDuplicateIndex = i;
            if (ImGui::MenuItem("Delete"))    requestedDeleteIndex = i;
            ImGui::EndPopup();
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

namespace {
    /**
     * @enum MaterialTextureSlot
     * @brief Los seis slots de textura que expone `MaterialInstance` — solo se usa dentro
     * de `materialEditor()` para no repetir el mismo switch seis veces.
     */
    enum class MaterialTextureSlot { Albedo, Normal, Metallic, Roughness, AO, Emissive };

    /** @brief `MaterialInstance::get{Albedo,Normal,...}()` elegido por `slot` en runtime. */
    Texture* materialEditorGetSlot(MaterialInstance* inst, MaterialTextureSlot slot) {
        switch (slot) {
        case MaterialTextureSlot::Albedo:    return inst->getAlbedo();
        case MaterialTextureSlot::Normal:    return inst->getNormal();
        case MaterialTextureSlot::Metallic:  return inst->getMetallic();
        case MaterialTextureSlot::Roughness: return inst->getRoughness();
        case MaterialTextureSlot::AO:        return inst->getAO();
        case MaterialTextureSlot::Emissive:  return inst->getEmissive();
        }
        return nullptr;
    }

    /** @brief `MaterialInstance::set{Albedo,Normal,...}(tex)` elegido por `slot` en runtime. */
    void materialEditorSetSlot(MaterialInstance* inst, MaterialTextureSlot slot, Texture* tex) {
        switch (slot) {
        case MaterialTextureSlot::Albedo:    inst->setAlbedo(tex);    break;
        case MaterialTextureSlot::Normal:    inst->setNormal(tex);    break;
        case MaterialTextureSlot::Metallic:  inst->setMetallic(tex);  break;
        case MaterialTextureSlot::Roughness: inst->setRoughness(tex); break;
        case MaterialTextureSlot::AO:        inst->setAO(tex);        break;
        case MaterialTextureSlot::Emissive:  inst->setEmissive(tex);  break;
        }
    }

    const char* materialEditorSlotLabel(MaterialTextureSlot slot) {
        switch (slot) {
        case MaterialTextureSlot::Albedo:    return "Albedo";
        case MaterialTextureSlot::Normal:    return "Normal";
        case MaterialTextureSlot::Metallic:  return "Metallic";
        case MaterialTextureSlot::Roughness: return "Roughness";
        case MaterialTextureSlot::AO:        return "AO";
        case MaterialTextureSlot::Emissive:  return "Emissive";
        }
        return "";
    }
}

/**
 * @copydoc UserInterface::materialEditor
 */
void UserInterface::materialEditor(std::vector<MaterialEditorEntry>& materials,
    std::vector<MaterialRenderSlot>& renderSlots,
    Device& device,
    std::deque<Texture>& texturePool,
    std::deque<MaterialInstance>& materialPool,
    Material& defaultMaterial,
    Material& transparentMaterial,
    Material& maskedMaterial) {

    static int   s_selectedMaterial = 0;
    static char  s_loadPathBuf[260] = {};
    // Slot pendiente de asignación: se fija al pulsar "Load..." en un slot y se consume
    // en el popup "LoadMaterialTexture" que se abre justo después.
    static MaterialInstance*   s_pendingInstance = nullptr;
    static MaterialTextureSlot s_pendingSlot     = MaterialTextureSlot::Albedo;
    static char  s_newMaterialName[128] = {};
    static int   s_newMaterialDomain = 0; // 0=Opaque, 1=Transparent, 2=Masked — ver Create más abajo.
    static int   s_targetRenderSlot = 0;
    static int   s_renameSyncedIndex = -1; // último índice para el que s_renameBuf ya tiene el nombre cargado.
    static char  s_renameBuf[128] = {};
    // Las primeras 5 entradas de `materials` son siempre los materiales built-in (Kirby,
    // Plane, SciFiToad Body/Glass/Head — ver BaseApp::init()), cuyo `name` el botón "Assign"
    // usa para mantener sincronizada la lista con MaterialRenderSlot (ver ese botón más
    // abajo). Permitir renombrarlas rompería esa sincronización por nombre; los materiales
    // creados en el editor (índice >= esto) no tienen esa restricción.
    constexpr int kBuiltInMaterialCount = 5;

    ImGui::Begin("Material Editor");

    if (materials.empty()) {
        ImGui::TextDisabled("No hay materiales registrados.");
        ImGui::End();
        return;
    }
    if (s_selectedMaterial >= (int)materials.size()) s_selectedMaterial = 0;

    ImGui::BeginChild("MaterialEditorList", ImVec2(200.0f, 0.0f), true);
    for (int i = 0; i < (int)materials.size(); ++i) {
        // "##i": nombres duplicados (dos materiales con el mismo nombre, nada lo impide)
        // no deben colapsar al mismo ID de ImGui — sin el índice, colisionan hover/click
        // entre ambas filas.
        std::string rowId = materials[i].name + "##" + std::to_string(i);
        if (ImGui::Selectable(rowId.c_str(), s_selectedMaterial == i)) {
            s_selectedMaterial = i;
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("New Material");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##NewMaterialName", "Name...", s_newMaterialName, sizeof(s_newMaterialName));
    // Combo en vez de RadioButton en fila: la lista de materiales mide solo 200px de ancho
    // (ver MaterialEditorList mas abajo) y tres botones lado a lado no entraban — el
    // tercero quedaba cortado/ilegible fuera del panel.
    //
    // [GameDev] Opaque/Transparent/Masked no son "grados de transparencia" en una escala —
    // son TRES TÉCNICAS distintas para manejar alpha, cada una con su propio costo:
    //  - Opaque:      alpha ignorado. Escribe profundidad, se ordena por material (no por
    //                 distancia) en el G-Buffer — el camino más barato.
    //  - Masked:      alpha "todo o nada" via clip(albedo.a - AlphaCutoff) en el shader del
    //                 G-Buffer (DeferredGBuffer.hlsl:105) — cada píxel se descarta o se
    //                 dibuja 100% opaco, así que SIGUE escribiendo profundidad y ordenándose
    //                 por material. Sirve para hojas, rejillas, alambre de púas: bordes
    //                 duros, no un fade suave.
    //  - Transparent: alpha real via blending (SRC_ALPHA/INV_SRC_ALPHA). No escribe
    //                 profundidad, necesita el pase forward aparte y ordenarse
    //                 back-to-front por objeto (ver DeferredRenderer::buildQueues) — el
    //                 camino más caro, y el único que da un fundido suave real.
    // El slider "Alpha Cutoff" de más abajo solo tiene efecto en Masked; en Opaque y
    // Transparent el shader lo ignora (por diseño, no por bug).
    static const char* kNewMaterialDomainNames[] = { "Opaque", "Transparent", "Masked" };
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##NewMaterialDomain", &s_newMaterialDomain, kNewMaterialDomainNames,
        IM_ARRAYSIZE(kNewMaterialDomainNames));
    // materialPool es std::deque: emplace_back nunca invalida los MaterialInstance* que
    // ya haya repartido a otras entries o a MaterialRenderSlot::target (ver @copydoc).
    if (ImGui::Button("Create", ImVec2(-1.0f, 0.0f)) && s_newMaterialName[0] != '\0') {
        materialPool.emplace_back();
        MaterialInstance& newMat = materialPool.back();
        switch (s_newMaterialDomain) {
        case 1:
            newMat.setMaterial(&transparentMaterial);
            // Sin esto, un material Transparent nuevo arranca en baseColor.a=1.0 (el
            // default de MaterialParams) — visualmente indistinguible de uno opaco hasta
            // que el usuario nota que hay que bajar el alpha a mano (ver nota en
            // BaseApp::init() junto al 0.35f fijo del vidrio de SciFiToad).
            newMat.getParams().baseColor.w = 0.5f;
            break;
        case 2:
            newMat.setMaterial(&maskedMaterial);
            break;
        default:
            newMat.setMaterial(&defaultMaterial);
            break;
        }
        materials.push_back({ std::string(s_newMaterialName), &newMat });
        s_selectedMaterial  = (int)materials.size() - 1;
        s_newMaterialName[0] = '\0';
        s_newMaterialDomain = 0;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("MaterialEditorDetail", ImVec2(0.0f, 0.0f), true);
    MaterialInstance* inst = materials[s_selectedMaterial].instance;
    if (inst == nullptr) {
        ImGui::TextDisabled("MaterialInstance nulo.");
    }
    else {
        MaterialParams& params = inst->getParams();

        if (s_selectedMaterial >= kBuiltInMaterialCount) {
            // Material creado en el editor: nombre editable. El buffer se resincroniza solo
            // cuando cambia la selección (no cada frame) para no pisar lo que el usuario esta
            // escribiendo mientras edita.
            if (s_renameSyncedIndex != s_selectedMaterial) {
                strncpy_s(s_renameBuf, materials[s_selectedMaterial].name.c_str(), sizeof(s_renameBuf) - 1);
                s_renameSyncedIndex = s_selectedMaterial;
            }
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputText("##RenameMaterial", s_renameBuf, sizeof(s_renameBuf))) {
                materials[s_selectedMaterial].name = s_renameBuf;
            }
        }
        else {
            // Built-in: nombre fijo (identifica el slot de render, ver nota junto a
            // kBuiltInMaterialCount).
            ImGui::Text("%s", materials[s_selectedMaterial].name.c_str());
        }
        ImGui::SameLine();
        // materialPool es std::deque: emplace_back nunca invalida los MaterialInstance* que
        // ya haya repartido a otras entries o a MaterialRenderSlot::target (ver @copydoc de
        // esta función) — seguro duplicar aunque el pool crezca despues.
        if (ImGui::Button("Duplicate")) {
            materialPool.emplace_back(*inst); // copy ctor: mismo Material*, mismas Texture*, copia de MaterialParams.
            MaterialInstance& dup = materialPool.back();
            materials.push_back({ materials[s_selectedMaterial].name + " (Copy)", &dup });
            s_selectedMaterial = (int)materials.size() - 1;
        }

        // Solo materiales creados en el editor son borrables (ver nota de
        // kBuiltInMaterialCount): los built-in identifican un MaterialRenderSlot por nombre
        // y borrarlos rompería esa sincronización.
        if (s_selectedMaterial >= kBuiltInMaterialCount) {
            ImGui::SameLine();
            // No se borra el MaterialInstance de materialPool (std::deque): un erase en medio
            // invalidaria TODOS los punteros ya repartidos a otras entries y a
            // MaterialRenderSlot::target, no solo el borrado (ver @copydoc de esta funcion).
            // Borrar solo la entry de `materials` es seguro y suficiente: el slot deja de
            // aparecer en la lista editable; la instancia sigue viva en el pool, igual que
            // cualquier otra nunca liberada.
            bool inUse = false;
            for (const MaterialRenderSlot& slot : renderSlots) {
                if (*(slot.target) == inst) { inUse = true; break; }
            }
            if (inUse) {
                ImGui::TextDisabled("Delete");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Reasigna este material desde \"Assign to Render Slot\" a otro material antes de borrarlo.");
                }
            }
            else if (ImGui::Button("Delete")) {
                materials.erase(materials.begin() + s_selectedMaterial);
                if (s_selectedMaterial >= (int)materials.size()) {
                    s_selectedMaterial = (int)materials.size() - 1;
                }
                s_renameSyncedIndex = -1; // fuerza resync de s_renameBuf con la nueva seleccion.
                ImGui::EndChild();
                ImGui::End();
                return;
            }
        }
        ImGui::Separator();

        ImGui::ColorEdit4("Base Color", &params.baseColor.x);
        ImGui::SliderFloat("Metallic", &params.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &params.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("AO", &params.ao, 0.0f, 1.0f);
        ImGui::SliderFloat("Normal Scale", &params.normalScale, 0.0f, 2.0f);
        ImGui::SliderFloat("Emissive Strength", &params.emissiveStrength, 0.0f, 10.0f);
        ImGui::SliderFloat("Alpha Cutoff", &params.alphaCutoff, 0.0f, 1.0f);
        // Repeticiones de textura en U/V — multiplica las UVs del vértice en el shader
        // (ver DeferredGBuffer.hlsl / Soulpher-Engine.fx), no las UVs propias de la malla.
        // Si la malla ya trae tiling horneado (ej. el plano de suelo, en 6x6), esto se
        // aplica ENCIMA de eso: el tiling efectivo es el del vértice multiplicado por esto.
        ImGui::DragFloat2("UV Tiling", &params.uvTiling.x, 0.05f, 0.01f, 64.0f, "%.2f");
        // Desplazamiento aplicado DESPUES del tiling — sin límite fijo, un patrón repetido
        // "da la vuelta" naturalmente, así que desplazarlo mucho no rompe nada visualmente.
        ImGui::DragFloat2("UV Offset", &params.uvOffset.x, 0.01f, 0.0f, 0.0f, "%.2f");

        ImGui::Separator();
        ImGui::Text("Texture Slots");

        const MaterialTextureSlot slots[] = {
            MaterialTextureSlot::Albedo,   MaterialTextureSlot::Normal,
            MaterialTextureSlot::Metallic, MaterialTextureSlot::Roughness,
            MaterialTextureSlot::AO,       MaterialTextureSlot::Emissive
        };

        for (MaterialTextureSlot slot : slots) {
            Texture* tex = materialEditorGetSlot(inst, slot);
            if (tex != nullptr && tex->srv() != nullptr) {
                ImGui::Image((ImTextureID)tex->srv(), ImVec2(48.0f, 48.0f));
            }
            else {
                ImGui::Dummy(ImVec2(48.0f, 48.0f));
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("%s", materialEditorSlotLabel(slot));
            // Labels sufijados con el slot (no ImGui::PushID) para que cada boton tenga un ID
            // unico por fila sin cambiar el ID stack en el que se abre el popup — OpenPopup()
            // y BeginPopup() calculan el ID del popup contra el ID stack vigente en cada
            // llamada, y deben coincidir para que el popup aparezca (ver nota mas abajo).
            std::string loadButtonId = std::string("Load...##") + materialEditorSlotLabel(slot);
            if (ImGui::Button(loadButtonId.c_str())) {
                s_pendingInstance = inst;
                s_pendingSlot     = slot;
                s_loadPathBuf[0]  = '\0';
                ImGui::OpenPopup("LoadMaterialTexture");
            }
            ImGui::SameLine();
            std::string clearButtonId = std::string("Clear##") + materialEditorSlotLabel(slot);
            if (ImGui::Button(clearButtonId.c_str())) {
                materialEditorSetSlot(inst, slot, nullptr);
            }
            ImGui::EndGroup();
        }

        if (!renderSlots.empty()) {
            ImGui::Separator();
            ImGui::Text("Assign to Render Slot");
            if (s_targetRenderSlot >= (int)renderSlots.size()) s_targetRenderSlot = 0;
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo("##TargetRenderSlot", renderSlots[s_targetRenderSlot].label.c_str())) {
                for (int i = 0; i < (int)renderSlots.size(); ++i) {
                    if (ImGui::Selectable(renderSlots[i].label.c_str(), s_targetRenderSlot == i)) {
                        s_targetRenderSlot = i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            // *(target) es el MaterialInstance* que BaseApp::render() bindea de verdad ese
            // slot — reasignarlo aqui cambia el material visible en el viewport sin tocar
            // el if/else por nombre de BaseApp::render() (ver @copydoc de esta funcion).
            if (ImGui::Button("Assign")) {
                *(renderSlots[s_targetRenderSlot].target) = inst;
                // Mantiene sincronizada la lista de la izquierda: sin esto, la entry
                // "built-in" cuyo nombre coincide con el slot reasignado (ej. "Kirby")
                // queda apuntando al MaterialInstance original ya no bindeado a nada —
                // seleccionarla y editarla despues no tendria ningun efecto visible, sin
                // ningun aviso de por que.
                const std::string& slotLabel = renderSlots[s_targetRenderSlot].label;
                for (MaterialEditorEntry& entry : materials) {
                    if (entry.name == slotLabel) {
                        entry.instance = inst;
                        break;
                    }
                }
            }
        }
    }

    // Popup compartido por todos los slots — s_pendingInstance/s_pendingSlot identifican
    // a cuál se aplica el resultado (fijados por el botón "Load..." que lo abrió).
    if (ImGui::BeginPopup("LoadMaterialTexture")) {
        ImGui::Text("Load %s texture", materialEditorSlotLabel(s_pendingSlot));
        ImGui::InputText("Path (sin extension)", s_loadPathBuf, sizeof(s_loadPathBuf));
        ImGui::TextDisabled("Ej: ModelsFBX\\piedra");

        // emplace_back en un deque nunca invalida Texture* ya entregados a otros
        // MaterialInstance (ver nota de arquitectura en UserInterface::materialEditor).
        auto loadAndAssign = [&](ExtensionType ext) {
            if (s_pendingInstance == nullptr || s_loadPathBuf[0] == '\0') return;
            texturePool.emplace_back();
            Texture& newTex = texturePool.back();
            HRESULT hr = newTex.init(device, s_loadPathBuf, ext);
            if (SUCCEEDED(hr)) {
                materialEditorSetSlot(s_pendingInstance, s_pendingSlot, &newTex);
            }
            else {
                texturePool.pop_back();
                LOG_ERROR("UserInterface", "materialEditor", "Failed to load texture from path");
            }
            ImGui::CloseCurrentPopup();
            };

        if (ImGui::Button("Load PNG")) loadAndAssign(PNG);
        ImGui::SameLine();
        if (ImGui::Button("Load JPG")) loadAndAssign(JPG);
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::End();
}