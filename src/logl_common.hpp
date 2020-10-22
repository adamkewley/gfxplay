#pragma once

#include "gfxplay_config.hpp"

#ifdef _MSC_VER
#pragma warning(disable: 26439) // "This kind of function may not throw. Declare it 'noexcept'
#pragma warning(disable: 4455) // "This kind of function may not throw. Declare it 'noexcept'
#endif

// wrappers
#include "sdl.hpp"
#include "gl.hpp"
#include "gl_extensions.hpp"

// imgui
#include "imgui.h"

// glm
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <exception>
#include <optional>
#include <vector>
#include <sstream>
#include <chrono>
#include <iostream>
#include <fstream>

using std::literals::string_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
constexpr float pi_f = static_cast<float>(M_PI);
constexpr double pi_d = M_PI;


// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

namespace ui {
    struct Window_state final {
        sdl::Context context = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
        sdl::Window window = [&]() {
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);
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

#ifndef NDEBUG
            fprintf(stderr,
                    "OpenGL info: %s: %s (%s) /w GLSL: %s\n",
                    glGetString(GL_VENDOR),
                    glGetString(GL_RENDERER),
                    glGetString(GL_VERSION),
                    glGetString(GL_SHADING_LANGUAGE_VERSION));
#endif

            OSC_GL_CALL_CHECK(glEnable, GL_DEPTH_TEST);
            OSC_GL_CALL_CHECK(glEnable, GL_BLEND);
            OSC_GL_CALL_CHECK(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            OSC_GL_CALL_CHECK(glEnable, GL_MULTISAMPLE);

            gl::assert_no_errors("ui::State::constructor::onExit");
        }
    };

    struct Persp_camera final {
        glm::vec3 pos = {0.0f, 0.0f, 0.0f};
        float pitch = 0.0f;
        float yaw = -pi_f/2.0f;

        glm::vec3 front() const {
            return glm::normalize(glm::vec3{
                cos(yaw) * cos(pitch),
                sin(pitch),
                sin(yaw) * cos(pitch),
            });
        }

        glm::vec3 up() const {
            return glm::vec3(0.0f, 1.0f, 0.0f);
        }

        glm::vec3 right() const {
            return glm::normalize(glm::cross(front(), up()));
        }

        glm::mat4 view_mtx() const {
            return glm::lookAt(pos, pos + front(), up());
        }

        glm::mat4 persp_mtx() const {
            return glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        }
    };

    enum class Handle_response {
        should_quit,
        ok,
    };

    struct Game_state final {
        Persp_camera camera;

        // world space per millisecond
        static constexpr float movement_speed = 0.03f;
        static constexpr float mouse_sensitivity = 0.001f;

        bool moving_forward = false;
        bool moving_backward = false;
        bool moving_left = false;
        bool moving_right = false;
        bool moving_up = false;
        bool moving_down = false;

        Handle_response handle(SDL_Event& e) {
            if (e.type == SDL_QUIT) {
                return Handle_response::should_quit;
            } else if (e.type == SDL_KEYDOWN or e.type == SDL_KEYUP) {
                bool is_button_down = e.type == SDL_KEYDOWN ? true : false;
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    moving_forward = is_button_down;
                    break;
                case SDLK_s:
                    moving_backward = is_button_down;
                    break;
                case SDLK_d:
                    moving_left = is_button_down;
                    break;
                case SDLK_a:
                    moving_right = is_button_down;
                    break;
                case SDLK_SPACE:
                    moving_up = is_button_down;
                    break;
                case SDLK_LCTRL:
                    moving_down = is_button_down;
                    break;
                case SDLK_ESCAPE:
                    return Handle_response::should_quit;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                camera.yaw += e.motion.xrel * mouse_sensitivity;
                camera.pitch -= e.motion.yrel * mouse_sensitivity;
                if (camera.pitch > pi_f/2.0f - 0.5f) {
                    camera.pitch = pi_f/2.0f - 0.5f;
                }
                if (camera.pitch < -pi_f/2.0f + 0.5f) {
                    camera.pitch = -pi_f/2.0f + 0.5f;
                }
            }

            return Handle_response::ok;
        }

        void tick(std::chrono::milliseconds const& dt) {
            float movement_amt = movement_speed * dt.count();

            if (moving_forward) {
                camera.pos += movement_amt * camera.front();
            }

            if (moving_backward) {
                camera.pos -= movement_amt * camera.front();
            }

            if (moving_left) {
                camera.pos += movement_amt * camera.right();
            }

            if (moving_right) {
                camera.pos -= movement_amt * camera.right();
            }

            if (moving_up) {
                camera.pos += movement_amt * camera.up();
            }

            if (moving_down) {
                camera.pos -= movement_amt * camera.up();
            }
        }
    };
}

namespace util {

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
