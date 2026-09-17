project(logger)

add_library(${PROJECT_NAME} SdlLoggerHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC stdc++exp)
