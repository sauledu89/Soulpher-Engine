# Soulpher Engine

Motor gráfico modular desarrollado desde cero en **DirectX 11** como parte del curso de **Arquitectura de Motores Gráficos** y **Programación de Materiales** impartidas por el profesor **Roberto Charretón Kaplun**.

Implementa un **Deferred Renderer** completo (G-Buffer de 4 render targets + lighting pass multi-luz + shadow maps + skybox) con carga de modelos FBX, sistema de actores ECS, Light Actors editables, gizmos de transformación en el viewport, cámara orbital interactiva, sistema de materiales PBR e interfaz de usuario con **Dear ImGui**.

Estado del proyecto en el final del parcial 2 : 

<img width="1197" height="985" alt="SOULPHER-ENGINE-PARCIAL2" src="https://github.com/user-attachments/assets/1edbe849-d33c-40d7-ab2b-932caec18d91" />

---

## Parcial 2 — qué cambió desde Parcial 1

El **Parcial 1** dejó la arquitectura del Deferred Renderer construida pero sin shaders (el motor seguía renderizando en Forward). En el **Parcial 2** se completó el pipeline deferred y se construyeron encima las herramientas de editor que le dan forma de motor "usable" en vez de solo "funcional":

- **Deferred Renderer con shaders reales** (`DeferredGBuffer.hlsl` + `DeferredLighting.hlsl` + `ShadowMap.hlsl`) — ya es el único camino de render activo, `ForwardRenderer` quedó como código legado sin usar en `render()`.
- **Multi-light real**: hasta 8 luces Directional/Point/Spot simultáneas con atenuación por distancia y cono de spot, todas evaluadas en el lighting pass.
- **Light Actors**: cualquier luz es ahora un `Actor` con `LightComponent`, editable desde el Inspector, con su propio ícono wireframe en el viewport y picking por clic.
- **Gizmos de transformación** (Translate/Rotate/Scale) para mover, rotar y escalar actores directamente en el viewport.
- **Skybox** equirectangular (DDS) con un parser de DDS propio, escrito para evitar un crash del loader legado de D3DX11 con BC7 + DX10 header.
- **`LogManager`** centraliza todos los logs del motor en un buffer que la consola de la UI lee en tiempo real (antes solo iban a `OutputDebugString`).
- **Editor UI**: tema visual propio (NES), paneles redimensionables/movibles, debug de los 4 canales del G-Buffer, outliner con duplicar/borrar/crear actores.

> 📘 **Nota GameDev:** que un pipeline "funcione" en el sentido de compilar y no crashear (Parcial 1) es un hito distinto a que sea *usable* para producir contenido (Parcial 2). En estudios reales estas dos etapas suelen dividirse entre ingeniería de render y herramientas de editor — aquí las hizo la misma persona, pero es útil notar que son disciplinas distintas: la primera pregunta es "¿es correcto matemáticamente?", la segunda es "¿puede un diseñador usar esto sin leer el código fuente?".

---

## Parcial 3 — Material Editor

Herramienta complementaria in-engine (ImGui) para crear y editar materiales PBR **en vivo**
— parámetros y texturas, sin recompilar — inspirada en el Material Instance Editor de Unreal
Engine, y asignarlos a los objetos de la escena demo sin tocar el wiring hardcodeado del
render loop.

- **Crear materiales desde cero**: nombre, dominio (Opaque / Transparent / Masked).
- **Editar en vivo** los 7 parámetros de `CBPerMaterial` (BaseColor, Metallic, Roughness,
  AO, NormalScale, EmissiveStrength, AlphaCutoff), con efecto inmediato en el viewport.
- **Cargar/quitar texturas** por slot (Albedo/Normal/Metallic/Roughness/AO/Emissive) con
  thumbnail en vivo.
- **Asignar** cualquier material (built-in o creado en el editor) a cualquiera de los 5
  "huecos" de render de la demo (Kirby, Plano, SciFiToad Body/Glass/Head), vía un patrón de
  indirección de punteros equivalente a los *Material Slots* de Unreal.
- **Borrar** materiales creados en el editor, bloqueado si el material sigue asignado a un
  "hueco" de render.

Documentación técnica completa (investigación, arquitectura, decisiones de diseño,
limitaciones como caminos de escalabilidad y pruebas de funcionamiento):
**[`docs/MaterialEditor.md`](docs/MaterialEditor.md)**.

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
- **Deferred Renderer** — pipeline activo por defecto, con shaders HLSL completos:
  - `DeferredGBuffer.hlsl` — geometry pass a 4 render targets simultáneos: Albedo+Metallic, Normal+Roughness, WorldPos+AO, Emissive+Alpha.
  - `DeferredLighting.hlsl` — lighting pass de pantalla completa que itera hasta 8 luces (`LightCount`), con atenuación real por distancia para Point/Spot y cono de falloff para Spot.
  - `ShadowMap.hlsl` — shadow depth pass ortográfico (2048×2048, `R24G8_TYPELESS`) desde el punto de vista de la luz principal, con **PCF 3×3** para bordes suavizados.
  - Pasada de **Skybox** y de transparentes en forward sobre el resultado ya iluminado.
- **`ForwardRenderer`** — pipeline original (Lambert + Blinn-Phong + shadow map, un único light), conservado en el código pero ya no es el camino activo de `render()`; útil como referencia y para el shadow-matrix legado.
- **Interfaz `ISceneRenderer`** — abstracción que permite intercambiar el renderer activo (Forward / Deferred) sin modificar `BaseApp`.
- **Sistema de materiales**: `Material` (definición) + `MaterialInstance` (bind por draw call, limpieza de slots 0–5).
- **`RenderScene`** — contenedor de frame con listas separadas de objetos opacos, transparentes, luces y skybox.

> 📘 **Nota GameDev:** el motivo técnico de mover todo a deferred es que el costo de iluminación deja de escalar con el número de *triángulos* y pasa a escalar con el número de *píxeles visibles* — con 8 luces activas, forward tendría que re-evaluar el shader de cada luz por cada fragmento de cada objeto que se solape en pantalla (overdraw), mientras que deferred lo calcula una sola vez por píxel final leyendo el G-Buffer. Es la misma razón por la que Unreal Engine usa deferred como su renderer por defecto (con Forward+ como alternativa para VR/transparencias, donde el overdraw es bajo y el ancho de banda del G-Buffer es más caro que las luces).

### Skybox
- Carga de un **panorama equirectangular** (`Assets/skybox.dds`, formato BC7 + DX10 header) — no un cubemap de 6 caras.
- Parser de DDS **escrito a mano** (`Texture::LoadDdsTexture`) que llama directo a `CreateTexture2D`/`CreateShaderResourceView`, evitando el loader legado `D3DX11CreateShaderResourceViewFromFileA` (crashea con la combinación BC7 + DX10 header en este SDK de 2010).
- `Skybox.hlsl` mapea la dirección de vista a UV esférica con `atan2`/`asin`, y fuerza `z = w` en el vertex shader para que el skybox siempre quede al fondo del depth buffer sin necesitar su propia geometría de cubo real.
- Falla de forma no fatal: si el DDS no carga, la escena sigue renderizando sin fondo.

> 📘 **Nota GameDev:** un panorama equirectangular (2:1, como un mapa mundi) es más barato de generar y editar en herramientas 2D que un cubemap de 6 caras, pero es más caro de muestrear en tiempo real por el `atan2`/`asin` trigonométrico frente al simple `TextureCube::Sample` de un cubemap. Motores como Unreal aceptan HDRIs equirectangulares como fuente pero los convierten a cubemap en un paso de cocción (bake) antes de usarlos en runtime — aquí se muestrea directo cada frame porque el motor no tiene pipeline de asset baking.

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
- **`LightComponent`** — convierte cualquier `Actor` en un **Light Actor** (Directional/Point/Spot). No guarda posición/dirección propia: las combina con el `Transform` del Actor dueño cada frame en `resolve()`, así que mover o rotar el Actor mueve la luz automáticamente, sin pasos manuales de sincronización.

> 📘 **Nota GameDev:** convertir una luz en "un Actor más" (en vez de un objeto especial fuera del ECS) es el mismo patrón que usa Unreal Engine 5 (`APointLight`/`ASpotLight`/`ADirectionalLight`, cada uno envolviendo un `ULightComponent`). La ventaja es reutilizar todo el sistema existente — Transform, Inspector, gizmos, outliner, duplicar/borrar — sin escribir un camino especial: una linterna que sigue al jugador es literalmente el mismo tipo de objeto que una lámpara estática, solo que su Transform cambia cada frame.

### Herramientas de editor (viewport)
- **Gizmos de transformación** (`GizmoRenderer`) para el actor seleccionado, con geometría procedural (conos + cilindros) y color de "affordance" (resalta al pasar el mouse / al arrastrar):
  - **Translate** (`T`) — mueve sobre los 3 ejes del mundo.
  - **Rotate** (`E`) — rota sobre los 3 ejes.
  - **Scale** (`R`) — escala por eje, más un handle central para **escala uniforme** en los 3 ejes a la vez.
  - Tamaño constante en pantalla (`screenScale`, independiente de la distancia a cámara) y ejes fijos en espacio del mundo (no local), para que el gizmo se comporte de forma predecible sin importar la rotación del objeto.
- **Picking unificado**: un solo ray-test por clic cubre tanto los ejes del gizmo como el AABB de los actores con malla y el punto de los íconos de luz — así seleccionar, arrastrar un eje o hacer clic en un ícono de luz usan el mismo pipeline de entrada.
- **`LightGizmoRenderer`** — íconos wireframe (flecha=Directional, esfera=Point, cono=Spot) para visualizar y seleccionar Light Actors en el viewport, ya que no tienen malla propia.
- **`EditorViewportPass`** — render target offscreen que permite mostrar la escena dentro de una ventana ImGui (con post-procesado futuro) en vez de directo al back buffer.
- **Redimensionado en vivo**: la ventana, el swap chain, los G-Buffers y el aspect ratio de la cámara se recalculan cuando cambia el tamaño del viewport, sin reiniciar el motor.

> 📘 **Nota GameDev:** trabajar en espacio del mundo para los ejes del gizmo (en vez de espacio local del objeto) es una decisión de UX deliberada, no solo técnica: si rotas un objeto 45° y el gizmo rotara con él, mover "el eje rojo" dejaría de significar "mover en X del mundo" — cosa que confunde a cualquiera que edite una escena. Unreal y Unity ofrecen ambos modos (World/Local) y por defecto casi todos los editores abren en World precisamente por esto.

### Utilidades de editor
- **`Camera`** FPS con basis vectors explícitos (right/up/look), `updateViewMatrix()` con re-ortogonalización Gram-Schmidt y `setLens()` para proyección perspectiva.
- **`LayoutBuilder`** — fluent builder para construir `D3D11_INPUT_ELEMENT_DESC[]` de forma declarativa.
- **`LogManager`** — singleton central de logging (`LOG_MESSAGE`/`LOG_WARNING`/`LOG_ERROR`) que alimenta tanto la consola de la UI como el Output de Visual Studio, reemplazando el uso disperso de `OutputDebugStringW` directo.
- **Cámara orbital interactiva** en `BaseApp`:
  - Botón derecho del mouse: órbita (yaw/pitch).
  - Rueda del mouse: zoom.
  - Botón central del mouse: pan.
- **Interfaz de usuario** con **Dear ImGui** (rama docking):
  - **Tema visual propio** ("NES"): fondo gris claro, acentos rojo/naranja, pensado para alto contraste y legibilidad en sesiones largas.
  - **Inspector** con sección "Transform" (posición/rotación/escala, con botón de vínculo XYZ para editar los 3 ejes a la vez) y sección "Light" condicional (tipo, color, intensidad, range, ángulo de spot) para Light Actors.
  - **Outliner** con botón "+ Add Light" y menú contextual (clic derecho) para **duplicar** y **borrar** actores.
  - **Panel de debug del G-Buffer**: 4 ventanas independientes (Albedo / Normals / WorldPos / Emissive) con checkbox de mostrar/ocultar cada canal.
  - **Consola** con niveles Message/Warning/Error, filtrable, con auto-scroll.

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
│   ├── UserInterface.h              # Dear ImGui: inspector, outliner, consola, G-Buffer debug
│   ├── Rendering/
│   │   ├── ISceneRenderer.h         # Interfaz abstracta Forward/Deferred
│   │   ├── ForwardRenderer.h        # Shadow pass + bind/unbind shadow map (legado, no activo)
│   │   ├── DeferredRenderer.h       # G-buffer 4-MRT + lighting pass multi-luz + skybox + transparentes
│   │   ├── GizmoRenderer.h          # Gizmos de transformación (Translate/Rotate/Scale)
│   │   ├── LightGizmoRenderer.h     # Íconos wireframe de Light Actors (flecha/esfera/cono)
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
│   │   └── LightComponent.h         # Convierte un Actor en Light Actor (Directional/Point/Spot)
│   └── EngineUtilities/
│       └── Utilities/
│           ├── Camera.h             # Cámara FPS con basis vectors y Gram-Schmidt
│           ├── EditorViewportPass.h # Render target offscreen para el editor
│           ├── LayoutBuilder.h      # Fluent builder para D3D11_INPUT_ELEMENT_DESC
│           ├── LogManager.h         # Logging centralizado (LOG_MESSAGE/WARNING/ERROR)
│           └── Skybox.h             # Skybox equirectangular (DDS, parser propio)
├── src/
│   ├── BaseApp.cpp
│   ├── Skybox.cpp
│   ├── ECS/
│   │   ├── Actor.cpp
│   │   └── Transform.cpp
│   ├── Rendering/
│   │   ├── ForwardRenderer.cpp
│   │   ├── DeferredRenderer.cpp
│   │   ├── GizmoRenderer.cpp
│   │   ├── LightGizmoRenderer.cpp
│   │   ├── MaterialInstance.cpp
│   │   ├── Mesh.cpp
│   │   └── RenderScene.cpp
│   └── EditorViewportPass.cpp
├── Soulpher-Engine.cpp              # Punto de entrada Win32 (wWinMain + WndProc)
├── Soulpher-Engine.fx               # VS + PS (Lambert + Blinn-Phong + PCF) + ShadowPS (forward, legado)
├── DeferredGBuffer.hlsl             # Geometry pass del deferred (4 MRT)
├── DeferredLighting.hlsl            # Lighting pass del deferred (multi-luz, fullscreen quad)
├── ShadowMap.hlsl                   # Shadow depth pass (luz principal)
├── Gizmo.fx                         # Shader unlit (posición+color) para gizmos y luces
├── Skybox.hlsl                      # Sampling equirectangular (atan2/asin) para el skybox
├── ImGui/                           # Dear ImGui rama docking (1.83 WIP)
├── ModelsFBX/                       # Modelos FBX y texturas (ver sección de assets)
├── Assets/                          # Assets no-FBX (skybox.dds)
└── bin/
    └── x64/                         # Ejecutable y DLLs de runtime
```

---

## Ubicación de assets

El directorio de trabajo al ejecutarse desde Visual Studio es la **raíz del proyecto** (donde está el `.vcxproj`). Los assets deben colocarse así:

```
Soulpher-Engine/                     <- directorio de trabajo
  Soulpher-Engine.fx                 <- shader forward (legado)
  DeferredGBuffer.hlsl               <- shader geometry pass del deferred
  DeferredLighting.hlsl              <- shader lighting pass del deferred
  ShadowMap.hlsl                     <- shader shadow depth pass
  Gizmo.fx                           <- shader unlit para gizmos y luces
  Skybox.hlsl                        <- shader del skybox equirectangular
  ModelsFBX/
    piedra.jpg                       <- textura del suelo
    kirby/
      KirbyTest.fbx                  <- modelo 3D cargado por defecto
      baking.png                     <- textura del modelo
  Assets/
    skybox.dds                       <- panorama equirectangular (BC7 + DX10 header)
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
6. Coloca los assets en `ModelsFBX\` y `Assets\` según la estructura indicada arriba (el skybox es opcional: si falta, el motor sigue funcionando sin fondo).
7. Ejecuta desde Visual Studio (`F5`).

Al iniciar deberías ver:
- Modelo FBX de Kirby cargado con su textura aplicada.
- Plano de suelo con textura `piedra.jpg` en tiling 6×6 recibiendo sombras.
- Sombras suaves con PCF 3×3 proyectadas por Kirby sobre el suelo.
- Un Light Actor "Sun" (Directional) ya en la escena, editable desde el Inspector.
- Skybox equirectangular de fondo (si `Assets/skybox.dds` cargó correctamente).
- Cámara orbital controlable con el mouse (orbitar, zoom, pan).
- Panel **ImGui** (tema NES) con inspector de actores, outliner con creación/duplicado/borrado, gizmos de transformación (`T`/`E`/`R` sobre el actor seleccionado) y debug del G-Buffer.

---

Estado del proyecto en el final del parcial 1 : 

<img width="1179" height="992" alt="image" src="https://github.com/user-attachments/assets/0e4a3658-3327-4eee-a05a-296b82831d47" />

## Créditos y librerías utilizadas

- **Microsoft DirectX 11 / DirectX SDK (June 2010)**
- **Dear ImGui** — rama docking 1.83 WIP (interfaz de usuario)
- **Autodesk FBX SDK 2020.3.7** — importación de modelos
- **stb_image** — carga de texturas PNG y JPG
- **C++17**
