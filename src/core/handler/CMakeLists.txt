project(handler)

add_library(${PROJECT_NAME} EngineHandler.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC gui logger graphics render shaders window)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src ${EXTERNAL_DIR})
