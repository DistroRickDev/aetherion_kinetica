project(engine)

add_subdirectory(core)
add_subdirectory(window)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include ${EXTERNAL_DIR}/imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC handler)
