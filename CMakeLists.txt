cmake_minimum_required(VERSION 3.12)

project(aetherion_kinetica)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT USE_SYSTEM_SDL)
  add_subdirectory(external/SDL)
endif()
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
add_subdirectory(src)
