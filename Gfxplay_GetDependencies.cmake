include(ExternalProject)

# set GFXPLAY_DEPENDENCY_CMAKE_ARGS
if(TRUE)
    if(CMAKE_CXX_COMPILER)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER})
    endif()
    if(CMAKE_C_COMPILER)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER})
    endif()
    if(CMAKE_CXX_FLAGS)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS})
    endif()
    if(CMAKE_CXX_FLAGS_DEBUG)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG})
    endif()
    if(CMAKE_CXX_FLAGS_MINSIZEREL)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL})
    endif()
    if(CMAKE_CXX_FLAGS_RELEASE)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE})
    endif()
    if(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    endif()
    if(CMAKE_C_FLAGS)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS})
    endif()
    if(CMAKE_C_FLAGS_DEBUG)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG})
    endif()
    if(CMAKE_C_FLAGS_MINSIZEREL)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL})
    endif()
    if(CMAKE_C_FLAGS_RELEASE)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE})
    endif()
    if(CMAKE_C_FLAGS_RELWITHDEBINFO)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO})
    endif()
    if(CMAKE_BUILD_TYPE)
        list(APPEND GFXPLAY_DEPENDENCY_CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
    endif()
endif()

# find OpenGL
find_package(OpenGL REQUIRED)

# TARGET: gfxplay-glew
if(TRUE)
    set(GLEW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glew)

    # get version from config/version
    if(TRUE)
        file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_MAJOR_STR REGEX "GLEW_MAJOR[ ]*=[ ]*[0-9]+.*")
        string(REGEX REPLACE "GLEW_MAJOR[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_MAJOR ${GLEW_VERSION_MAJOR_STR})
        file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_MINOR_STRING REGEX "GLEW_MINOR[ ]*=[ ]*[0-9]+.*")
        string(REGEX REPLACE "GLEW_MINOR[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_MINOR ${GLEW_VERSION_MINOR_STRING})
        file(STRINGS ${GLEW_DIR}/config/version GLEW_VERSION_PATCH_STRING REGEX "GLEW_MICRO[ ]*=[ ]*[0-9]+.*")
        string(REGEX REPLACE "GLEW_MICRO[ ]*=[ ]*([0-9]+)" "\\1" GLEW_VERSION_PATCH ${GLEW_VERSION_PATCH_STRING})

        set(GLEW_VERSION ${GLEW_VERSION_MAJOR}.${GLEW_VERSION_MINOR}.${GLEW_VERSION_PATCH})

        unset(GLEW_VERSION_MAJOR_STR)
        unset(GLEW_VERSION_MAJOR)
        unset(GLEW_VERSION_MINOR_STR)
        unset(GLEW_VERSION_MINOR)
        unset(GLEW_VERSION_PATCH_STR)
        unset(GLEW_VERSION_PATCH)
    endif()

    set(GLEW_SRC_FILES ${GLEW_DIR}/src/glew.c)
    set(GLEW_PUBLIC_HEADER_FILES
        ${GLEW_DIR}/include/GL/wglew.h
        ${GLEW_DIR}/include/GL/glew.h
        ${GLEW_DIR}/include/GL/glxew.h
    )
    if(WIN32)
        list(APPEND GLEW_SRC_FILES ${GLEW_DIR}/build/glew.rc)
    endif()

    add_library(gfxplay-glew STATIC ${GLEW_PUBLIC_HEADER_FILES} ${GLEW_SRC_FILES})
    target_include_directories(gfxplay-glew PUBLIC ${GLEW_DIR}/include/)
    target_compile_definitions(gfxplay-glew PRIVATE -DGLEW_NO_GLU)
    target_link_libraries(gfxplay-glew PUBLIC ${OPENGL_LIBRARIES})
    set_target_properties(gfxplay-glew PROPERTIES
        VERSION ${GLEW_VERSION}
        COMPILE_DEFINITIONS "GLEW_STATIC"
        INTERFACE_INCLUDE_DIRECTORIES ${GLEW_DIR}/include
        INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC
        POSITION_INDEPENDENT_CODE ON
    )

    # kill security checks which are dependent on stdlib
    if(MSVC)
        target_compile_definitions(gfxplay-glew PRIVATE "GLEW_STATIC;VC_EXTRALEAN")
        target_compile_options(gfxplay-glew PRIVATE -GS-)
    elseif (WIN32 AND ((CMAKE_C_COMPILER_ID MATCHES "GNU") OR (CMAKE_C_COMPILER_ID MATCHES "Clang")))
        target_compile_options (glew_s PRIVATE -fno-builtin -fno-stack-protector)
    endif()

    unset(GLEW_DIR)
    unset(GLEW_VERSION)
    unset(GLEW_SRC_FILES)
    unset(GLEW_PUBLIC_HEADER_FILES)
endif()

# TARGET: gfxplay-sdl2
if(TRUE)

    if(UNIX AND NOT APPLE)  # i.e. linux
        find_library(GFXPLAY_SDL2_LOCATION SDL2 REQUIRED)
        add_library(gfxplay-sdl2 SHARED IMPORTED)
        set_target_properties(gfxplay-sdl2 PROPERTIES
            IMPORTED_LOCATION ${GFXPLAY_SDL2_LOCATION}
            INTERFACE_INCLUDE_DIRECTORIES /usr/include/SDL2
        )
    else()
        # on non-Linux, build SDL from source and package it with the install
        ExternalProject_Add(sdl2-project
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2
            CMAKE_CACHE_ARGS ${GFXPLAY_DEPENDENCY_CMAKE_ARGS}
            INSTALL_COMMAND ""
            EXCLUDE_FROM_ALL TRUE
            UPDATE_DISCONNECTED ON
            # HACK: this is specifically required by Ninja, because it
            # needs to know the side-effects of external build steps
            BUILD_BYPRODUCTS
            "sdl2-project-prefix/src/sdl2-project-build/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX};sdl2-project-prefix/src/sdl2-project-build/libSDL2-2.0d${CMAKE_SHARED_LIBRARY_SUFFIX}"
        )
        ExternalProject_Get_Property(sdl2-project SOURCE_DIR)
        ExternalProject_Get_Property(sdl2-project BINARY_DIR)

        # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
        file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

        add_library(gfxplay-sdl2 SHARED IMPORTED)
        add_dependencies(gfxplay-sdl2 sdl2-project)

        if(WIN32)
            set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2)
            set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2d)
        elseif(APPLE)
            set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0)
            set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2)
        else()
            set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0)
            set(DEBUG_LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}SDL2-2.0d)
        endif()

        if(CMAKE_BUILD_TYPE MATCHES Debug)
            set(SDL2_LIB_SUFFIX "d")
        else()
            set(SDL2_LIB_SUFFIX "")
        endif()

        set_target_properties(gfxplay-sdl2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include
        )

        if(${GFXPLAY_GENERATOR_IS_MULTI_CONFIG})
            set_target_properties(gfxplay-sdl2 PROPERTIES
                IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
                IMPORTED_IMPLIB_DEBUG ${BINARY_DIR}/Debug/${DEBUG_LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
                IMPORTED_LOCATION_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
                IMPORTED_IMPLIB_RELWITHDEBINFO ${BINARY_DIR}/RelWithDebInfo/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
                IMPORTED_LOCATION_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
                IMPORTED_IMPLIB_MINSIZEREL ${BINARY_DIR}/MinSizeRel/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
                IMPORTED_LOCATION_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
                IMPORTED_IMPLIB_RELEASE ${BINARY_DIR}/Release/${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
            )
        else()
            set_target_properties(gfxplay-sdl2 PROPERTIES
                IMPORTED_LOCATION ${BINARY_DIR}/${LIBNAME}${SDL2_LIB_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
            )
        endif()

        unset(SDL2_LIB_SUFFIX)
        unset(SOURCE_DIR)
        unset(BINARY_DIR)
        unset(LIBNAME)
        unset(DEBUG_LIBNAME)
    endif()

endif()

# TARGET: gfxplay-glm
if(TRUE)
    add_library(gfxplay-glm INTERFACE)
    target_include_directories(gfxplay-glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
endif()

# TARGET: gfxplay-imgui
if(TRUE)
    add_library(gfxplay-imgui STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_demo.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_sdl.cpp
    )
    target_link_libraries(gfxplay-imgui PUBLIC gfxplay-sdl2 gfxplay-glew gfxplay-glm)
    target_include_directories(gfxplay-imgui PUBLIC third_party/ third_party/imgui/)
    set_target_properties(gfxplay-imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# TARGET: gfxplay-stb-image
if(TRUE)
    add_library(gfxplay-stb INTERFACE)
    target_include_directories(gfxplay-stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
endif()

# TARGET: gfxplay-assimp
if(GFXPLAY_USE_ASSIMP)

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(ASSIMP_LIB_SUFFIX "d")
    else()
        set(ASSIMP_LIB_SUFFIX "")
    endif()

    ExternalProject_Add(assimp-project
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp
        CMAKE_CACHE_ARGS ${GFXPLAY_DEPENDENCY_CMAKE_ARGS}
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS "assimp-project-prefix/src/assimp-project-build/code/libassimp${ASSIMP_LIB_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
    ExternalProject_Get_Property(assimp-project SOURCE_DIR)
    ExternalProject_Get_Property(assimp-project BINARY_DIR)

    # HACK: see: https://gitlab.kitware.com/cmake/cmake/-/issues/15052
#    file(MAKE_DIRECTORY ${SOURCE_DIR}/include)
    file(MAKE_DIRECTORY ${BINARY_DIR}/include)

    add_library(gfxplay-assimp SHARED IMPORTED)
    add_dependencies(gfxplay-assimp assimp-project)
    set_target_properties(gfxplay-assimp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}/include;${BINARY_DIR}/include"
    )
    set_target_properties(gfxplay-assimp PROPERTIES
        IMPORTED_LOCATION ${BINARY_DIR}/code/libassimp${ASSIMP_LIB_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )

    unset(SOURCE_DIR)
    unset(BINARY_DIR)
endif()

# TARGET: gfxplay-entt
if(TRUE)
    add_library(gfxplay-entt INTERFACE)
    target_include_directories(gfxplay-entt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/entt)
endif()

# TARGET: gfxplay-all-dependencies
if(TRUE)
    add_library(gfxplay-all-dependencies INTERFACE)
    target_link_libraries(gfxplay-all-dependencies INTERFACE
        gfxplay-glew
        gfxplay-sdl2
        gfxplay-glm
        gfxplay-imgui
        gfxplay-stb
        gfxplay-entt
        ${OPENGL_LIBRARIES}
    )
endif()
