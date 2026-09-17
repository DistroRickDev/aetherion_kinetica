#ifndef AETHERION_KINETICA_ENGINE_CORE_LOGGER_ILOGGER_H
#define AETHERION_KINETICA_ENGINE_CORE_LOGGER_ILOGGER_H
#include <memory>

namespace engine::core::logger {
    class ILogger {
    public:
        virtual ~ILogger() = default;

        static void debug(const std::string &);

        static void error(const std::string &);

        static void info(const std::string &);

        static void warn(const std::string &);
    };
} // namespace engine::core::logger
#endif
