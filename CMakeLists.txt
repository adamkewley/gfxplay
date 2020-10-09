cmake_minimum_required(VERSION 3.13)

project(gfxplay VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(GFXPLAY_USE_ASSIMP "build assimp-related demos" NO)
option(GFXPLAY_USE_CAIRO "build cairo-based demos" NO)

add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
find_package(OpenGL REQUIRED)

set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/)
configure_file(src/gfxplay_config.hpp.in gfxplay_config.hpp)

add_library(common STATIC
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/sdl.hpp
    src/gl.hpp
    src/stbi.hpp
    src/logl_common.hpp
)
target_include_directories(common PUBLIC
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
    glew_s
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


# https://learnopengl.com/Getting-started/Hello-Triangle
add_executable(logl_1_hellotriangle src/logl_1_hellotriangle.cpp)
target_link_libraries(logl_1_hellotriangle common)

# https://learnopengl.com/Getting-started/Shaders
add_executable(logl_2_shaders src/logl_2_shaders.cpp)
target_link_libraries(logl_2_shaders common)

# https://learnopengl.com/Getting-started/Textures
add_executable(logl_3_textures src/logl_3_textures.cpp)
target_link_libraries(logl_3_textures common)

# https://learnopengl.com/Getting-started/Transformations
add_executable(logl_4_transformations src/logl_4_transformations.cpp)
target_link_libraries(logl_4_transformations common)

# https://learnopengl.com/Getting-started/Coordinate-Systems
add_executable(logl_5_coordinate-systems src/logl_5_coordinate-systems.cpp)
target_link_libraries(logl_5_coordinate-systems common)

# https://learnopengl.com/Getting-started/Camera
add_executable(logl_6_camera src/logl_6_camera.cpp)
target_link_libraries(logl_6_camera common)

# https://learnopengl.com/Lighting/Colors
add_executable(logl_7_colors src/logl_7_colors.cpp)
target_link_libraries(logl_7_colors common)

# https://learnopengl.com/Lighting/Basic-Lighting
add_executable(logl_8_basic-lighting src/logl_8_basic-lighting.cpp)
target_link_libraries(logl_8_basic-lighting common)

# https://learnopengl.com/Lighting/Materials
add_executable(logl_9_materials src/logl_9_materials.cpp)
target_link_libraries(logl_9_materials common)

# https://learnopengl.com/Lighting/Lighting-maps
add_executable(logl_10_lighting-maps src/logl_10_lighting-maps.cpp)
target_link_libraries(logl_10_lighting-maps common)

# https://learnopengl.com/Lighting/Light-casters
add_executable(logl_11_light-casters src/logl_11_light-casters.cpp)
target_link_libraries(logl_11_light-casters common)

# https://learnopengl.com/Lighting/Multiple-lights
add_executable(logl_12_multiple-lights src/logl_12_multiple-lights.cpp)
target_link_libraries(logl_12_multiple-lights common)

# https://learnopengl.com/Advanced-OpenGL/Depth-testing
add_executable(logl_depth-testing src/logl_depth-testing.cpp)
target_link_libraries(logl_depth-testing common)

# https://learnopengl.com/Advanced-OpenGL/Stencil-testing
add_executable(logl_stencil-testing src/logl_stencil-testing.cpp)
target_link_libraries(logl_stencil-testing common)

# https://learnopengl.com/Advanced-OpenGL/Framebuffers
add_executable(logl_framebuffers src/logl_framebuffers.cpp)
target_link_libraries(logl_framebuffers common)

add_executable(logl_save-to-file src/logl_save-to-file.cpp)
target_link_libraries(logl_save-to-file common)

if (GFXPLAY_USE_ASSIMP)
    add_subdirectory(third_party/assimp-5.0.1 EXCLUDE_FROM_ALL)

    # https://learnopengl.com/Model-Loading/Assimp
    add_executable(logl_13_model-loading src/logl_13_model-loading.cpp)
    target_link_libraries(logl_13_model-loading common assimp)
endif()

if (GFXPLAY_USE_CAIRO)
    find_package(Cairo REQUIRED)

    add_executable(clock src/clock.cpp)
    target_link_libraries(clock common ${Cairo_LIBRARY})
    target_include_directories(clock PUBLIC src/cairo.hpp)
endif()

source_group(TREE third_party/)
