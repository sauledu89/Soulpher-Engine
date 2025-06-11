#pragma once

// =======================================
// @file Prerequisites.h
// @brief Incluye librer�as esenciales y define macros, estructuras y tipos usados globalmente.
// =======================================

// ------------------------------
// Librer�as est�ndar de C++
// ------------------------------
#include <string>     ///< Para manipulaci�n de cadenas.
#include <sstream>    ///< Para construcci�n de textos con flujos.
#include <vector>     ///< Para uso de arreglos din�micos.
#include <windows.h>  ///< Interacci�n con la API de Windows.
#include <xnamath.h>  ///< Operaciones matem�ticas de DirectX.
#include <thread>     ///< Soporte para ejecuci�n multihilo.

// ------------------------------
// Librer�as de DirectX 11
// ------------------------------
#include <d3d11.h>       ///< Interfaces principales de Direct3D 11.
#include <d3dx11.h>      ///< Funciones auxiliares de inicializaci�n y carga.
#include <d3dcompiler.h> ///< Compilaci�n de shaders.
#include "Resource.h"    ///< Recursos de Win32 (�conos, cursores, etc.).

// ------------------------------
// Macros �tiles para debugging y gesti�n de recursos
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
  * @brief Macro para imprimir mensajes de creaci�n de recursos en la consola de depuraci�n.
  *
  * Se utiliza para verificar la correcta inicializaci�n de recursos gr�ficos en desarrollo.
  *
  * @param classObj Nombre de la clase desde la que se llama.
  * @param method Nombre del m�todo.
  * @param state Mensaje de estado o descripci�n.
  */
#define MESSAGE(classObj, method, state)                       \
{                                                              \
   std::wostringstream os_;                                    \
   os_ << classObj << "::" << method << " : "                  \
       << "[CREATION OF RESOURCE : " << state << "] \n";       \
   OutputDebugStringW(os_.str().c_str());                      \
}

  /**
   * @brief Macro para imprimir errores con formato en la consola de depuraci�n.
   *
   * Captura errores en tiempo de ejecuci�n y los muestra en Visual Studio.
   *
   * @param classObj Nombre de la clase desde la que se llama.
   * @param method Nombre del m�todo.
   * @param errorMSG Mensaje de error espec�fico.
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
    * @brief Representa un v�rtice con posici�n en 3D y coordenadas UV para texturas.
    *
    * Se utiliza en buffers de v�rtices para geometr�as b�sicas.
    */
struct SimpleVertex {
    XMFLOAT3 Pos; ///< Posici�n 3D del v�rtice.
    XMFLOAT2 Tex; ///< Coordenadas de textura (UV).
};

/**
 * @struct CBNeverChanges
 * @brief Constant Buffer que almacena la matriz de vista (View), que no cambia por frame.
 *
 * Se actualiza solo una vez a lo largo de toda la ejecuci�n.
 */
struct CBNeverChanges {
    XMMATRIX mView;
};

/**
 * @struct CBChangeOnResize
 * @brief Constant Buffer que almacena la matriz de proyecci�n, actualizada al redimensionar.
 *
 * �til para ajustes cuando la ventana cambia de tama�o o resoluci�n.
 */
struct CBChangeOnResize {
    XMMATRIX mProjection;
};

/**
 * @struct CBChangesEveryFrame
 * @brief Constant Buffer que cambia en cada frame con la transformaci�n del mundo y color.
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
    JPG = 2   ///< Formato JPG (alta compresi�n, sin alpha).
};
