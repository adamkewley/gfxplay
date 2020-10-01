cmake_minimum_required(VERSION 3.13)

project(gfxplay VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
find_package(OpenGL REQUIRED)
find_package(Cairo REQUIRED)

add_library(common STATIC
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/sdl.hpp
    src/gl.hpp
    src/glglm.hpp
    src/stbi.hpp
    src/cairo.hpp
)
target_include_directories(common PUBLIC
    # ${OPENGL_INCLUDE_DIR}  // GLEW handles forwarding the OpenGL API
    third_party/SDL2-2.0.12/include
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include
    third_party/imgui-1.78/
    third_party/stb_image/
    ${PROJECT_BINARY_DIR}  # so configure_file config can be #included
)
target_link_libraries(common
    ${OPENGL_LIBRARIES}
    SDL2
    glew
    ${Cairo_LIBRARY}
)
target_compile_options(common PUBLIC
    # disable MSVC permissiveness. Forces MSVC to obey C++ standard
    # - specifically, this enables the use of 'and', 'not', etc.
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(common PUBLIC
    cxx_std_20
)
set_target_properties(common PROPERTIES
    CXX_EXTENSIONS OFF
)


add_executable(learnopengl src/gfxplay.cpp)
target_link_libraries(learnopengl common)
add_executable(clock src/clock.cpp)
target_link_libraries(clock common)
source_group(TREE third_party/)
