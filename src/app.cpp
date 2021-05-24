#include "app.hpp"

#include <SDL.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

class Stdout_sink final : public gp::log::Sink {
    std::mutex mutex;

    void log(gp::log::Msg const& msg) override {
        std::lock_guard g{mutex};
        std::cout << '[' << msg.loggerName << "] [" << gp::log::toStringView(msg.level) << "] " << msg.payload << std::endl;
    }
};

static std::shared_ptr<gp::log::Logger> create_default_sink() {
    return std::make_shared<gp::log::Logger>("default", std::make_shared<Stdout_sink>());
}

std::string_view const gp::log::level::g_nameViews[] LOG_LVL_NAMES;
char const* const gp::log::level::g_nameCStrings[] LOG_LVL_NAMES;
static std::shared_ptr<gp::log::Logger> default_sink = create_default_sink();

std::shared_ptr<gp::log::Logger> gp::log::defaultLogger() noexcept {
    return default_sink;
}

gp::log::Logger* gp::log::defaultLoggerRaw() noexcept {
    return defaultLogger().get();
}

void gp::onAssertFailed(char const* failingSourceCode, char const* file, unsigned int line) noexcept {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s:%u: an assertion failed: %s", file, line, failingSourceCode);

    try {
        throw std::runtime_error{buf};
    } catch (std::runtime_error const&) {
        std::terminate();
    }
}

struct SdlCtx final {
    SdlCtx(Uint32 flags) {
        if (SDL_Init(flags) != 0) {
            std::stringstream ss;
            ss << "SDL_Init failed: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }
    }
    SdlCtx(SdlCtx const&) = delete;
    SdlCtx(SdlCtx&&) = delete;
    SdlCtx& operator=(SdlCtx const&) = delete;
    SdlCtx& operator=(SdlCtx&&) = delete;
    ~SdlCtx() noexcept {
        SDL_Quit();
    }
};

struct SdlWindow final {
    SDL_Window* handle;

    SdlWindow(const char* title, int x, int y, int w, int h, Uint32 flags) :
        handle{SDL_CreateWindow(title, x, y, w, h, flags)} {

        if (!handle) {
            std::stringstream ss;
            ss << "SDL_CreateWindow failed: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }
    }
    SdlWindow(SdlWindow const&) = delete;
    SdlWindow(SdlWindow&&) = delete;
    SdlWindow& operator=(SdlWindow const&) = delete;
    SdlWindow& operator=(SdlWindow&&) = delete;
    ~SdlWindow() noexcept {
        SDL_DestroyWindow(handle);
    }

    constexpr operator SDL_Window*() noexcept {
        return handle;
    }
};

struct SdlGLContext final {
    SDL_GLContext handle;

    SdlGLContext(SdlWindow& w) : handle{SDL_GL_CreateContext(w)} {
        if (!handle) {
            std::stringstream ss;
            ss << "SDL_GL_CreateContext failed: " << SDL_GetError();
            throw std::runtime_error{std::move(ss).str()};
        }
    }
    SdlGLContext(SdlGLContext const&) = delete;
    SdlGLContext(SdlGLContext&& tmp) noexcept : handle{tmp.handle} {
        tmp.handle = nullptr;
    }
    SdlGLContext& operator=(SdlGLContext const&) = delete;
    SdlGLContext& operator=(SdlGLContext&&) = delete;
    ~SdlGLContext() noexcept {
        if (handle) {
            SDL_GL_DeleteContext(handle);
        }
    }

    constexpr operator SDL_GLContext() noexcept {
        return handle;
    }
};

// convenience macro for setting SDL attrs
#define GP_SDL_GL_SetAttribute_CHECK(attr, value)                                                                     \
    {                                                                                                                  \
        if (SDL_GL_SetAttribute((attr), (value)) != 0) {                                                               \
            std::stringstream ss; \
            ss << "SDL_GL_SetAttribute failed when setting " #attr " = " #value " : " \
               << SDL_GetError(); \
            throw std::runtime_error{std::move(ss).str()}; \
        } \
    }

static SdlWindow initMainWindow() {
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);    
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_DEPTH_SIZE, 24);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_STENCIL_SIZE, 8);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLEBUFFERS, 1);
    GP_SDL_GL_SetAttribute_CHECK(SDL_GL_MULTISAMPLESAMPLES, 16);

    return SdlWindow{
        "windowname",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    };
}

static SdlGLContext initWindowOpenGLContext(SdlWindow& window) {
    SdlGLContext rv{window};

    // enable the context
    if (SDL_GL_MakeCurrent(window, rv) != 0) {
        std::stringstream ss;
        ss << "SDL_GL_MakeCurrent failed: " << SDL_GetError();
        throw std::runtime_error{std::move(ss).str()};
    }

    // vsync
    if (SDL_GL_SetSwapInterval(-1) != 0) {
        SDL_GL_SetSwapInterval(1);
    }

    // GLEW
    if (auto err = glewInit(); err != GLEW_OK) {
        std::stringstream ss;
        ss << "glewInit() failed: " << glewGetErrorString(err);
        throw std::runtime_error{std::move(ss).str()};
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);

    return rv;
}

struct gp::App::Impl final {
    SdlCtx sdlctx{SDL_INIT_VIDEO};
    SdlWindow window = initMainWindow();
    SdlGLContext sdlglctx = initWindowOpenGLContext(window);
};

gp::App* gp::App::g_Current = nullptr;

gp::App::App() : impl{new Impl{}} {
    App::g_Current = this;
}

gp::App::~App() noexcept {
    delete impl;
}

void gp::App::show(std::unique_ptr<Screen> screenptr) {
    Screen& screen = *screenptr;

    screen.onMount();
    GP_SCOPEGUARD({ screen.onUnmount(); });

    auto prev = std::chrono::high_resolution_clock::now();
    dms lag;

    while (true) {
        // handle clocks
        auto cur = std::chrono::high_resolution_clock::now();
        dms elapsed = std::chrono::duration_cast<dms>(cur - prev);
        prev = cur;
        lag += elapsed;

        // pump events
        for (SDL_Event e; SDL_PollEvent(&e);) {
            if (e.type == SDL_QUIT) {
                return;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                return;
            }
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int w, h;
                SDL_GetWindowSize(impl->window, &w, &h);
                glViewport(0, 0, w, h);
            }

            screen.onEvent(e);
        }

        // update (fixed step)
        while (lag >= GP_DMS_PER_UPDATE) {
            screen.onUpdate();
            lag -= GP_DMS_PER_UPDATE;
        }

        // render
        screen.onDraw(lag / GP_DMS_PER_UPDATE);

        // present
        SDL_GL_SwapWindow(impl->window);
    }
}

gp::WindowDimensions gp::App::windowDimensions() const noexcept {
    int w, h;
    SDL_GetWindowSize(impl->window, &w, &h);
    return {w, h};
}

void* gp::App::windowRAW() noexcept {
    return impl->window.handle;
}

void* gp::App::glRAW() noexcept {
    return impl->sdlglctx.handle;
}

void gp::App::hideCursor() noexcept {
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void gp::ImGuiInit() {
    ImGui::CreateContext();

    App& app = App::cur();
    SDL_Window* window = static_cast<SDL_Window*>(app.windowRAW());
    SDL_GLContext gl = static_cast<SDL_GLContext>(app.glRAW());
    ImGui_ImplSDL2_InitForOpenGL(window, gl);

    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void gp::ImGuiShutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void gp::ImGuiNewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(static_cast<SDL_Window*>(App::cur().windowRAW()));
    ImGui::NewFrame();
}

void gp::ImGuiRender() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

glm::vec3 gp::Euler_perspective_camera::front() const noexcept {
    return glm::normalize(glm::vec3{
        cos(yaw) * cos(pitch),
        sin(pitch),
        sin(yaw) * cos(pitch),
    });
}

[[nodiscard]] glm::vec3 gp::Euler_perspective_camera::up() const noexcept {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

[[nodiscard]] glm::vec3 gp::Euler_perspective_camera::right() const noexcept {
    return glm::normalize(glm::cross(front(), up()));
}

[[nodiscard]] glm::mat4 gp::Euler_perspective_camera::viewMatrix() const noexcept {
    return glm::lookAt(pos, pos + front(), up());
}

[[nodiscard]] glm::mat4 gp::Euler_perspective_camera::projectionMatrix(float aspectRatio) const noexcept {
    return glm::perspective(glm::radians(fov), aspectRatio, znear, zfar);
}

bool gp::Euler_perspective_camera::onEvent(SDL_KeyboardEvent const& e) noexcept {
    bool is_pressed = e.state == SDL_PRESSED;

    switch (e.keysym.sym) {
    case SDLK_w:
        moving_forward = is_pressed;
        return true;
    case SDLK_s:
        moving_backward = is_pressed;
        return true;
    case SDLK_d:
        moving_left = is_pressed;
        return true;
    case SDLK_a:
        moving_right = is_pressed;
        return true;
    case SDLK_SPACE:
        moving_up = is_pressed;
        return true;
    case SDLK_LCTRL:
        moving_down = is_pressed;
        return true;
    }

    return false;
}

// world space per millisecond
static constexpr float movement_speed = 0.03f;
static constexpr float mouse_sensitivity = 0.001f;

bool gp::Euler_perspective_camera::onEvent(SDL_MouseMotionEvent const& e) noexcept {
    yaw += mouse_sensitivity * e.xrel;
    pitch -= mouse_sensitivity * e.yrel;

    if (pitch > pi_f/2.0f - 0.5f) {
        pitch = pi_f/2.0f - 0.5f;
    }

    if (pitch < -pi_f/2.0f + 0.5f) {
        pitch = -pi_f/2.0f + 0.5f;
    }

    return true;
}

void gp::Euler_perspective_camera::onUpdate() {
    float amt = GP_MS_PER_UPDATE * movement_speed;

    if (moving_forward) {
        pos += amt * front();
    }

    if (moving_backward) {
        pos -= amt * front();
    }

    if (moving_left) {
        pos += amt * right();
    }

    if (moving_right) {
        pos -= amt * right();
    }

    if (moving_up) {
        pos += amt * up();
    }

    if (moving_down) {
        pos -= amt * up();
    }
}
