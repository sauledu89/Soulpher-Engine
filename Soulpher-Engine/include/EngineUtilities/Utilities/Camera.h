/**
 * @file Camera.h
 * @brief Cámara FPS con basis vectors explícitos para el sistema de render.
 *
 * @details
 * Implementa una cámara 3D estilo First-Person-Shooter usando tres vectores ortogonales
 * (right, up, look) como base del espacio de cámara. El movimiento se expresa en términos
 * de estos vectores locales, lo que simplifica el código de input: "w = mover en look",
 * "strafe = mover en right", independientemente de la orientación actual.
 *
 * La matriz de vista se reconstruye explícitamente desde los basis vectors en
 * `updateViewMatrix()`, lo que evita acumular errores de punto flotante que ocurren
 * al multiplicar matrices de rotación sucesivamente.
 *
 * @note [GameDev] Los motores comerciales (Unreal Engine, Unity) usan cuaterniones
 * internamente para representar la orientación y evitar el gimbal lock (pérdida de
 * un grado de libertad cuando dos ejes de rotación se alinean). Esta implementación
 * con basis vectors es equivalente para rotaciones pequeñas (FPS típico), pero
 * acumularía drift en rotaciones arbitrarias de 360°. Para una cámara de editor
 * completa se recomienda migrar a cuaterniones.
 *
 * @note [GameDev] `GetViewNoTranslation()` devuelve la vista sin la componente de
 * traslación, lo que hace que el skybox siempre parezca infinitamente lejano sin
 * importar la posición de la cámara. En HLSL el shader del skybox multiplica solo
 * por esta matriz en lugar de la vista completa.
 *
 * @ingroup camera
 * @see DeferredRenderer, ISceneRenderer
 */
#pragma once
#include "Prerequisites.h"
#include "EngineUtilities/Vectors/Vector3.h"

/**
 * @class Camera
 * @brief Cámara 3D con basis vectors (right/up/look) y matrices View/Proj.
 *
 * Gestiona la posición, orientación y proyección de la cámara. Provee métodos
 * de movimiento (walk, strafe) y rotación (yaw, pitch) directamente en términos
 * del espacio local de la cámara.
 */
class Camera {
public:
    Camera() = default;

    // ─────────────────────────────────────────────────────────────────────────
    // Configuración de la lente (proyección)
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Configura la proyección perspectiva.
     * @param fovY   Campo visual vertical en radianes (ej. XM_PIDIV4 = 45°).
     * @param aspect Relación de aspecto ancho/alto (ej. 1280/720).
     * @param nearZ  Plano near clip (ej. 0.1f). No usar 0.0f (división por cero).
     * @param farZ   Plano far clip (ej. 1000.0f).
     *
     * @note [GameDev] `fovY` controla cuánto ve la cámara verticalmente. Un fov mayor
     * da sensación de velocidad (juegos de carreras usan ~90°), uno menor da efecto
     * de zoom (francotirador ~20°). El estándar para FPS es 60–75°.
     */
    void setLens(float fovY, float aspect, float nearZ, float farZ);

    // ─────────────────────────────────────────────────────────────────────────
    // Posición y orientación
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Posiciona y orienta la cámara mirando a un objetivo.
     * @param pos    Posición de la cámara en espacio mundo.
     * @param target Punto al que mira la cámara.
     * @param up     Vector "arriba" de referencia (generalmente {0,1,0}).
     */
    void lookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

    // ─────────────────────────────────────────────────────────────────────────
    // Movimiento en espacio local de la cámara
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Mueve la cámara a lo largo del vector look (hacia adelante/atrás).
     * @param d Distancia a moverse (positivo = adelante, negativo = atrás).
     */
    void walk(float d);

    /**
     * @brief Mueve la cámara a lo largo del vector right (lateral izquierda/derecha).
     * @param d Distancia a moverse (positivo = derecha, negativo = izquierda).
     */
    void strafe(float d);

    // ─────────────────────────────────────────────────────────────────────────
    // Rotación en espacio local de la cámara
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Rota la cámara alrededor del eje Y mundial (mirar izquierda/derecha).
     * @param angle Ángulo en radianes (positivo = rotar a la derecha).
     *
     * @note [GameDev] El yaw siempre rota alrededor del eje Y MUNDIAL (no local)
     * para que el horizonte se mantenga recto. Si rotaras alrededor del Y local,
     * al inclinar la cámara el "left/right" también se inclinaría (indeseable en FPS).
     */
    void yaw(float angle);

    /**
     * @brief Rota la cámara alrededor del vector right local (mirar arriba/abajo).
     * @param angle Ángulo en radianes (positivo = mirar abajo).
     */
    void pitch(float angle);

    // ─────────────────────────────────────────────────────────────────────────
    // Actualización y accessores
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Reconstruye la matriz de vista desde la posición y los basis vectors.
     *
     * @details
     * Llama a este método después de cualquier movimiento o rotación para reflejar
     * los cambios en la matriz de vista. La implementación ortogonaliza los basis
     * vectors (Gram-Schmidt) en cada actualización para prevenir drift numérico.
     */
    void updateViewMatrix();

    /** @brief Devuelve la matriz de vista como XMMATRIX (para XMMatrixTranspose antes de subir a GPU). */
    XMMATRIX getView()  const { return XMLoadFloat4x4(&m_view); }

    /** @brief Devuelve la matriz de proyección como XMMATRIX (para XMMatrixTranspose antes de subir a GPU). */
    XMMATRIX getProj()  const { return XMLoadFloat4x4(&m_proj); }

    /**
     * @brief Devuelve la vista sin componente de traslación (para skybox).
     *
     * Devuelve la parte 3×3 de rotación de la vista embebida en una 4×4,
     * con la columna de traslación a cero. Usada en el skybox pass para que
     * el fondo siempre parezca infinitamente lejano.
     */
    XMFLOAT4X4 GetViewNoTranslation() const;

    /** @brief Posición actual de la cámara en espacio mundo. */
    EU::Vector3 getPosition() const { return EU::Vector3(m_position.x, m_position.y, m_position.z); }

    /** @brief Vector "right" local de la cámara. */
    EU::Vector3 getRight() const { return EU::Vector3(m_right.x, m_right.y, m_right.z); }

    /** @brief Vector "up" local de la cámara. */
    EU::Vector3 getUp() const { return EU::Vector3(m_up.x, m_up.y, m_up.z); }

    /** @brief Vector "look" (forward) local de la cámara. */
    EU::Vector3 getLook() const { return EU::Vector3(m_look.x, m_look.y, m_look.z); }

private:
    XMFLOAT3   m_position = { 0.0f, 0.0f, -5.0f }; ///< Posición en espacio mundo.
    XMFLOAT3   m_right    = { 1.0f, 0.0f,  0.0f }; ///< Eje X local (derecha).
    XMFLOAT3   m_up       = { 0.0f, 1.0f,  0.0f }; ///< Eje Y local (arriba).
    XMFLOAT3   m_look     = { 0.0f, 0.0f,  1.0f }; ///< Eje Z local (adelante).

    XMFLOAT4X4 m_view = {}; ///< Matriz de vista actualizada por updateViewMatrix().
    XMFLOAT4X4 m_proj = {}; ///< Matriz de proyección configurada por setLens().
};


// ─────────────────────────────────────────────────────────────────────────────
// Implementaciones inline
// ─────────────────────────────────────────────────────────────────────────────

inline void Camera::setLens(float fovY, float aspect, float nearZ, float farZ) {
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    XMStoreFloat4x4(&m_proj, proj); // DeferredRenderer.cpp transposes before GPU upload
}

inline void Camera::lookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& worldUp) {
    XMVECTOR P = XMLoadFloat3(&pos);
    XMVECTOR T = XMLoadFloat3(&target);
    XMVECTOR U = XMLoadFloat3(&worldUp);

    XMVECTOR L = XMVector3Normalize(XMVectorSubtract(T, P));
    XMVECTOR R = XMVector3Normalize(XMVector3Cross(U, L));
    XMVECTOR Ulocal = XMVector3Cross(L, R);

    XMStoreFloat3(&m_position, P);
    XMStoreFloat3(&m_right,    R);
    XMStoreFloat3(&m_up,       Ulocal);
    XMStoreFloat3(&m_look,     L);
}

inline void Camera::walk(float d) {
    XMVECTOR s = XMVectorReplicate(d);
    XMVECTOR l = XMLoadFloat3(&m_look);
    XMVECTOR p = XMLoadFloat3(&m_position);
    XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, l, p));
}

inline void Camera::strafe(float d) {
    XMVECTOR s = XMVectorReplicate(d);
    XMVECTOR r = XMLoadFloat3(&m_right);
    XMVECTOR p = XMLoadFloat3(&m_position);
    XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, r, p));
}

inline void Camera::yaw(float angle) {
    XMMATRIX R = XMMatrixRotationY(angle);
    XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
    XMStoreFloat3(&m_up,    XMVector3TransformNormal(XMLoadFloat3(&m_up),    R));
    XMStoreFloat3(&m_look,  XMVector3TransformNormal(XMLoadFloat3(&m_look),  R));
}

inline void Camera::pitch(float angle) {
    XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);
    XMStoreFloat3(&m_up,   XMVector3TransformNormal(XMLoadFloat3(&m_up),   R));
    XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));
}

inline void Camera::updateViewMatrix() {
    XMVECTOR R = XMLoadFloat3(&m_right);
    XMVECTOR U = XMLoadFloat3(&m_up);
    XMVECTOR L = XMLoadFloat3(&m_look);
    XMVECTOR P = XMLoadFloat3(&m_position);

    // Re-ortogonalizar los basis vectors (Gram-Schmidt)
    L = XMVector3Normalize(L);
    U = XMVector3Normalize(XMVector3Cross(L, R));
    R = XMVector3Cross(U, L);

    XMStoreFloat3(&m_right, R);
    XMStoreFloat3(&m_up,    U);
    XMStoreFloat3(&m_look,  L);

    float x = -XMVectorGetX(XMVector3Dot(P, R));
    float y = -XMVectorGetX(XMVector3Dot(P, U));
    float z = -XMVectorGetX(XMVector3Dot(P, L));

    // Construir la matriz de vista en row-major (DX11 la transpondrá al shader)
    m_view(0,0) = m_right.x; m_view(1,0) = m_right.y; m_view(2,0) = m_right.z; m_view(3,0) = x;
    m_view(0,1) = m_up.x;    m_view(1,1) = m_up.y;    m_view(2,1) = m_up.z;    m_view(3,1) = y;
    m_view(0,2) = m_look.x;  m_view(1,2) = m_look.y;  m_view(2,2) = m_look.z;  m_view(3,2) = z;
    m_view(0,3) = 0.0f;      m_view(1,3) = 0.0f;      m_view(2,3) = 0.0f;      m_view(3,3) = 1.0f;
}

inline XMFLOAT4X4 Camera::GetViewNoTranslation() const {
    XMFLOAT4X4 v = m_view;
    // Zerear la columna de traslación (última columna en layout column-major DX)
    v(3,0) = 0.0f;
    v(3,1) = 0.0f;
    v(3,2) = 0.0f;
    v(3,3) = 1.0f;
    return v;
}
