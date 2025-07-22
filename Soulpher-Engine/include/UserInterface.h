#pragma once    
#include "Prerequisites.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"


/**
 * @class UserInterface
 * @brief Gestiona la interfaz de usuario utilizando ImGui.
 *
 * Esta clase encapsula la inicialización, actualización y renderizado de la interfaz de usuario.
 */
    class
    UserInterface {
    public:
        UserInterface() = default;
        ~UserInterface() = default;

        void
            init(void* window, ID3D11Device* device, ID3D11DeviceContext* deviceContext);

        void
            update();

        void
            render();

        void
            destroy();

        void
            vec3Control(std::string label, float* values, float resetValue = 0.0f, float columnWidth = 100.0f);

        void
            floatControl(const std::string& label, float* value, float resetValue = 0.0f, float columnWidth = 100.0f);

    private:
};
