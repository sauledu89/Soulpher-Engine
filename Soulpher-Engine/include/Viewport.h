#pragma once
#include "Prerequisites.h"

/**
 * @file Viewport.h
 * @brief Define el �rea de renderizado (viewport) en DirectX 11.
 *
 * El viewport especifica la regi�n del render target (pantalla o textura) donde se proyectar�n
 * los p�xeles procesados. Es crucial para adaptar resoluciones, implementar sistemas como
 * pantalla dividida, herramientas de depuraci�n, c�maras m�ltiples, entre otros.
 */

 //--------------------------------------------------------------------------------------
 // Forward declarations para evitar dependencias circulares innecesarias
 //--------------------------------------------------------------------------------------
class Window;
class DeviceContext;

/**
 * @class Viewport
 * @brief Representa un D3D11_VIEWPORT configurable en tiempo de ejecuci�n.
 *
 * Esta clase facilita la creaci�n y activaci�n de un viewport para el pipeline gr�fico.
 * Permite definir sus dimensiones desde una ventana o de forma manual, y lo activa
 * en el contexto gr�fico para ser utilizado durante el renderizado.
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
     * Esta funci�n extrae el tama�o de la ventana principal para configurar
     * correctamente la regi�n de dibujo.
     *
     * @param window Objeto `Window` que representa la ventana principal de la app.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(const Window& window);

    /**
     * @brief Inicializa el viewport manualmente con valores expl�citos.
     *
     * �til para configuraciones personalizadas como splitscreen, �reas de UI o
     * renderizado a texturas fuera de pantalla.
     *
     * @param width Ancho del viewport.
     * @param height Alto del viewport.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(unsigned int width, unsigned int height);

    /**
     * @brief Actualizaci�n por cuadro. Actualmente no implementado.
     */
    void update();

    /**
     * @brief Activa este viewport en el pipeline gr�fico.
     *
     * Llama internamente a `RSSetViewports()` para aplicar la configuraci�n actual.
     *
     * @param deviceContext Contexto de dispositivo sobre el que se aplicar�.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Libera cualquier recurso asociado al viewport.
     *
     * Actualmente no hace nada, ya que `D3D11_VIEWPORT` no requiere destrucci�n manual.
     */
    void destroy();

public:
    /**
     * @brief Estructura `D3D11_VIEWPORT` que define la regi�n visible de render.
     *
     * Contiene dimensiones, profundidad m�nima y m�xima, y posici�n dentro del render target.
     */
    D3D11_VIEWPORT m_viewport;
};
