project(render)

add_library(${PROJECT_NAME} EngineRenderer.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 logger shaders)
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include ${CMAKE_SOURCE_DIR}/src)
