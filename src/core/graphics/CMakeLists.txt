project(graphics)

add_library(${PROJECT_NAME} GpuDeviceHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include)
