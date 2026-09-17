project(engine_gui)

set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)

file(GLOB IM_GUI_SRC ${IMGUI_DIR}/*.cpp)

add_library(${PROJECT_NAME}
        EngineGui.cpp
        ${IM_GUI_SRC}
        ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC logger)

target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR} ${IMGUI_DIR} ${CMAKE_SOURCE_DIR}/src)
