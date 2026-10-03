project(window)

add_library(${PROJECT_NAME} WindowHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
