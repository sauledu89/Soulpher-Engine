# Soulpher Engine

Motor gráfico modular desarrollado desde cero en **DirectX 11** como parte del curso de **Arquitectura de Motores Gráficos** y **Programación de Materiales** impartidas por el profesor **Roberto Charretón Kaplun**.

Implementa un pipeline gráfico de dos etapas (Forward Rendering con shadow maps + arquitectura Deferred Rendering) con carga de modelos FBX, sistema de actores ECS, cámara orbital interactiva, sistema de materiales PBR e interfaz de usuario con **Dear ImGui**.

<img width="1179" height="992" alt="image" src="https://github.com/user-attachments/assets/0e4a3658-3327-4eee-a05a-296b82831d47" />

---

## Características implementadas y funcionales

### Subsistemas base
- **Ventana Win32** personalizada con clase propia (`Window`).
- **Device & DeviceContext** — creación y administración completa de recursos DirectX 11.
- **SwapChain** sin MSAA, configurable por ventana.
- **Render Target View** y **Depth Stencil View** para control de color y profundidad.
- **Viewport** adaptable al tamaño de la ventana.
- **ShaderProgram** — compilación de Vertex y Pixel shaders desde archivos `.fx` / `.hlsl` en tiempo de ejecución.
- **Buffers constantes** estructurados en tres niveles:
  - `CBPerFrame` (b0): cámara, luz principal, arrays multi-luz (hasta 8), `LightViewProjection` para shadow maps.
  - `CBPerObject` (b1): matriz World por actor.
  - `CBPerMaterial` (b2): `BaseColor`, `Metallic`, `Roughness`, `AO`, `NormalScale`, `EmissiveStrength`, `AlphaCutoff` (64 bytes, listo para PBR).

### Rendering
- **Forward Renderer** con iluminación Lambert difuso + Blinn-Phong especular.
- **Shadow Maps** ortográficos (2048×2048, formato `R24G8_TYPELESS`) con pasada depth-only desde el punto de vista de la luz.
- **PCF 3×3** (Percentage Closer Filtering) para bordes de sombra suavizados.
- **Interfaz `ISceneRenderer`** — abstracción que permite intercambiar el renderer activo (Forward / Deferred) sin modificar `BaseApp`.
- **Deferred Renderer** (arquitectura completa, shaders HLSL pendientes de clase):
  - G-buffer con 4 render targets simultáneos: Albedo+Metallic, Normal+Roughness, WorldPos+AO, Emissive+Alpha.
  - Lighting pass con quad de pantalla completa.
  - Pasada de skybox y transparentes en forward sobre el resultado deferred.
- **Sistema de materiales**: `Material` (definición) + `MaterialInstance` (bind por draw call, limpieza de slots 0–5).
- **`RenderScene`** — contenedor de frame con listas separadas de objetos opacos, transparentes, luces y skybox.

### Carga de recursos
- **Sistema de mallas GPU** (`Mesh`, `Submesh`) con vertex buffer, index buffer y `localTransform` por submalla.
- **`MeshComponent`** — representación CPU de la malla antes de subir a GPU.
- **Cargador de modelos FBX** (`ModelLoader`) con:
  - Triangulación automática de la escena.
  - Aplicación de transform global por nodo.
  - Generación automática de tangentes y bitangentes para normal mapping.
  - Corrección de coordenadas UV (eByControlPoint / eByPolygonVertex).
  - Conversión automática de unidades a metros.
- **Texturas** con soporte para DDS, PNG y JPG.
- **Captura de pantalla** a PNG (`Screenshot`).

### ECS — Entity-Component-System
- **`Actor`** como contenedor de componentes: `Transform`, `MeshComponent`, `LightComponent`.
- **`Transform`** — posición, rotación y escala con actualización de matriz World.
- Flags de shadow por actor: `castShadow`, `receiveShadow`.

### Utilidades de editor
- **`Camera`** FPS con basis vectors explícitos (right/up/look), `updateViewMatrix()` con re-ortogonalización Gram-Schmidt y `setLens()` para proyección perspectiva.
- **`EditorViewportPass`** — render target offscreen que habilita post-procesado y viewport dentro de ImGui.
- **`LayoutBuilder`** — fluent builder para construir `D3D11_INPUT_ELEMENT_DESC[]` de forma declarativa.
- **Cámara orbital interactiva** en `BaseApp`:
  - Botón derecho del mouse: órbita (yaw/pitch).
  - Rueda del mouse: zoom.
  - Botón central del mouse: pan.
- **Interfaz de usuario** con **Dear ImGui** (rama docking): inspector de actores, outliner de escena y panel de control de luz en tiempo real.

### Post-build
- Copia automática de `libfbxsdk.dll` al directorio de salida al compilar.

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
│   ├── Prerequisites.h              # Includes globales, macros, SimpleVertex, EU::Vector3/2
│   ├── BaseApp.h                    # Clase principal: ciclo init / update / render / destroy
│   ├── Window.h                     # Gestión de ventana Win32
│   ├── Device.h                     # ID3D11Device wrapper
│   ├── DeviceContext.h              # ID3D11DeviceContext wrapper
│   ├── SwapChain.h                  # IDXGISwapChain wrapper
│   ├── Texture.h                    # ID3D11Texture2D + SRV wrapper
│   ├── RenderTargetView.h           # ID3D11RenderTargetView wrapper
│   ├── DepthStencilView.h           # ID3D11DepthStencilView wrapper
│   ├── DepthStencilState.h          # ID3D11DepthStencilState wrapper
│   ├── Viewport.h                   # D3D11_VIEWPORT wrapper
│   ├── ShaderProgram.h              # Compilación VS + PS + InputLayout
│   ├── Buffer.h                     # Vertex, Index y Constant buffers
│   ├── InputLayout.h                # ID3D11InputLayout wrapper
│   ├── SamplerState.h               # ID3D11SamplerState wrapper
│   ├── BlendState.h                 # ID3D11BlendState wrapper
│   ├── Rasterizer.h                 # ID3D11RasterizerState (API heredada)
│   ├── RasterizerState.h            # ID3D11RasterizerState (API nueva, dos overloads)
│   ├── MeshComponent.h              # Malla en CPU (SimpleVertex + índices)
│   ├── ModelLoader.h                # Importador FBX SDK con generación de tangentes
│   ├── UserInterface.h              # Dear ImGui: inspector, outliner, panel de luz
│   ├── Rendering/
│   │   ├── ISceneRenderer.h         # Interfaz abstracta Forward/Deferred
│   │   ├── ForwardRenderer.h        # Shadow pass + bind/unbind shadow map
│   │   ├── DeferredRenderer.h       # G-buffer 4-MRT + lighting pass + transparentes
│   │   ├── Material.h               # Definición de material (shader + rasterizer)
│   │   ├── MaterialInstance.h       # Bind de material por draw call
│   │   ├── Mesh.h                   # Submesh GPU con localTransform
│   │   ├── RenderScene.h            # Contenedor de frame: opacos, transparentes, luces, skybox
│   │   └── RenderTypes.h            # CBPerFrame, CBPerObject, CBPerMaterial, kMaxSceneLights=8
│   ├── ECS/
│   │   ├── Actor.h                  # Entidad con mesh + textura + flags de sombra
│   │   ├── Entity.h                 # Clase base de entidad
│   │   ├── Component.h              # Clase base de componente
│   │   ├── Transform.h              # Posición, rotación y escala → World matrix
│   │   └── LightComponent.h         # Datos de luz como componente
│   └── EngineUtilities/
│       └── Utilities/
│           ├── Camera.h             # Cámara FPS con basis vectors y Gram-Schmidt
│           ├── EditorViewportPass.h # Render target offscreen para el editor
│           ├── LayoutBuilder.h      # Fluent builder para D3D11_INPUT_ELEMENT_DESC
│           └── Skybox.h             # Stub de skybox (implementación pendiente)
├── src/
│   ├── BaseApp.cpp
│   ├── ECS/
│   │   ├── Actor.cpp
│   │   └── Transform.cpp
│   ├── Rendering/
│   │   ├── ForwardRenderer.cpp
│   │   ├── DeferredRenderer.cpp
│   │   ├── MaterialInstance.cpp
│   │   ├── Mesh.cpp
│   │   └── RenderScene.cpp
│   └── EditorViewportPass.cpp
├── Soulpher-Engine.cpp              # Punto de entrada Win32 (wWinMain + WndProc)
├── Soulpher-Engine.fx               # VS + PS (Lambert + Blinn-Phong + PCF) + ShadowPS
├── ImGui/                           # Dear ImGui rama docking (1.83 WIP)
├── ModelsFBX/                       # Assets de modelos y texturas (ver sección de assets)
└── bin/
    └── x64/                         # Ejecutable y DLLs de runtime
```

---

## Ubicación de assets

El directorio de trabajo al ejecutarse desde Visual Studio es la **raíz del proyecto** (donde está el `.vcxproj`). Los assets deben colocarse así:

```
Soulpher-Engine/                     <- directorio de trabajo
  Soulpher-Engine.fx                 <- shader principal
  ModelsFBX/
    piedra.jpg                       <- textura del suelo
    martis-ashura-king/
      Martis/
        hero_asura.fbx               <- modelo 3D principal
        axl_D.png                    <- textura difusa
    kirby/
      KirbyTest.fbx                  <- modelo 3D secundario
      baking.png                     <- textura del modelo
  bin/
    x64/
      Soulpher-Engine_d.exe
      libfbxsdk.dll                  <- copiada automáticamente por post-build
```

> **Nota sobre exportación FBX:** exportar desde Blender con **Apply Scale** (`Ctrl+A → Apply → Scale`) antes de exportar para que el engine reciba el modelo en escala 1:1 sin corrección manual.

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
- Dos modelos FBX cargados (Martis Ashura King y Kirby) con texturas aplicadas.
- Plano de suelo con textura `piedra.jpg` en tiling 6×6 recibiendo sombras.
- Sombras suaves con PCF 3×3 proyectadas por los modelos sobre el suelo.
- Cámara orbital controlable con el mouse (orbitar, zoom, pan).
- Panel **ImGui** con inspector de actores, outliner de escena y control de dirección/color de luz en tiempo real.

---

## Créditos y librerías utilizadas

- **Microsoft DirectX 11 / DirectX SDK (June 2010)**
- **Dear ImGui** — rama docking 1.83 WIP (interfaz de usuario)
- **Autodesk FBX SDK 2020.3.7** — importación de modelos
- **stb_image** — carga de texturas PNG y JPG
- **C++17**
