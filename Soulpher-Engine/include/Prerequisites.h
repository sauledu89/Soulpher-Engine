/**
 * @file Prerequisites.h
 * @brief Definiciones base, utilidades y tipos comunes para The Visionary Engine.
 *
 * @details
 * Este archivo actúa como punto central de inclusión de:
 * - Librerías estándar de C++ (STL).
 * - Librerías y APIs de DirectX 11.
 * - Utilidades propias del motor (punteros inteligentes).
 * - Macros y tipos de datos comunes usados en todo el motor.
 *
 * @note Es uno de los primeros archivos que se compilan, por lo que otros headers dependen de él.
 * @warning Un cambio aquí puede requerir recompilar la mayoría del proyecto.
 */

#pragma once

 // === Librerías estándar de C++ ===
#include <string>       ///< Manejo de cadenas.
#include <sstream>      ///< Construcción de strings formateados.
#include <vector>       ///< Contenedor dinámico para listas.
#include <windows.h>    ///< API de Windows (necesaria para DirectX).
#include <xnamath.h>    ///< Tipos matemáticos optimizados para DirectX.
#include <thread>       ///< Soporte para programación multihilo.
#include <algorithm>    ///< Funciones utilitarias (min, max, sort, etc.).
#undef min
#undef max

// === Librerías de DirectX 11 ===
#include <d3d11.h>      ///< Núcleo de Direct3D 11.
#include <d3dx11.h>     ///< Utilidades de Direct3D 11.
#include <d3dcompiler.h>///< Compilador de shaders HLSL.
#include "Resource.h"   ///< Recursos del proyecto.
#include "resource.h"   ///< Recursos de Windows.

// === Librerías de utilidades propias del motor ===
#include "EngineUtilities\Memory\TSharedPointer.h" ///< Puntero inteligente compartido.
#include "EngineUtilities\Memory\TWeakPointer.h"   ///< Puntero inteligente débil.
#include "EngineUtilities\Memory\TStaticPtr.h"     ///< Puntero estático.
#include "EngineUtilities\Memory\TUniquePtr.h"     ///< Puntero inteligente único.

// === Macros de utilidad ===

/**
 * @brief Libera un recurso COM y lo pone a nullptr.
 * @param x Recurso COM a liberar.
 *
 * @note Usado para evitar fugas de memoria en objetos DirectX (como buffers, views, shaders).
 */
#define SAFE_RELEASE(x) if(x != nullptr) x->Release(); x = nullptr;

 /**
  * @brief Muestra un mensaje de creación de recurso en el depurador.
  * @param classObj Nombre de la clase que crea el recurso.
  * @param method Nombre del método.
  * @param state Estado o descripción del recurso creado.
  */
#define MESSAGE(classObj, method, state) \
{ std::wostringstream os_; os_ << classObj << "::" << method << " : [CREATION OF RESOURCE : " << state << "]\n"; \
  OutputDebugStringW(os_.str().c_str()); }

  /**
   * @brief Muestra un mensaje de error en el depurador.
   * @param classObj Nombre de la clase que genera el error.
   * @param method Nombre del método.
   * @param errorMSG Mensaje descriptivo del error.
   */
#define ERROR(classObj, method, errorMSG) \
{ try { std::wostringstream os_; os_ << L"ERROR : " << classObj << L"::" << method << L" : " << errorMSG << L"\n"; \
  OutputDebugStringW(os_.str().c_str()); } \
  catch (...) { OutputDebugStringW(L"Failed to log error message.\n"); } }

   // === Estructuras comunes ===

   /**
	* @struct SimpleVertex
	* @brief Estructura básica para un vértice con posición y coordenadas UV.
	*
	* @note Este es un formato mínimo para renderizado básico con texturas.
	*/
struct SimpleVertex { XMFLOAT3 Pos; XMFLOAT2 Tex; };

/**
 * @struct CBNeverChanges
 * @brief Buffer constante con la matriz de vista.
 *
 * @note "NeverChanges" implica que este buffer se actualiza muy rara vez (p.ej., cámara fija).
 */
struct CBNeverChanges { XMMATRIX mView; };

/**
 * @struct CBChangeOnResize
 * @brief Buffer constante con la matriz de proyección.
 *
 * @note Se actualiza al cambiar el tamaño de la ventana.
 */
struct CBChangeOnResize { XMMATRIX mProjection; };

/**
 * @struct CBChangesEveryFrame
 * @brief Buffer constante con matriz de mundo y color de malla.
 *
 * @note Este se actualiza cada frame, por ejemplo, para animar objetos.
 */
struct CBChangesEveryFrame { XMMATRIX mWorld; XMFLOAT4 vMeshColor; };

// === Enumeraciones ===

/**
 * @enum ExtensionType
 * @brief Tipos de extensión de texturas soportadas por el motor.
 */
enum ExtensionType { DDS = 0, PNG = 1, JPG = 2 };

/**
 * @enum ShaderType
 * @brief Tipos de shader soportados.
 */
enum ShaderType { VERTEX_SHADER = 0, PIXEL_SHADER = 1 };

/**
 * @enum ComponentType
 * @brief Tipos de componentes en el sistema ECS.
 */
enum ComponentType { NONE = 0, TRANSFORM = 1, MESH = 2, MATERIAL = 3 };
