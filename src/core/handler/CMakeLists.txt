project(handler)

add_library(${PROJECT_NAME} EngineHandler.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE engine_gui logger graphics window)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${EXTERNAL_DIR})
