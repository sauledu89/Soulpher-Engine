/**
 * @file Screenshot.cpp
 * @brief Implementación de captura de pantalla del back buffer y su integración con la UI.
 *
 * @details
 * Este módulo permite capturar una imagen del contenido actual de la ventana del juego
 * (back buffer) y guardarla como un archivo de imagen.
 *
 * Flujo general:
 *  1. Obtener el **back buffer** desde el `SwapChain`.
 *  2. Crear un contexto de dispositivo de memoria (DC) y un bitmap compatible.
 *  3. Copiar el contenido de la ventana al bitmap.
 *  4. Convertir los datos de píxeles a formato RGB.
 *  5. (Opcional) Guardar en un archivo PNG.
 *
 * Además, se integra un **popup en ImGui** para permitir al usuario capturar la pantalla
 * directamente desde la interfaz del juego.
 *
 * @note
 * - Actualmente el guardado a PNG está comentado porque requiere `stb_image_write.h`.
 * - El sistema usa **GDI** para la captura, lo que es más simple pero menos eficiente
 *   que acceder directamente a los datos de GPU.
 */

#include "Screenshot.h"
#include "Window.h"
#include "SwapChain.h"
#include "Texture.h"
 //#define STB_IMAGE_WRITE_IMPLEMENTATION
 //#include "stb_image_write.h"

 /**
  * @brief Captura el contenido actual del back buffer de la ventana.
  *
  * @param window       Referencia a la ventana activa.
  * @param swapChain    Cadena de intercambio asociada.
  * @param backBuffer   Textura donde se copiará el contenido del back buffer.
  *
  * @details
  * - Usa `GetBuffer()` para acceder al back buffer de Direct3D.
  * - Utiliza funciones de Windows GDI (`BitBlt`, `GetDIBits`) para copiar y leer píxeles.
  * - Convierte el formato de color de **BGR a RGB**.
  *
  * @note
  * Esta función actualmente no guarda la imagen en disco, pero está preparada
  * para hacerlo mediante **stb_image_write**.
  */
void Screenshot::captureScreenshot(Window window, SwapChain swapChain, Texture& backBuffer) {
    // Obtener el back buffer
    swapChain.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&backBuffer.m_texture));

    // Obtener el HWND de la ventana
    HWND hWnd = GetForegroundWindow();

    // Obtener el DC de la ventana
    HDC hDC = GetDC(hWnd);

    // Calcular dimensiones de la ventana
    RECT rect;
    GetClientRect(hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Crear un bitmap compatible en memoria
    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, width, height);
    HDC hMemoryDC = CreateCompatibleDC(hDC);
    SelectObject(hMemoryDC, hBitmap);

    // Copiar el contenido de la ventana al bitmap
    BitBlt(hMemoryDC, 0, 0, width, height, hDC, 0, 0, SRCCOPY);

    // Obtener datos de píxeles
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = window.m_width;
    bmi.bmiHeader.biHeight = -window.m_height; // negativo para evitar imagen invertida
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    std::vector<uint8_t> pixels(window.m_width * window.m_height * 4);
    GetDIBits(hMemoryDC, hBitmap, 0, window.m_height, pixels.data(), &bmi, DIB_RGB_COLORS);

    // Convertir BGR → RGB
    for (size_t i = 0; i < pixels.size(); i += 4) {
        std::swap(pixels[i], pixels[i + 2]);
    }

    // Guardar en archivo PNG (comentado)
    // stbi_write_png("screenshot.png", window.m_width, window.m_height, 4, pixels.data(), window.m_width * 4);

    // Liberar recursos
    DeleteDC(hMemoryDC);
    DeleteObject(hBitmap);
    ReleaseDC(hWnd, hDC);
    backBuffer.m_texture->Release();
}

/**
 * @brief Dibuja en ImGui el popup de captura de pantalla.
 *
 * @param window       Referencia a la ventana activa.
 * @param swapChain    Cadena de intercambio asociada.
 * @param backBuffer   Textura donde se copiará el contenido del back buffer.
 *
 * @details
 * - Muestra un botón "Capture Screenshot".
 * - Abre un popup con opciones para cerrar o capturar.
 * - Llama a `captureScreenshot()` si el usuario lo confirma.
 *
 * @note
 * Esta integración permite que el jugador capture la pantalla
 * sin necesidad de salir del juego.
 */
void Screenshot::ui(Window window, SwapChain swapChain, Texture& backBuffer) {
    if (ImGui::Button("Capture Screenshot"))
    {
        ImGui::OpenPopup("popup_mensaje");
    }
    if (ImGui::BeginPopup("popup_mensaje")) {

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Capture Screenshot")) {
            captureScreenshot(window, swapChain, backBuffer);
        }
        ImGui::EndPopup();
    }
}
