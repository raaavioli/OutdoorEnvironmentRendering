cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(ParticleClustering VERSION 1.0
  DESCRIPTION "Particle clustering for master's degree proejct"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Setting MSVC flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /std:c++latest -O2 -Wall")

message("Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message("Voidptr size: ${CMAKE_SIZEOF_VOID_P}") # 8 for 64 bit and 4 for 32 bit
message("Binary dir: ${CMAKE_CURRENT_BINARY_DIR}") 

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_EXTERNAL_DIRECTORY       "${CMAKE_SOURCE_DIR}/external")

IF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build arm architecture for Mac OS X" FORCE)
ENDIF(APPLE)

set(SUBMODULE_DIR ${CMAKE_SOURCE_DIR}/submodules)
add_subdirectory(${SUBMODULE_DIR}/glfw)
add_subdirectory(${SUBMODULE_DIR}/glm)
add_subdirectory(${SUBMODULE_DIR}/glad)

include_directories(
  ${SUBMODULE_DIR}
  ${GLFW_INCLUDE_DIRS}
  ${SUBMODULE_DIR}/imgui
  ${GLAD_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include
  ${SUBMODULE_DIR}/OpenFBX/src
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(GLOB SRCFILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB IMGUI 
  ${SUBMODULE_DIR}/imgui/imgui.cpp 
  ${SUBMODULE_DIR}/imgui/imgui_draw.cpp
  ${SUBMODULE_DIR}/imgui/imgui_tables.cpp
  ${SUBMODULE_DIR}/imgui/imgui_widgets.cpp
  ${SUBMODULE_DIR}/imgui/backends/imgui_impl_opengl3.cpp 
  ${SUBMODULE_DIR}/imgui/backends/imgui_impl_glfw.cpp
)

file(GLOB OPENFBX
  ${SUBMODULE_DIR}/OpenFBX/src/ofbx.cpp
  ${SUBMODULE_DIR}/OpenFBX/src/miniz.c
)

add_executable(${PROJECT_NAME}
  ${SRCFILES}
  ${IMGUI}
  ${OPENFBX}
)

target_link_libraries(${PROJECT_NAME} glm glfw glad)