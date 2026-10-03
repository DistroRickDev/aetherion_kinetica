#include "SdlLoggerHandler.h"

#include <chrono>
#include <iomanip>
#include <stacktrace>

#include <SDL3/SDL_log.h>

namespace {
    enum class LogSeverity { Debug, Error, Info, Warn };

    constexpr std::string logSeverityToString(const LogSeverity logSeverity) {
        switch (logSeverity) {
            case LogSeverity::Debug: return "DEBUG";
            case LogSeverity::Error: return "ERROR";
            case LogSeverity::Info: return "INFO";
            case LogSeverity::Warn: return "WARN";
        }
        return "INVALID";
    }

    constexpr auto kCalleePos{2U};

    std::string createFormattedLogMessage(const LogSeverity logSeverity, const std::string &msg) {
        const auto st{std::stacktrace::current()};
        const auto timeStamp{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
        const auto [M_tmb, M_fmt]{std::put_time(std::localtime(&timeStamp), "%Y-%m-%d %X")};
        const auto formattedTime{
            std::format("{}/{}/{} {}:{}:{}",
                        M_tmb->tm_mon < 10 ? std::format("0{}", M_tmb->tm_mon) : std::format("{}", M_tmb->tm_mon),
                        M_tmb->tm_mday, (1900 + M_tmb->tm_year), M_tmb->tm_hour,
                        M_tmb->tm_min < 10 ? std::format("0{}", M_tmb->tm_min) : std::format("{}", M_tmb->tm_min),
                        M_tmb->tm_sec)
        };
        return std::format("{} | {}:{} | [{}] | : {}", formattedTime, st[kCalleePos].source_file(),
                           st[kCalleePos].source_line(), logSeverityToString(logSeverity), msg);
    }
} // namespace

namespace engine::core::logger {
    void ILogger::debug(const std::string &msg) {
        SDL_Log("%s", createFormattedLogMessage(LogSeverity::Debug, msg).c_str());
    }

    void ILogger::error(const std::string &msg) {
        SDL_Log("%s", createFormattedLogMessage(LogSeverity::Error, msg).c_str());
    }

    void ILogger::info(const std::string &msg) {
        SDL_Log("%s", createFormattedLogMessage(LogSeverity::Info, msg).c_str());
    }

    void ILogger::warn(const std::string &msg) {
        SDL_Log("%s", createFormattedLogMessage(LogSeverity::Warn, msg).c_str());
    }
} // namespace engine::core::logger
