/**
 * @file Soulpher-Engine.cpp
 * @brief Punto de entrada Win32 y gateway de mensajes — núcleo de arranque de Soulpher-Engine.
 *
 * @details
 * Este archivo cumple dos funciones críticas:
 *  1. Define el punto de entrada de la aplicación en Windows (`wWinMain`),
 *     el cual inicializa el motor llamando a `BaseApp::run`.
 *  2. Implementa el procedimiento de ventana (`WndProc`), encargado de
 *     procesar los mensajes enviados por el sistema operativo.
 *
 * En el flujo del motor:
 *  - `wWinMain` es donde comienza todo: se instancia la aplicación base (`BaseApp`)
 *    y se delega el control a su bucle principal.
 *  - `WndProc` es un "traductor" entre Windows y el motor. Todos los eventos
 *    (teclado, ratón, redimensionado, cierre, etc.) pasan por aquí.
 *
 * @note
 * Se da prioridad al backend de ImGui (`ImGui_ImplWin32_WndProcHandler`) para
 * manejar entradas de la interfaz gráfica antes que el motor.
 *
 * @par Relación con otros módulos:
 * - **BaseApp**: Controla la inicialización y el ciclo de vida del motor.
 * - **UserInterface**: Contiene las herramientas y paneles de ImGui.
 * - **Win32 API**: Proporciona la comunicación con el sistema operativo.
 *
 * @note [GameDev] wWinMain (Unicode) vs WinMain (ANSI): en motores modernos se usa
 * siempre la version Unicode. En Unreal Engine, WinMain esta en
 * Engine/Source/Runtime/Launch/Private/Windows/LaunchWindows.cpp — igual que aqui,
 * es el punto de entrada mas fino posible antes de delegar todo a FEngineLoop.
 * WndProc es el "message pump": todos los eventos de teclado, raton y resize pasan
 * por aqui. ImGui_ImplWin32_WndProcHandler DEBE procesarlos primero para que los
 * paneles de ImGui puedan capturar input sin que el motor lo intercepte.
 */

#include <windows.h>
#include "BaseApp.h"
#include "UserInterface.h"

 // ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"

// Reenvío de eventos Win32 → ImGui (externo, provisto por el backend de ImGui)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Declaración adelantada del procedimiento de ventana
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * @brief Punto de entrada principal para aplicaciones Win32.
 *
 * @param hInstance     Instancia de la aplicación.
 * @param hPrevInstance Reservado (siempre NULL).
 * @param lpCmdLine     Línea de comandos.
 * @param nCmdShow      Modo de visualización inicial de la ventana.
 * @return int          Código de salida de la aplicación.
 *
 * @note
 * Aquí se instancia la clase `BaseApp` y se llama a su método `run`,
 * que inicia todo el motor, crea la ventana y comienza el bucle principal.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Abre una consola auxiliar para diagnostico en tiempo real
    AllocConsole();
    FILE* pf = nullptr;
    freopen_s(&pf, "CONOUT$", "w", stdout);
    freopen_s(&pf, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Soulpher-Engine — Debug Console");

    // Directorio de trabajo real en tiempo de ejecucion
    char cwd[512] = {};
    GetCurrentDirectoryA(512, cwd);
    printf("[BOOT] Working directory : %s\n", cwd);
    printf("[BOOT] -----------------------------------------------\n");

    BaseApp app;
    int result = app.run(hInstance, hPrevInstance, lpCmdLine, nCmdShow, WndProc);

    printf("[BOOT] App exited with code %d\n", result);
    FreeConsole();
    return result;
}

/**
 * @brief Procedimiento de ventana (Window Procedure).
 *
 * @details
 * Recibe y procesa todos los mensajes enviados a la ventana:
 *  - Eventos de teclado y ratón.
 *  - Redibujado (`WM_PAINT`).
 *  - Cierre de ventana (`WM_DESTROY`).
 *
 * @note
 * Antes de procesar los mensajes, se delegan a `ImGui_ImplWin32_WndProcHandler`
 * para que la UI de ImGui pueda capturar interacciones como drag & drop,
 * clicks o escritura en campos de texto.
 *
 * @param hWnd   Handle de la ventana.
 * @param message Código del mensaje recibido.
 * @param wParam Parámetro adicional (dependiente del mensaje).
 * @param lParam Parámetro adicional (dependiente del mensaje).
 * @return LRESULT Resultado del procesamiento del mensaje.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 1) Permitir que ImGui procese primero; si lo consume, retornar inmediatamente.
    if (ImGui::GetCurrentContext() &&
        ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true; // Importante para drag y manejo de inputs
    }

    // 2) Manejo normal de mensajes
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0); // Notifica al bucle principal que debe cerrar
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
