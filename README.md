# Soulpher Engine

Bienvenido al repositorio oficial de **Soulpher Engine**, un motor gráfico modular desarrollado desde cero en **DirectX 11** como parte del curso de **Arquitectura de Motores Gráficos** impartido por el profesor **Roberto Charretón Kaplun**.  
Esta versión final implementa un pipeline gráfico más completo y estructurado, incorporando manejo avanzado de recursos, renderizado modular y soporte para interfaz de usuario con **Dear ImGui**.

---

## 🚀 Características principales

- **Inicialización de ventana Win32** personalizada con clase propia (`Window`).
- **Device & DeviceContext** para la creación y administración de recursos DirectX 11.
- **SwapChain** configurable para el intercambio de buffers.
- **Render Target View** y **Depth Stencil View** para control de color, profundidad y stencil.
- **Viewport** adaptable al tamaño de la ventana.
- **Sistema de Shaders** (`ShaderProgram`) con compilación desde archivos `.fx`.
- **Buffers constantes de cámara** (`CBNeverChanges`, `CBChangeOnResize`).
- **Sistema de malla y texturas** (`MeshComponent`, `Texture`).
- **Cargador de modelos FBX** (`ModelLoader`) para importar assets externos.
- **Sistema ECS básico** con actores (`Actor`, `Transform`).
- **Plano de referencia** con textura y un modelo FBX 3D (Martis Ashura King) cargado dinámicamente.
- **Interfaz de usuario** con **Dear ImGui**, incluyendo soporte para múltiples viewports.
- **Arquitectura modular** separada por clases independientes.

---

## 📂 Estructura del proyecto



![Soulpher-Engine-2](https://github.com/user-attachments/assets/56045229-0796-4791-a405-babeee5f21dc)
