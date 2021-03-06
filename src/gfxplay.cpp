#ifdef _MSC_VER
#pragma warning(disable: 26439) // "This kind of function may not throw. Declare it 'noexcept'
#pragma warning(disable: 4455) // "This kind of function may not throw. Declare it 'noexcept'
#endif

// wrappers
#include "sdl.hpp"
#include "gl.hpp"
#include "stbi.hpp"
#include "glglm.hpp"

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
#define OSC_GLSL_VERSION "#version 150"
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

namespace {
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

    struct Gl_State final {
        gl::Program prog = []() {
            auto p = gl::Program();
            auto vs = gl::Vertex_shader::Compile(OSC_GLSL_VERSION R"(
in vec3 aPos;

void main() {
gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)");
            auto fs = gl::Fragment_shader::Compile(OSC_GLSL_VERSION R"(
out vec4 FragColor;

void main() {
FragColor = vec4(1.0, 0.5f, 0.2f, 1.0f);
}
)");
            gl::AttachShader(p, vs);
            gl::AttachShader(p, fs);
            gl::LinkProgram(p);
            return p;
        }();

        gl::Attribute aPos = gl::Attribute{prog, "aPos"};
        gl::Array_buffer ab = {};
        gl::Element_array_buffer ebo = {};
        gl::Vertex_array vao = {};

        Gl_State() {
            float vertices[] = {
                 0.5f,  0.5f, 0.0f,  // top right
                 0.5f, -0.5f, 0.0f,  // bottom right
                -0.5f, -0.5f, 0.0f,  // bottom left
                -0.5f,  0.5f, 0.0f   // top left
            };
            unsigned int indices[] = {  // note that we start from 0!
                0, 1, 3,   // first triangle
                1, 2, 3    // second triangle
            };

            gl::BindVertexArray(vao);
            gl::BindBuffer(ab);
            gl::BufferData(ab, sizeof(vertices), vertices, GL_STATIC_DRAW);
            gl::BindBuffer(ebo);
            gl::BufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);
            gl::VertexAttributePointer(aPos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    auto s = Window_state{};
    auto gls = Gl_State{};

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // ImGUI
    auto imgui_ctx = ig::Context{};
    auto imgui_sdl2_ctx = ig::SDL2_Context{s.window, s.gl};
    auto imgui_ogl3_ctx = ig::OpenGL3_Context{OSC_GLSL_VERSION};
    ImGui::StyleColorsLight();
    ImGuiIO& io = ImGui::GetIO();

    auto last_render_timepoint = std::chrono::high_resolution_clock::now();
    auto min_delay_between_frames = 8ms;

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl::UseProgram(gls.prog);
        gl::BindVertexArray(gls.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        gl::BindVertexArray();
        gl::UseProgram();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(s.window);

        ImGui::NewFrame();
        bool b;
        ImGui::Begin("Scene", &b, ImGuiWindowFlags_MenuBar);
        {
            std::stringstream fps;
            fps << "Fps: " << io.Framerate;
            ImGui::Text(fps.str().c_str());
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // software-throttle the framerate: no need to render at an insane
        // (e.g. 2000 FPS, on my machine) FPS, but do not use VSYNC because
        // it makes the entire application feel *very* laggy.
        auto now = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_timepoint);
        if (dt < min_delay_between_frames) {
            SDL_Delay(static_cast<Uint32>((min_delay_between_frames - dt).count()));
        }

        // draw
        SDL_GL_SwapWindow(s.window);
        last_render_timepoint = std::chrono::high_resolution_clock::now();
    }
}
