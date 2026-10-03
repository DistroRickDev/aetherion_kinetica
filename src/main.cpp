#include "core/logger/SdlLoggerHandler.h"
#include "core/handler/EngineHandler.h"

using namespace engine::core::logger;

int main(const int argc, const char **argv) {
    /* 1. Initialize Engine */
    auto engine{engine::core::handler::EngineHandler()};
    engine.initialize();
    engine.run(argc, argv);
    return 0;
}
