/**
 * @file Viewport.h
 * @brief Declaración de la clase Viewport para gestión de áreas de renderizado en Direct3D 11.
 *
 * @details
 * Un **viewport** es la región de la superficie de render donde Direct3D dibuja la escena.
 *
 * ---
 * 🔹 **Conceptos clave para estudiantes**:
 * - Piensa en el viewport como un "marco" o "ventana" dentro de la pantalla donde se muestra la imagen.
 * - Puedes tener un viewport que ocupe **toda la ventana** o dividir la pantalla en varios viewports (ej. pantalla dividida).
 * - El viewport no cambia el contenido de la escena, solo dónde y cómo se muestra.
 * - Se usa junto con la proyección y la vista de la cámara para calcular la imagen final.
 *
 * ---
 * 💡 **Ejemplo de uso en un motor**:
 * - Inicializar el viewport para que coincida con el tamaño de la ventana principal.
 * - Configurar viewports adicionales para depuración (ej. ver mapa de sombras en un recuadro).
 */

#pragma once
#include "Prerequisites.h"

class Window;
class DeviceContext;

/**
 * @class Viewport
 * @brief Define el área de renderizado en el pipeline gráfico de Direct3D 11.
 *
 * @note Forma parte del motor gráfico **The Visionary**.
 */
class Viewport {
public:
    /** @brief Constructor por defecto: no asigna valores iniciales. */
    Viewport() = default;

    /** @brief Destructor por defecto. */
    ~Viewport() = default;

    /**
     * @brief Inicializa el viewport usando el tamaño de la ventana asociada.
     * @param window Ventana desde la que se obtendrán ancho y alto.
     * @return HRESULT con el estado de la operación.
     *
     * @note Ideal para que el área de render coincida siempre con la ventana principal.
     */
    HRESULT init(const Window& window);

    /**
     * @brief Inicializa el viewport con valores personalizados.
     * @param width Ancho en píxeles del área de render.
     * @param height Alto en píxeles del área de render.
     * @return HRESULT con el estado de la operación.
     *
     * @note Útil para mini-mapas, cámaras de vista previa o efectos en pantalla dividida.
     */
    HRESULT init(unsigned int width, unsigned int height);

    /** @brief Actualiza propiedades internas del viewport si es necesario. */
    void update();

    /**
     * @brief Activa este viewport en el pipeline de render.
     * @param deviceContext Contexto de dispositivo donde se aplicará.
     *
     * @warning Llamar a este método antes de dibujar la escena, para asegurar que el render se haga en el área correcta.
     */
    void render(DeviceContext& deviceContext);

    /** @brief Libera recursos asociados al viewport. */
    void destroy();

public:
    D3D11_VIEWPORT m_viewport; ///< Estructura de configuración del viewport en Direct3D 11.
};
