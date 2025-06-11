#pragma once

// =======================================
// @file Prerequisites.h
// @brief Incluye librerías esenciales y define macros, estructuras y tipos usados globalmente.
// =======================================

// ------------------------------
// Librerías estándar de C++
// ------------------------------
#include <string>     ///< Para manipulación de cadenas.
#include <sstream>    ///< Para construcción de textos con flujos.
#include <vector>     ///< Para uso de arreglos dinámicos.
#include <windows.h>  ///< Interacción con la API de Windows.
#include <xnamath.h>  ///< Operaciones matemáticas de DirectX.
#include <thread>     ///< Soporte para ejecución multihilo.

// ------------------------------
// Librerías de DirectX 11
// ------------------------------
#include <d3d11.h>       ///< Interfaces principales de Direct3D 11.
#include <d3dx11.h>      ///< Funciones auxiliares de inicialización y carga.
#include <d3dcompiler.h> ///< Compilación de shaders.
#include "Resource.h"    ///< Recursos de Win32 (íconos, cursores, etc.).

// ------------------------------
// Macros útiles para debugging y gestión de recursos
// ------------------------------

/**
 * @brief Libera un recurso COM de forma segura y lo pone en nullptr.
 *
 * @param x Recurso de tipo puntero a interfaz COM.
 *
 * @note Esta macro evita memory leaks al liberar correctamente los recursos de DirectX.
 */
#define SAFE_RELEASE(x) if(x != nullptr) x->Release(); x = nullptr;

 /**
  * @brief Macro para imprimir mensajes de creación de recursos en la consola de depuración.
  *
  * Se utiliza para verificar la correcta inicialización de recursos gráficos en desarrollo.
  *
  * @param classObj Nombre de la clase desde la que se llama.
  * @param method Nombre del método.
  * @param state Mensaje de estado o descripción.
  */
#define MESSAGE(classObj, method, state)                       \
{                                                              \
   std::wostringstream os_;                                    \
   os_ << classObj << "::" << method << " : "                  \
       << "[CREATION OF RESOURCE : " << state << "] \n";       \
   OutputDebugStringW(os_.str().c_str());                      \
}

  /**
   * @brief Macro para imprimir errores con formato en la consola de depuración.
   *
   * Captura errores en tiempo de ejecución y los muestra en Visual Studio.
   *
   * @param classObj Nombre de la clase desde la que se llama.
   * @param method Nombre del método.
   * @param errorMSG Mensaje de error específico.
   */
#define ERROR(classObj, method, errorMSG)                                       \
{                                                                               \
    try {                                                                       \
    std::wostringstream os_;                                                    \
    os_ << L"ERROR : " << classObj << L"::" << method                           \
        << L" : " << errorMSG << L"\n";                                         \
    OutputDebugStringW(os_.str().c_str());                                      \
    } catch (...) {                                                             \
    OutputDebugStringW(L"Failed to log error message.\n");                      \
    }                                                                           \
}

   // ------------------------------
   // Estructuras usadas en shaders y buffers constantes
   // ------------------------------

   /**
    * @struct SimpleVertex
    * @brief Representa un vértice con posición en 3D y coordenadas UV para texturas.
    *
    * Se utiliza en buffers de vértices para geometrías básicas.
    */
struct SimpleVertex {
    XMFLOAT3 Pos; ///< Posición 3D del vértice.
    XMFLOAT2 Tex; ///< Coordenadas de textura (UV).
};

/**
 * @struct CBNeverChanges
 * @brief Constant Buffer que almacena la matriz de vista (View), que no cambia por frame.
 *
 * Se actualiza solo una vez a lo largo de toda la ejecución.
 */
struct CBNeverChanges {
    XMMATRIX mView;
};

/**
 * @struct CBChangeOnResize
 * @brief Constant Buffer que almacena la matriz de proyección, actualizada al redimensionar.
 *
 * Útil para ajustes cuando la ventana cambia de tamaño o resolución.
 */
struct CBChangeOnResize {
    XMMATRIX mProjection;
};

/**
 * @struct CBChangesEveryFrame
 * @brief Constant Buffer que cambia en cada frame con la transformación del mundo y color.
 *
 * Se usa para aplicar animaciones, transformaciones o efectos visuales por objeto.
 */
struct CBChangesEveryFrame {
    XMMATRIX mWorld;      ///< Matriz del mundo (modelado).
    XMFLOAT4 vMeshColor;  ///< Color del objeto a renderizar.
};

// ------------------------------
// Enumeraciones globales
// ------------------------------

/**
 * @enum ExtensionType
 * @brief Tipos de extensiones soportadas para cargar texturas.
 *
 * Se puede usar para definir flujos condicionales al cargar archivos.
 */
enum ExtensionType {
    DDS = 0,  ///< Formato DDS (DirectDraw Surface), eficiente en DirectX.
    PNG = 1,  ///< Formato PNG con transparencia.
    JPG = 2   ///< Formato JPG (alta compresión, sin alpha).
};
