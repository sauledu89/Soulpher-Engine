# Soulpher Engine

Motor gráfico modular desarrollado desde cero en **DirectX 11** como parte del curso de **Arquitectura de Motores Gráficos** y **Programación de Materiales** impartidas por el profesor **Roberto Charretón Kaplun**.

Implementa un pipeline gráfico estructurado con carga de modelos FBX, sistema de actores ECS, cámara orbital interactiva e interfaz de usuario con **Dear ImGui**.

<img width="1179" height="992" alt="image" src="https://github.com/user-attachments/assets/0e4a3658-3327-4eee-a05a-296b82831d47" />

---

## Características implementadas y funcionales

- **Ventana Win32** personalizada con clase propia (`Window`).
- **Device & DeviceContext** — creación y administración completa de recursos DirectX 11.
- **SwapChain** sin MSAA, configurable por ventana.
- **Render Target View** y **Depth Stencil View** para control de color y profundidad.
- **Viewport** adaptable al tamaño de la ventana.
- **ShaderProgram** — compilación de Vertex y Pixel shaders desde archivos `.fx`.
- **Buffers constantes de cámara** (`CBNeverChanges`, `CBChangeOnResize`, `CBChangesEveryFrame`).
- **Sistema de mallas** (`MeshComponent`) y **texturas** (`Texture`) con soporte para DDS, PNG y JPG.
- **Cargador de modelos FBX** (`ModelLoader`) con:
  - Triangulación automática de la escena (quads y n-gons → triángulos).
  - Aplicación de transform global por nodo (ensambla piezas en posición correcta).
  - Corrección de coordenadas UV (`eByControlPoint` y `eByPolygonVertex`, eDirect e eIndexToDirect).
  - Conversión automática de unidades de la escena a metros.
- **Sistema ECS básico** — `Actor`, `Transform`, `Component`, `MeshComponent`.
- **Plano de referencia** con geometría manual, tiling UV 6×6 y textura propia.
- **Cámara orbital interactiva**:
  - Botón derecho del mouse: órbita (yaw/pitch).
  - Rueda del mouse: zoom.
  - Botón central del mouse: pan.
- **Interfaz de usuario** con **Dear ImGui** (rama docking): inspector de actores y outliner de escena.
- **Post-build automático** — copia `libfbxsdk.dll` al directorio de salida al compilar.

---

## Requisitos

- **Visual Studio 2022** (toolset v143)
- **Windows SDK 10.0.26100.0** o superior
- **DirectX SDK (June 2010)** — para `xnamath.h`, `d3dx11`, `d3dx9`
- **FBX SDK 2020.3.7** instalado en `C:\Program Files\Autodesk\FBX\FBX SDK\2020.3.7\`
- Plataforma de compilación: **x64** (no existe versión x86 del FBX SDK 2020)

### Librerías linkeadas

| Debug | Release/Profile |
|---|---|
| d3d11.lib | d3d11.lib |
| d3dcompiler.lib | d3dcompiler.lib |
| libfbxsdk.lib | libfbxsdk.lib |
| libxml2.lib | libxml2.lib |
| zlib.lib | zlib.lib |
| d3dx11d.lib | d3dx11.lib |
| d3dx9d.lib | d3dx9.lib |
| dxerr.lib | dxerr.lib |
| dxguid.lib | dxguid.lib |
| winmm.lib | winmm.lib |
| comctl32.lib | comctl32.lib |

---

## Estructura del proyecto

```
Soulpher-Engine/
├── include/
│   ├── Prerequisites.h         # Includes globales, macros y structs compartidos
│   ├── BaseApp.h / .cpp        # Clase principal: ciclo init / update / render / destroy
│   ├── Window.h / .cpp         # Gestión de ventana Win32
│   ├── Device.h / .cpp         # Creación de recursos DirectX 11
│   ├── DeviceContext.h / .cpp  # Comandos de render y estado del pipeline
│   ├── SwapChain.h / .cpp      # Intercambio de buffers
│   ├── Texture.h / .cpp        # Carga de texturas (DDS, PNG, JPG via stb_image)
│   ├── RenderTargetView.h/.cpp # Target de renderizado
│   ├── DepthStencilView.h/.cpp # Buffer de profundidad y stencil
│   ├── DepthStencilState.h/.cpp
│   ├── Viewport.h / .cpp       # Área visible de dibujo
│   ├── ShaderProgram.h / .cpp  # Compilación y binding de shaders .fx
│   ├── Buffer.h / .cpp         # Vertex, index y constant buffers
│   ├── InputLayout.h / .cpp    # Descripción del formato de vértice
│   ├── SamplerState.h / .cpp   # Estado de muestreo de texturas
│   ├── BlendState.h / .cpp     # Estado de mezcla de color
│   ├── Rasterizer.h / .cpp     # Estado del rasterizador
│   ├── MeshComponent.h         # Representación de malla 3D
│   ├── ModelLoader.h / .cpp    # Importador de modelos FBX
│   ├── UserInterface.h / .cpp  # Integración con Dear ImGui (docking)
│   └── ECS/
│       ├── Actor.h / .cpp      # Entidad de escena con componentes
│       ├── Entity.h            # Clase base de entidad
│       ├── Component.h         # Clase base de componente
│       └── Transform.h / .cpp  # Posición, rotación y escala
├── Soulpher-Engine.cpp         # Punto de entrada Win32
├── Soulpher-Engine.fx          # Vertex y Pixel shader principal
├── ImGui/                      # Dear ImGui rama docking (1.83 WIP)
├── ModelsFBX/                  # Assets de modelos y texturas (ver sección de assets)
└── bin/
    └── x64/                    # Ejecutable y DLLs de runtime
```

---

## Ubicación de assets

El directorio de trabajo del engine al ejecutarse desde Visual Studio es la **raíz del proyecto** (donde está el `.vcxproj`). Los assets deben colocarse así:

```
Soulpher-Engine/               <- directorio de trabajo
  Soulpher-Engine.fx           <- shader principal
  ModelsFBX/
    piedra.jpg                 <- textura del suelo
    kirby/
      KirbyTest.fbx            <- modelo 3D
      baking.png               <- textura del modelo
  bin/
    x64/
      Soulpher-Engine_d.exe
      libfbxsdk.dll            <- copiada automáticamente por post-build
```

> **Nota sobre exportación FBX:** exportar desde Blender con **Apply Scale** (`Ctrl+A → Apply → Scale`) antes de exportar para que el engine reciba el modelo en escala 1:1 sin necesitar corrección manual.

---

## Instrucciones de compilación

1. Clona el repositorio.
2. Instala el **FBX SDK 2020.3.7** en la ruta por defecto de Autodesk.
3. Abre `Soulpher-Engine_2010.sln` con Visual Studio 2022.
4. Selecciona configuración **Debug | x64** (recomendada).
5. Compila (`Ctrl + Shift + B`). El post-build copia `libfbxsdk.dll` automáticamente.
6. Coloca los assets en `ModelsFBX\` según la estructura indicada arriba.
7. Ejecuta desde Visual Studio (`F5`).

Al iniciar deberías ver:
- Modelo 3D cargado desde FBX con textura aplicada.
- Plano de suelo con textura `piedra.jpg` en tiling 6×6.
- Cámara orbital controlable con el mouse.
- Panel de **ImGui** con inspector y outliner activos.

---

## Créditos y librerías utilizadas

- **Microsoft DirectX 11 / DirectX SDK (June 2010)**
- **Dear ImGui** — rama docking 1.83 WIP (interfaz de usuario)
- **Autodesk FBX SDK 2020.3.7** — importación de modelos
- **stb_image** — carga de texturas PNG y JPG
- **C++17**
