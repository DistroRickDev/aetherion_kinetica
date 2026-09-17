#ifndef AETHERION_KINETICA_SDL_LOGGER_HANDLER_H
#define AETHERION_KINETICA_SDL_LOGGER_HANDLER_H

#include "ILogger.h"

namespace engine::core::logger {
    class SdlLoggerHandler final : public ILogger {
    public:
        SdlLoggerHandler() = default;

        SdlLoggerHandler(SdlLoggerHandler const &) = delete;

        void operator=(SdlLoggerHandler const &) = delete;

        ~SdlLoggerHandler() override = default;
    };
} // namespace engine::core::logger
#endif
