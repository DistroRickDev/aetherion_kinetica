project(shaders)

add_library(${PROJECT_NAME} internal/GlslToSpirv.cpp ShaderHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC spirv-cross-cpp spirv-cross-glsl shaderc_combined logger SDL3::SDL3)
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SPIRV-Cross ${EXTERNAL_DIR}/shaderc/libshaderc/include ${CMAKE_SOURCE_DIR}/src)
