#pragma once
#include "Prerequisites.h"

/**
 * @file Viewport.h
 * @brief Define el área de renderizado (viewport) en DirectX 11.
 *
 * El viewport especifica la región del render target (pantalla o textura) donde se proyectarán
 * los píxeles procesados. Es crucial para adaptar resoluciones, implementar sistemas como
 * pantalla dividida, herramientas de depuración, cámaras múltiples, entre otros.
 */

 //--------------------------------------------------------------------------------------
 // Forward declarations para evitar dependencias circulares innecesarias
 //--------------------------------------------------------------------------------------
class Window;
class DeviceContext;

/**
 * @class Viewport
 * @brief Representa un D3D11_VIEWPORT configurable en tiempo de ejecución.
 *
 * Esta clase facilita la creación y activación de un viewport para el pipeline gráfico.
 * Permite definir sus dimensiones desde una ventana o de forma manual, y lo activa
 * en el contexto gráfico para ser utilizado durante el renderizado.
 */
class Viewport {
public:
    /**
     * @brief Constructor por defecto.
     */
    Viewport() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~Viewport() = default;

    /**
     * @brief Inicializa el viewport con base en las dimensiones de una ventana.
     *
     * Esta función extrae el tamaño de la ventana principal para configurar
     * correctamente la región de dibujo.
     *
     * @param window Objeto `Window` que representa la ventana principal de la app.
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(const Window& window);

    /**
     * @brief Inicializa el viewport manualmente con valores explícitos.
     *
     * Útil para configuraciones personalizadas como splitscreen, áreas de UI o
     * renderizado a texturas fuera de pantalla.
     *
     * @param width Ancho del viewport.
     * @param height Alto del viewport.
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(unsigned int width, unsigned int height);

    /**
     * @brief Actualización por cuadro. Actualmente no implementado.
     */
    void update();

    /**
     * @brief Activa este viewport en el pipeline gráfico.
     *
     * Llama internamente a `RSSetViewports()` para aplicar la configuración actual.
     *
     * @param deviceContext Contexto de dispositivo sobre el que se aplicará.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera cualquier recurso asociado al viewport.
     *
     * Actualmente no hace nada, ya que `D3D11_VIEWPORT` no requiere destrucción manual.
     */
    void destroy();

public:
    /**
     * @brief Estructura `D3D11_VIEWPORT` que define la región visible de render.
     *
     * Contiene dimensiones, profundidad mínima y máxima, y posición dentro del render target.
     */
    D3D11_VIEWPORT m_viewport;
};
