#pragma once

#include "gfxplay_config.hpp"

#ifdef _MSC_VER
#pragma warning(disable: 26439) // "This kind of function may not throw. Declare it 'noexcept'
#pragma warning(disable: 4455) // "This kind of function may not throw. Declare it 'noexcept'
#endif

// wrappers
#include "sdl.hpp"
#include "gl.hpp"
#include "stbi.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui
#include "imgui.h"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

#include <string>
#include <exception>
#include <optional>
#include <vector>
#include <sstream>
#include <chrono>
#include <iostream>

using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
constexpr float pi_f = static_cast<float>(M_PI);
constexpr double pi_d = M_PI;


// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// platform-specific bs
#ifdef __APPLE__
#define OSC_GLSL_VERSION "#version 150"
#define OSC_GL_CTX_FLAGS SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#define OSC_GL_CTX_MAJOR_VERSION 3
#define OSC_GL_CTX_MINOR_VERSION 2
#else
#define OSC_GLSL_VERSION "#version 330 core"
#define OSC_GL_CTX_FLAGS 0
#define OSC_GL_CTX_MAJOR_VERSION 3
#define OSC_GL_CTX_MINOR_VERSION 0
#endif

// macros for quality-of-life checks
#define OSC_SDL_GL_SetAttribute_CHECK(attr, value) { \
        int rv = SDL_GL_SetAttribute((attr), (value)); \
        if (rv != 0) { \
            throw std::runtime_error{"SDL_GL_SetAttribute failed when setting " #attr " = " #value " : "s + SDL_GetError()}; \
        } \
    }
#define OSC_GL_CALL_CHECK(func, ...) { \
        func(__VA_ARGS__); \
        gl::assert_no_errors(#func); \
    }
#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...)
#else
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#endif

namespace glm {
    std::ostream& operator<<(std::ostream& o, vec3 const& v) {
        return o << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    }

    std::ostream& operator<<(std::ostream& o, vec4 const& v) {
        return o << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    }

    std::ostream& operator<<(std::ostream& o, mat4 const& m) {
        o << "[";
        for (auto i = 0U; i < 3; ++i) {
            o << m[i];
            o << ", ";
        }
        o << m[3];
        o << "]";
        return o;
    }
}

namespace ig {
    struct Context final {
        Context() {
            ImGui::CreateContext();
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            ImGui::DestroyContext();
        }
    };

    struct SDL2_Context final {
        SDL2_Context(SDL_Window* w, SDL_GLContext gl) {
            ImGui_ImplSDL2_InitForOpenGL(w, gl);
        }
        SDL2_Context(SDL2_Context const&) = delete;
        SDL2_Context(SDL2_Context&&) = delete;
        SDL2_Context& operator=(SDL2_Context const&) = delete;
        SDL2_Context& operator=(SDL2_Context&&) = delete;
        ~SDL2_Context() noexcept {
            ImGui_ImplSDL2_Shutdown();
        }
    };

    struct OpenGL3_Context final {
        OpenGL3_Context(char const* version) {
            ImGui_ImplOpenGL3_Init(version);
        }
        OpenGL3_Context(OpenGL3_Context const&) = delete;
        OpenGL3_Context(OpenGL3_Context&&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context const&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context&&) = delete;
        ~OpenGL3_Context() noexcept {
            ImGui_ImplOpenGL3_Shutdown();
        }
    };
}

namespace ui {
    struct Window_state final {
        sdl::Context context = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
        sdl::Window window = [&]() {
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, OSC_GL_CTX_FLAGS);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, OSC_GL_CTX_FLAGS);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, OSC_GL_CTX_MAJOR_VERSION);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, OSC_GL_CTX_MINOR_VERSION);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_DEPTH_SIZE, 24);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_STENCIL_SIZE, 8);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLEBUFFERS, 1);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLESAMPLES, 16);

            return sdl::CreateWindoww(
                "gfxplay",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                1024,
                768,
                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        }();
        sdl::GLContext gl = sdl::GL_CreateContext(window);

        Window_state() {
            gl::assert_no_errors("ui::State::constructor::onEnter");
            sdl::GL_SetSwapInterval(0);  // disable VSYNC

            // enable SDL's OpenGL context
            if (SDL_GL_MakeCurrent(window, gl) != 0) {
                throw std::runtime_error{"SDL_GL_MakeCurrent failed: "s  + SDL_GetError()};
            }

            // initialize GLEW, which is what imgui is using under the hood
            if (auto err = glewInit(); err != GLEW_OK) {
                std::stringstream ss;
                ss << "glewInit() failed: ";
                ss << glewGetErrorString(err);
                throw std::runtime_error{ss.str()};
            }

            DEBUG_PRINT("OpenGL info: %s: %s (%s) /w GLSL: %s\n",
                        glGetString(GL_VENDOR),
                        glGetString(GL_RENDERER),
                        glGetString(GL_VERSION),
                        glGetString(GL_SHADING_LANGUAGE_VERSION));

            OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);
            OSC_GL_CALL_CHECK(glEnable, GL_BLEND);
            OSC_GL_CALL_CHECK(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);

            gl::assert_no_errors("ui::State::constructor::onExit");
        }
    };
}

namespace util {
    void TexImage2D(gl::Texture_2d& t, GLint mipmap_lvl, stbi::Image const& image) {
        stbi_set_flip_vertically_on_load(true);
        gl::BindTexture(t);
        glTexImage2D(GL_TEXTURE_2D,
                     mipmap_lvl,
                     GL_RGB,
                     image.width,
                     image.height,
                     0,
                     image.nrChannels == 3 ? GL_RGB : GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     image.data);
    }

    gl::Texture_2d mipmapped_texture(char const* path) {
        auto t = gl::Texture_2d{};
        auto img = stbi::Image{path};
        TexImage2D(t, 0, img);
        gl::GenerateMipMap(t);
        return t;
    }

    void Uniform(gl::UniformMatrix4fv& u, glm::mat4 const& mat) {
        gl::Uniform(u, glm::value_ptr(mat));
    }

    void Uniform(gl::UniformVec4f& u, glm::vec4 const& v) {
        glUniform4f(u, v.x, v.y, v.z, v.w);
    }

    void Uniform(gl::UniformVec3f& u, glm::vec3 const& v) {
        glUniform3f(u, v.x, v.y, v.z);
    }

    void Uniform(gl::UniformMatrix3fv& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u, 1, false, glm::value_ptr(mat));
    }

    std::chrono::milliseconds now() {
        // milliseconds is grabbed from SDL to ensure the clocks used by the UI
        // (e.g. SDL_Delay, etc.) match
        return std::chrono::milliseconds{SDL_GetTicks()};
    }

    struct Software_throttle final {
        std::chrono::milliseconds last = util::now();
        std::chrono::milliseconds min_wait;

        Software_throttle(std::chrono::milliseconds _min_wait) :
            min_wait{_min_wait} {
        }

        void wait() {
            // software-throttle the framerate: no need to render at an insane
            // (e.g. 2000 FPS, on my machine) FPS, but do not use VSYNC because
            // it makes the entire application feel *very* laggy.
            auto now = util::now();
            auto dt = now - last;
            if (dt < min_wait) {
                auto rem = min_wait - dt;
                SDL_Delay(static_cast<Uint32>(rem.count()));
            }
            last = util::now();
        }
    };
}
