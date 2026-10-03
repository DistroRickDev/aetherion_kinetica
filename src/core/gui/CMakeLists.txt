project(gui)

set(IMGUI_DIR ${EXTERNAL_DIR}/imgui)

file(GLOB IM_GUI_SRC ${IMGUI_DIR}/*.cpp)

add_library(${PROJECT_NAME}
        GuiHandler.cpp
        shader/CompileShaderWindow.cpp
        ${IM_GUI_SRC}
        ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC logger graphics shaders)

target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR} ${IMGUI_DIR} ${CMAKE_SOURCE_DIR}/src)
