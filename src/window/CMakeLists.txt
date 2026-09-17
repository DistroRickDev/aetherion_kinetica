project(window)

add_library(${PROJECT_NAME} WindowHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3)
if(NOT USE_SYSTEM_SDL)
  target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
