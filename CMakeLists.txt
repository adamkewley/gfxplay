cmake_minimum_required(VERSION 3.13)

project(gfxplay VERSION 0.0.1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_BUILD_TYPE MATCHES "Rel")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

option(GFXPLAY_USE_ASSIMP "build assimp-related demos" YES)
option(GFXPLAY_USE_CAIRO "build cairo-based demos" YES)

add_subdirectory(third_party/glew-2.1.0/build/cmake/ EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm-0.9.9.8 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/SDL2-2.0.12 EXCLUDE_FROM_ALL)
# find_package(OpenGL REQUIRED) linking to glew/glew_s resolves this

set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/)
configure_file(src/gfxplay_config.hpp.in gfxplay_config.hpp)

add_library(common STATIC
    third_party/imgui-1.78/imgui.cpp
    third_party/imgui-1.78/imgui_demo.cpp
    third_party/imgui-1.78/imgui_draw.cpp
    third_party/imgui-1.78/imgui_widgets.cpp
    third_party/imgui-1.78/examples/imgui_impl_opengl3.cpp
    third_party/imgui-1.78/examples/imgui_impl_sdl.cpp

    src/sdl.hpp
    src/sdl.cpp
    src/gl.hpp
    src/gl.cpp
    src/gl_extensions.hpp
    src/gl_extensions.cpp
    src/imgui_extensions.hpp
    src/imgui_extensions.cpp
    src/logl_common.hpp
    src/ak_common-shaders.hpp
)
target_include_directories(common PUBLIC
    third_party/SDL2-2.0.12/include
    third_party/glm-0.9.9.8/
    third_party/glew-2.1.0/include
    third_party/imgui-1.78/
    third_party/stb_image/
    ${PROJECT_BINARY_DIR}  # so configure_file config can be #included
)

target_link_libraries(common stdc++fs)
if (CMAKE_BUILD_TYPE MATCHES "Rel")
    target_link_libraries(common SDL2 glew)
else()
    target_link_libraries(common SDL2-static glew_s)
endif()

target_compile_options(common PUBLIC
    # disable MSVC permissiveness. Forces MSVC to obey C++ standard
    # - specifically, this enables the use of 'and', 'not', etc.
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)
target_compile_features(common PUBLIC
    cxx_std_20
)
target_link_options(common PUBLIC
    # disable opening a console window when booting normally
    $<$<CXX_COMPILER_ID:MSVC>:/SUBSYSTEM:windows>
    $<$<CXX_COMPILER_ID:MSVC>:/ENTRY:mainCRTStartup>
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

# https://learnopengl.com/Advanced-OpenGL/Blending
add_executable(logl_blending src/logl_blending.cpp)
target_link_libraries(logl_blending common)

# https://learnopengl.com/Advanced-OpenGL/Face-culling
add_executable(logl_face-culling src/logl_face-culling.cpp)
target_link_libraries(logl_face-culling common)

# https://learnopengl.com/Advanced-OpenGL/Geometry-Shader
add_executable(logl_geom-shader-normals src/logl_geom-shader-normals.cpp)
target_link_libraries(logl_geom-shader-normals common)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps src/logl_cubemaps.cpp)
target_link_libraries(logl_cubemaps common)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps_environment-mapping
    src/logl_cubemaps_environment-mapping.cpp
)
target_link_libraries(logl_cubemaps_environment-mapping common)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps_environment-mapping_refractions
    src/logl_cubemaps_environment-mapping_refractions.cpp
)
target_link_libraries(logl_cubemaps_environment-mapping_refractions common)

# https://learnopengl.com/Advanced-OpenGL/Instancing
add_executable(logl_instancing_1_basic src/logl_instancing_1_basic.cpp)
target_link_libraries(logl_instancing_1_basic common)

# https://learnopengl.com/Advanced-OpenGL/Instancing
add_executable(logl_instancing_2_instanced-arrays src/logl_instancing_2_instanced-arrays.cpp)
target_link_libraries(logl_instancing_2_instanced-arrays common)

# https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing
add_executable(logl_anti-aliasing-a-framebuffer src/logl_anti-aliasing-a-framebuffer.cpp)
target_link_libraries(logl_anti-aliasing-a-framebuffer common)

add_executable(logl_save-to-file src/logl_save-to-file.cpp)
target_link_libraries(logl_save-to-file common)

add_executable(imgui_demo src/imgui_demo.cpp)
target_link_libraries(imgui_demo common)

if (GFXPLAY_USE_ASSIMP)
    add_subdirectory(third_party/assimp-5.0.1 EXCLUDE_FROM_ALL)

    # https://learnopengl.com/Model-Loading/Assimp
    add_executable(logl_13_model-loading
        src/logl_13_model-loading.cpp
        src/logl_model.hpp
    )
    target_link_libraries(logl_13_model-loading common assimp)

    # https://learnopengl.com/Advanced-OpenGL/Instancing
    add_executable(logl_instancing_3_planet
        src/logl_instancing_3_planet.cpp
    )
    target_link_libraries(logl_instancing_3_planet common assimp)
endif()

# https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
add_executable(logl_advanced-lighting
    src/logl_advanced-lighting.cpp
)
target_link_libraries(logl_advanced-lighting common)

# https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
add_executable(logl_gamma-correction
    src/logl_gamma-correction.cpp
)
target_link_libraries(logl_gamma-correction common)

# https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
add_executable(logl_shadows_shadow-mapping
    src/logl_shadows_shadow-mapping.cpp
)
target_link_libraries(logl_shadows_shadow-mapping common)

# https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
add_executable(logl_shadows_point-shadows
    src/logl_shadows_point-shadows.cpp
)
target_link_libraries(logl_shadows_point-shadows common)

add_executable(logl_normal-mapping
    src/logl_normal-mapping.cpp
)
target_link_libraries(logl_normal-mapping common)

# personal experiment: selectable geometry
add_executable(ak_selectable-geom src/ak_selectable-geom.cpp)
target_link_libraries(ak_selectable-geom common)

if (GFXPLAY_USE_CAIRO)
    find_package(Cairo REQUIRED)

    add_executable(clock src/clock.cpp)
    target_link_libraries(clock common ${Cairo_LIBRARY})
    target_include_directories(clock PUBLIC src/cairo.hpp)

    add_executable(qtree src/qtree.cpp)
    target_link_libraries(qtree common ${Cairo_LIBRARY})
    target_include_directories(qtree PUBLIC src/cairo.hpp)
endif()

source_group(TREE third_party/)
