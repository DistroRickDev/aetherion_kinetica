project(engine)

add_subdirectory(core)
add_subdirectory(graphics)
add_subdirectory(window)

add_executable(${PROJECT_NAME} main.cpp)
if(NOT USE_SYSTEM_SDL)
  target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include ${EXTERNAL_DIR}/imgui)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC handler logger graphics window)
