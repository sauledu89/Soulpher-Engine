/**
 * @file LogManager.h
 * @brief Sistema central de logging del motor (patrón Observer/singleton).
 *
 * @details
 * Punto único al que todas las clases del motor envían sus mensajes de
 * depuración mediante las macros `LOG_MESSAGE`, `LOG_WARNING` y `LOG_ERROR`.
 * Cada entrada se guarda en un buffer circular que la consola de la UI
 * (`UserInterface::consolePanel`) lee y muestra en tiempo real, y además se
 * reenvía a `OutputDebugStringA` para no perder la visibilidad en el Output
 * de Visual Studio.
 *
 * @note [GameDev] Este es el mismo patrón que Unreal Engine expone como
 * `UE_LOG` + Output Log window, o Unity como `Debug.Log` + la consola del
 * editor: un sink central que desacopla "quién genera el mensaje" de
 * "dónde se muestra".
 *
 * @ingroup utilities
 */
#pragma once
#include <string>
#include <vector>
#include <windows.h>

/**
 * @enum LogLevel
 * @brief Severidad de una entrada de log.
 */
enum class LogLevel {
    Message = 0, ///< Información general (creación de recursos, estado normal).
    Warning = 1, ///< Situación anómala pero no fatal.
    Error   = 2  ///< Fallo que impide continuar con la operación actual.
};

/**
 * @struct LogEntry
 * @brief Una línea de log ya formateada, lista para mostrarse en la consola de la UI.
 */
struct LogEntry {
    LogLevel    level;
    std::string text;
};

/**
 * @class LogManager
 * @brief Singleton que recolecta y almacena todos los mensajes de log del motor.
 */
class LogManager {
public:
    /** @brief Acceso a la única instancia del manager. */
    static LogManager& instance() {
        static LogManager inst;
        return inst;
    }

    /**
     * @brief Registra un mensaje. `state` se trata como texto plano (nunca como
     * un formato printf), así que nunca hay riesgo de interpretar un '%' que
     * venga de contenido dinámico (nombres de archivo, rutas, etc.).
     * @param level    Severidad del mensaje.
     * @param classObj Nombre de la clase que origina el mensaje.
     * @param method   Nombre del método que origina el mensaje.
     * @param state    Descripción del evento.
     */
    void log(LogLevel level, const char* classObj, const char* method, const std::string& state) {
        std::string text = std::string(classObj) + "::" + method + " : " + state;

        m_entries.push_back(LogEntry{ level, text });
        if (m_entries.size() > kMaxEntries) {
            m_entries.erase(m_entries.begin(), m_entries.begin() + (m_entries.size() - kMaxEntries));
        }

        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    /** @brief Todas las entradas almacenadas, en orden cronológico. */
    const std::vector<LogEntry>& entries() const { return m_entries; }

    /** @brief Vacía la consola. */
    void clear() { m_entries.clear(); }

private:
    LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::vector<LogEntry> m_entries;
    static constexpr size_t kMaxEntries = 2000; ///< Descarta las entradas mas viejas al superar este limite.
};

/** @def LOG_MESSAGE(classObj, method, state) Registra un mensaje informativo. */
#define LOG_MESSAGE(classObj, method, state) LogManager::instance().log(LogLevel::Message, classObj, method, state)

/** @def LOG_WARNING(classObj, method, state) Registra una advertencia (no detiene la ejecución). */
#define LOG_WARNING(classObj, method, state) LogManager::instance().log(LogLevel::Warning, classObj, method, state)

/** @def LOG_ERROR(classObj, method, state) Registra un error. */
#define LOG_ERROR(classObj, method, state)   LogManager::instance().log(LogLevel::Error,   classObj, method, state)
