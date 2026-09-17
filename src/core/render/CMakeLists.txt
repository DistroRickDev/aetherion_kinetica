project(EngineRenderer)

add_library(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3)
if(NOT USE_SYSTEM_SDL)
  target_include_directories(${PROJECT_NAME} PUBLIC ${EXTERNAL_DIR}/SDL/include)
endif()
