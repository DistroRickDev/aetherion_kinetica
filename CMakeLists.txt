cmake_minimum_required(VERSION 3.12)

project(aetherion_kinetica)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)

add_subdirectory(${EXTERNAL_DIR}/SDL)

if(NOT DISABLE_SHADER_C_SUPPORT)
  #TODO(): Run python3 external/shaderc/utils/git-sync-deps
  add_subdirectory(${EXTERNAL_DIR}/shaderc)
  set(SPIRV_CROSS_STATIC ON)
  add_subdirectory(${EXTERNAL_DIR}/SPIRV-Cross)
endif ()

add_subdirectory(src)
