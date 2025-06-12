Soulpher Engine

Bienvenido al repositorio oficial del Soulpher Engine, un motor gráfico modular desarrollado desde cero en DirectX 11 como parte del curso de Arquitectura de Motores Gráficos impartida por el profesor Roberto Charretón Kaplun. Este proyecto tiene como objetivo comprender e implementar los componentes esenciales de un pipeline gráfico moderno.

Características principales:

- Inicialización de ventana Win32 personalizada

- Dispositivo y contexto DirectX 11 (Device, DeviceContext)

- Sistema de SwapChain para buffer de intercambio

- Render Target y Depth Stencil View configurables

- Viewport adaptable al tamaño de ventana

- Compilación de shaders desde archivos .fx

- Renderizado de cubo 3D con textura

- Proyección de sombra plana usando blending y stencil buffer

- Sistema modular y escalable por clases independientes

Estructura del proyecto

SoulpherEngine/
├── BaseApp.*                 # Clase base del motor
├── Window.*                  # Manejo de ventana Win32
├── Device.* / DeviceContext.*# Inicialización de DirectX
├── SwapChain.*               # Control del intercambio de buffers
├── Texture.*                 # Manejo de texturas y recursos
├── RenderTargetView.*        # Target de renderizado principal
├── DepthStencilView.*        # Buffer de profundidad y stencil
├── Viewport.*                # Área visible de dibujo
├── Soulpher-Engine.cpp       # Punto de entrada principal
├── Shaders/*.fx              # Archivos de vertex y pixel shaders

Requisitos

- Visual Studio 2022 o superior

- SDK de Windows 10 o superior

- DirectX 11 (D3D11)

Librerías linkeadas:

- d3d11.lib

- d3dcompiler.lib

- dxguid.lib

- winmm.lib

Instrucciones de compilación

- Clona el repositorio en tu equipo local.

- Abre el proyecto con Visual Studio.

- Asegúrate de tener los archivos .fx y seafloor.dds en la carpeta del ejecutable (/Debug/ o /Release/).

- Compila el proyecto (Ctrl + Shift + B).

- Ejecuta el programa. Deberías ver una ventana con un cubo animado sobre un plano con su sombra proyectada.

Vista previa

![image](https://github.com/user-attachments/assets/0f98383a-9fe3-47f9-9093-b6ff8741288d)


Autor

Saúl Eduardo González Vargassgonzalez37079@ucq.edu.mx

