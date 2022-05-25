cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(RenderingEnvironment VERSION 1.0
  DESCRIPTION "Hobby rendering environment"
  LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message("Voidptr size: ${CMAKE_SIZEOF_VOID_P}") # 8 for 64 bit and 4 for 32 bit
message("Binary dir: ${CMAKE_CURRENT_BINARY_DIR}") 

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_EXTERNAL_DIRECTORY       "${CMAKE_SOURCE_DIR}/external")

IF(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build arm architecture for Mac OS X" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -framework AppKit \
    -framework Metal \
    -framework MetalKit \
    -framework ModelIO")
ELSEIF (WIN32)
  message(STATUS "Setting MSVC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /std:c++latest -O2 -Wall")
ENDIF()

set(SUBMODULE_DIR ${CMAKE_SOURCE_DIR}/submodules)
add_subdirectory(${SUBMODULE_DIR}/glfw)
add_subdirectory(${SUBMODULE_DIR}/glm)
add_subdirectory(${SUBMODULE_DIR}/glad)
add_subdirectory(${SUBMODULE_DIR}/entt)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${SUBMODULE_DIR}
  ${SUBMODULE_DIR}/imgui
  ${SUBMODULE_DIR}/ImGuizmo
  ${SUBMODULE_DIR}/entt/src
  ${SUBMODULE_DIR}/metal-cpp
  ${SUBMODULE_DIR}/metal-cpp-extensions
  ${SUBMODULE_DIR}/OpenFBX/src
  ${GLFW_INCLUDE_DIRS}
  ${GLAD_INCLUDE_DIRS}
)

add_compile_definitions(IMGUI_USER_CONFIG="local_imgui_config.h")

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(GLOB SRCFILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB SRCHEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB GLAD ${SUBMODULE_DIR}/glad/src/*.c)
file(GLOB IMGUI 
  ${SUBMODULE_DIR}/imgui/imgui.cpp 
  ${SUBMODULE_DIR}/imgui/imgui_draw.cpp
  ${SUBMODULE_DIR}/imgui/imgui_tables.cpp
  ${SUBMODULE_DIR}/imgui/imgui_widgets.cpp
  ${SUBMODULE_DIR}/imgui/backends/imgui_impl_opengl3.cpp 
  ${SUBMODULE_DIR}/imgui/backends/imgui_impl_glfw.cpp
  ${SUBMODULE_DIR}/ImGuizmo/ImGuizmo.cpp
  #${SUBMODULE_DIR}/imgui/backends/imgui_impl_metal.mm
)

file(GLOB OPENFBX ${SUBMODULE_DIR}/OpenFBX/src/ofbx.cpp ${SUBMODULE_DIR}/OpenFBX/src/miniz.c)

add_executable(${PROJECT_NAME}
  ${SRCFILES}
  ${SRCHEADERS}
  ${IMGUI}
  ${OPENFBX}
  ${GLAD}
)

target_link_libraries(${PROJECT_NAME} glm glfw glad)
