cmake_minimum_required(VERSION 3.13)

project(gfxplay VERSION 0.0.1 LANGUAGES C CXX)

# -------------- top-level configuration ---------------- #

get_property(GFXPLAY_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

set(GFXPLAY_USE_LINTER OFF CACHE BOOL "disable/enable clang tidy")
set(GFXPLAY_USE_ASSIMP OFF CACHE BOOL "disable/enable assimp demos")
set(GFXPLAY_USE_CAIRO OFF CACHE BOOL "disable/enable cairo demos")
set(GFXPLAY_USE_IWYU OFF CACHE BOOL "enable/disable include-what-you-use (iwyu)")
set(GFXPLAY_IWYU_COMMAND "iwyu" CACHE STRING "command to run when using iwyu")
set(GFXPLAY_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/" CACHE STRING "where runtime resources are loaded from")

if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set(LINUX TRUE)
    else()
        message(WARNING "you are building on a UNIX system that may not be supported yet - beware!")
    endif()
endif()


# --------------- dependency management ----------------- #
include(Gfxplay_GetDependencies.cmake)

# -------------- build target configuration ------------- #

# create gfxplay_config.hpp with static, compile-time, values that the
# codebase can use
configure_file(src/gfxplay_config.hpp.in ${CMAKE_BINARY_DIR}/gfxplay_config.hpp)

add_library(gfxplaycore SHARED
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
    src/runtime_config.hpp
    src/runtime_config.cpp
)
target_link_libraries(gfxplaycore stdc++fs gfxplay-all-dependencies)
target_include_directories(gfxplaycore PUBLIC
    ${CMAKE_BINARY_DIR}
)
target_compile_options(gfxplaycore PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        # force MSVC to obey ISO C++ (e.g. allow `and` keyword)
        /permissive-
    >

    # turn on warnings / errors
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall
        -Wextra
        -pedantic
        -Werror
        -Wuninitialized
        -Winit-self
        -Wmaybe-uninitialized
        -Wshadow
        -Wcast-align
        -Wcast-function-type
        -Wwrite-strings
        -Wdangling-else
        -Wdate-time
        -Wlogical-op
        -Wmissing-declarations
        -Wno-multichar
        -Wnormalized
        -Wredundant-decls
        #-Winline
        -Wint-in-bool-context
        -Wvla
        -Wdisabled-optimization
        -Wmissing-noreturn
        -Wsuggest-attribute=noreturn

        # disabled because it's a PITA to prototype with it enabled
        -Wno-unused

        # broken in STB headers
        -Wno-sign-compare

        # things I would like to enable:
        #
        # -Wcast-qual : broken by STB headers
        # -Wpacked : broken by ASSIMP
        # -Wconversion : hits shitloads of lines
    >
)
target_compile_features(gfxplaycore PUBLIC
    cxx_std_20
)
target_link_options(gfxplaycore PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        # disable opening a console window when booting normally
        /SUBSYSTEM:windows
        /ENTRY:mainCRTStartup
    >
)
set_target_properties(gfxplaycore PROPERTIES
    CXX_EXTENSIONS OFF
)


# https://learnopengl.com/Getting-started/Hello-Triangle
add_executable(logl_1_hellotriangle src/logl_1_hellotriangle.cpp)
target_link_libraries(logl_1_hellotriangle gfxplaycore)

# https://learnopengl.com/Getting-started/Shaders
add_executable(logl_2_shaders src/logl_2_shaders.cpp)
target_link_libraries(logl_2_shaders gfxplaycore)

# https://learnopengl.com/Getting-started/Textures
add_executable(logl_3_textures src/logl_3_textures.cpp)
target_link_libraries(logl_3_textures gfxplaycore)

# https://learnopengl.com/Getting-started/Transformations
add_executable(logl_4_transformations src/logl_4_transformations.cpp)
target_link_libraries(logl_4_transformations gfxplaycore)

# https://learnopengl.com/Getting-started/Coordinate-Systems
add_executable(logl_5_coordinate-systems src/logl_5_coordinate-systems.cpp)
target_link_libraries(logl_5_coordinate-systems gfxplaycore)

# https://learnopengl.com/Getting-started/Camera
add_executable(logl_6_camera src/logl_6_camera.cpp)
target_link_libraries(logl_6_camera gfxplaycore)

# https://learnopengl.com/Lighting/Colors
add_executable(logl_7_colors src/logl_7_colors.cpp)
target_link_libraries(logl_7_colors gfxplaycore)

# https://learnopengl.com/Lighting/Basic-Lighting
add_executable(logl_8_basic-lighting src/logl_8_basic-lighting.cpp)
target_link_libraries(logl_8_basic-lighting gfxplaycore)

# https://learnopengl.com/Lighting/Materials
add_executable(logl_9_materials src/logl_9_materials.cpp)
target_link_libraries(logl_9_materials gfxplaycore)

# https://learnopengl.com/Lighting/Lighting-maps
add_executable(logl_10_lighting-maps src/logl_10_lighting-maps.cpp)
target_link_libraries(logl_10_lighting-maps gfxplaycore)

# https://learnopengl.com/Lighting/Light-casters
add_executable(logl_11_light-casters src/logl_11_light-casters.cpp)
target_link_libraries(logl_11_light-casters gfxplaycore)

# https://learnopengl.com/Lighting/Multiple-lights
add_executable(logl_12_multiple-lights src/logl_12_multiple-lights.cpp)
target_link_libraries(logl_12_multiple-lights gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Depth-testing
add_executable(logl_depth-testing src/logl_depth-testing.cpp)
target_link_libraries(logl_depth-testing gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Stencil-testing
add_executable(logl_stencil-testing src/logl_stencil-testing.cpp)
target_link_libraries(logl_stencil-testing gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Framebuffers
add_executable(logl_framebuffers src/logl_framebuffers.cpp)
target_link_libraries(logl_framebuffers gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Blending
add_executable(logl_blending src/logl_blending.cpp)
target_link_libraries(logl_blending gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Face-culling
add_executable(logl_face-culling src/logl_face-culling.cpp)
target_link_libraries(logl_face-culling gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Geometry-Shader
add_executable(logl_geom-shader-normals src/logl_geom-shader-normals.cpp)
target_link_libraries(logl_geom-shader-normals gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps src/logl_cubemaps.cpp)
target_link_libraries(logl_cubemaps gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps_environment-mapping
    src/logl_cubemaps_environment-mapping.cpp
)
target_link_libraries(logl_cubemaps_environment-mapping gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Cubemaps
add_executable(logl_cubemaps_environment-mapping_refractions
    src/logl_cubemaps_environment-mapping_refractions.cpp
)
target_link_libraries(logl_cubemaps_environment-mapping_refractions gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Instancing
add_executable(logl_instancing_1_basic src/logl_instancing_1_basic.cpp)
target_link_libraries(logl_instancing_1_basic gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Instancing
add_executable(logl_instancing_2_instanced-arrays src/logl_instancing_2_instanced-arrays.cpp)
target_link_libraries(logl_instancing_2_instanced-arrays gfxplaycore)

# https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing
add_executable(logl_anti-aliasing-a-framebuffer src/logl_anti-aliasing-a-framebuffer.cpp)
target_link_libraries(logl_anti-aliasing-a-framebuffer gfxplaycore)

add_executable(logl_save-to-file src/logl_save-to-file.cpp)
target_link_libraries(logl_save-to-file gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
add_executable(logl_advanced-lighting
    src/logl_advanced-lighting.cpp
)
target_link_libraries(logl_advanced-lighting gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
add_executable(logl_gamma-correction
    src/logl_gamma-correction.cpp
)
target_link_libraries(logl_gamma-correction gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
add_executable(logl_shadows_shadow-mapping
    src/logl_shadows_shadow-mapping.cpp
)
target_link_libraries(logl_shadows_shadow-mapping gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
add_executable(logl_shadows_point-shadows
    src/logl_shadows_point-shadows.cpp
)
target_link_libraries(logl_shadows_point-shadows gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Normal-Mapping
add_executable(logl_normal-mapping
    src/logl_normal-mapping.cpp
)
target_link_libraries(logl_normal-mapping gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
add_executable(logl_parallax-mapping
    src/logl_parallax-mapping.cpp
)
target_link_libraries(logl_parallax-mapping gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/HDR
add_executable(logl_hdr
    src/logl_hdr.cpp
)
target_link_libraries(logl_hdr gfxplaycore)

# https://learnopengl.com/Advanced-Lighting/Bloom
add_executable(logl_bloom
    src/logl_bloom.cpp
)
target_link_libraries(logl_bloom gfxplaycore)

# personal experiment: selectable geometry
add_executable(ak_selectable-geom src/ak_selectable-geom.cpp)
target_link_libraries(ak_selectable-geom gfxplaycore)

if (GFXPLAY_USE_ASSIMP)

    # https://learnopengl.com/Model-Loading/Assimp
    add_executable(logl_13_model-loading
        src/logl_13_model-loading.cpp
        src/logl_model.hpp
    )
    target_link_libraries(logl_13_model-loading gfxplaycore gfxplay-assimp)

    # https://learnopengl.com/Advanced-OpenGL/Instancing
    add_executable(logl_instancing_3_planet
        src/logl_instancing_3_planet.cpp
    )
    target_link_libraries(logl_instancing_3_planet gfxplaycore gfxplay-assimp)

    # https://learnopengl.com/Advanced-Lighting/Deferred-Shading
    add_executable(logl_deferred-shading
        src/logl_deferred-shading.cpp
    )
    target_link_libraries(logl_deferred-shading gfxplaycore gfxplay-assimp)

    # https://learnopengl.com/Advanced-Lighting/SSAO
    add_executable(logl_ssao
        src/logl_ssao.cpp
    )
    target_link_libraries(logl_ssao gfxplaycore gfxplay-assimp)
endif()

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
