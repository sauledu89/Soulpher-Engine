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

SoulpherEngine/
├── BaseApp.* # Clase principal del motor y ciclo de vida
├── Soulpher-Engine.cpp # Punto de entrada Win32 y reenvío de mensajes
├── Window.* # Gestión de ventana Win32
├── Device.* # Inicialización y creación de recursos DirectX
├── DeviceContext.* # Operaciones sobre el contexto de render
├── SwapChain.* # Control del intercambio de buffers
├── Texture.* # Carga y gestión de texturas
├── RenderTargetView.* # Target de renderizado principal
├── DepthStencilView.* # Buffer de profundidad y stencil
├── Viewport.* # Área visible de dibujo
├── ShaderProgram.* # Compilación y uso de shaders
├── Buffer.* # Buffers constantes y de vértices/índices
├── MeshComponent.* # Representación de mallas 3D
├── ModelLoader.* # Importador de modelos FBX
├── ECS/
│ ├── Actor.* # Entidad de la escena
│ ├── Transform.* # Posición, rotación y escala
├── UserInterface.* # Integración con Dear ImGui
├── Shaders/*.fx # Vertex y Pixel shaders
└── Assets/ # Modelos, texturas y recursos


---

## 📌 Requisitos

- **Visual Studio 2022** o superior  
- **SDK de Windows 10** o superior  
- **DirectX 11 (D3D11)**  

**Librerías linkeadas:**
- `d3d11.lib`
- `d3dcompiler.lib`
- `dxguid.lib`
- `winmm.lib`

---

## 🛠 Instrucciones de compilación

1. Clona el repositorio en tu equipo local.
2. Abre la solución del proyecto con Visual Studio.
3. Verifica que los assets (`.fx`, modelos FBX y texturas) se encuentren en la carpeta `bin/` junto al ejecutable.
4. Compila el proyecto (`Ctrl + Shift + B`).
5. Ejecuta el programa.  
   Deberías ver un **plano con textura**, un **modelo FBX cargado** y la interfaz **ImGui** operativa.

---

## 📸 Vista previa

*(Aquí puedes insertar capturas de pantalla del motor en ejecución)*

---

## 📚 Créditos y librerías utilizadas

- **Microsoft DirectX 11 SDK**
- **Dear ImGui** (interfaz de usuario)
- **FBX SDK** (carga de modelos)
- **C++17** (estándar del lenguaje)



![Soulpher-Engine-2](https://github.com/user-attachments/assets/56045229-0796-4791-a405-babeee5f21dc)
