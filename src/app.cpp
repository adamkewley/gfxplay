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

// `App` also maintains these
//
// they're needed because purely asking for the mouse state (via SDL_GetMouseState)
// does provide the latest (lowest-latency) state, but might miss rapid clicks that
// toggle the state on+off before update is called
static bool g_MousePressedInEvent[3] = {false, false, false};

static void updateMousePosAndButtons(gp::Io& io, SDL_Window* window) {
    // update `MousePosPrevious`
    io.MousePosPrevious = io.MousePos;

    // update `MousePressed[3]`
    glm::ivec2 mouseLocal;
    Uint32 mouseState = SDL_GetMouseState(&mouseLocal.x, &mouseLocal.y);
    io.MousePressed[0] = g_MousePressedInEvent[0] || mouseState & SDL_BUTTON(SDL_BUTTON_LEFT);
    g_MousePressedInEvent[0] = false;
    io.MousePressed[1] = g_MousePressedInEvent[1] || mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT);
    g_MousePressedInEvent[1] = false;
    io.MousePressed[2] = g_MousePressedInEvent[2] || mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE);
    g_MousePressedInEvent[2] = false;

    // compute `MousePos`
    //
    // this is a little uglier than just querying it from GetMouseState because
    // the mouse *heavily* affects how laggy the UI feels and other behavior
    // like whether the mouse should work when another window is focused is
    // important
    static bool mouseCanUseGlobalState = strncmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0;
    bool curWindowHasFocus = SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS;
    if (curWindowHasFocus) {
        // SDL_GetMouseGlobalState typically gives better mouse positions than
        // the other methods because it uses a direct OS query for the mouse
        // position (whereas the other method rely on the event queue, which can
        // lag a little on Windows)
        if (mouseCanUseGlobalState) {
            glm::ivec2 mouseGlobal;
            SDL_GetGlobalMouseState(&mouseGlobal.x, &mouseGlobal.y);
            glm::ivec2 mouseWindow;
            SDL_GetWindowPosition(window, &mouseWindow.x, &mouseWindow.y);

            io.MousePos = mouseGlobal - mouseWindow;
        } else {
            io.MousePos = mouseLocal;
        }
    }

    io.MousePosDelta = io.MousePos - io.MousePosPrevious;
}

static void updateIOPoller(gp::Io& io, SDL_Window* window) {
    GP_ASSERT(window != nullptr && "application is not initialized correctly: not showing a window?");

    // update DisplaySize
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    io.DisplaySize = {w, h};

    // update Ticks and DeltaTime (ctor initializes TickFrequency, which should not change)
    uint64_t currentTicks = SDL_GetPerformanceCounter();
    double dTicks = static_cast<double>(currentTicks - io.Ticks);
    io.DeltaTime = static_cast<float>(dTicks/static_cast<double>(io.TickFrequency));
    io.Ticks = currentTicks;

    // update mouse
    updateMousePosAndButtons(io, window);
}

static gp::Io initIOPoller(SDL_Window* window) {
    gp::Io rv;
    rv.Ticks = SDL_GetPerformanceCounter();
    rv.TickFrequency = SDL_GetPerformanceFrequency();

    updateIOPoller(rv, window);

    return rv;
}

struct gp::App::Impl final {
    SdlCtx sdlctx{SDL_INIT_VIDEO};
    SdlWindow window = initMainWindow();
    SdlGLContext sdlglctx = initWindowOpenGLContext(window);
    Io io = initIOPoller(window);
    bool quit = false;
};

gp::App* gp::App::g_Current = nullptr;
gp::Io* gp::App::g_CurrentIO = nullptr;

gp::App::App() : impl{new Impl{}} {
    App::g_Current = this;
    App::g_CurrentIO = &this->impl->io;
}

gp::App::~App() noexcept {
    delete impl;
}

void gp::App::show(std::unique_ptr<Screen> screenptr) {
    Screen& screen = *screenptr;

    screen.onMount();
    GP_SCOPEGUARD({ screen.onUnmount(); });

    while (!impl->quit) {

        // pump events
        for (SDL_Event e; SDL_PollEvent(&e);) {

            // top-level (pre-Screen) event handling
            //
            // this maintains some app-level state (Io polling, OpenGL)
            switch (e.type) {
            case SDL_QUIT:
                // quit application
                return;
            case SDL_MOUSEBUTTONDOWN:
            {
                // set flags that help the IO poller figure out whether the
                // user clicked a mouse button during a step

                if (e.button.button == SDL_BUTTON_LEFT) {
                    g_MousePressedInEvent[0] = true;
                }

                if (e.button.button == SDL_BUTTON_RIGHT) {
                    g_MousePressedInEvent[1] = true;
                }

                if (e.button.button == SDL_BUTTON_MIDDLE) {
                    g_MousePressedInEvent[2] = true;
                }
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                // update IO poller keyboard state

                int scanCode = e.key.keysym.scancode;
                impl->io.KeysDown[scanCode] = e.type == SDL_KEYDOWN;
                impl->io.ShiftDown = SDL_GetModState() & KMOD_SHIFT;
                impl->io.CtrlDown = SDL_GetModState() & KMOD_CTRL;
                impl->io.AltDown = SDL_GetModState() & KMOD_ALT;
                break;
            }
            case SDL_WINDOWEVENT:
            {
                // update OpenGL viewport if window was resized

                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int w, h;
                    SDL_GetWindowSize(impl->window, &w, &h);
                    glViewport(0, 0, w, h);
                }
                break;
            }
            }

            screen.onEvent(e);
        }

        // update IO state
        //
        // this assumes all events are processed and that the current screen will
        // expect the IO poller to be in the correct state (deltas, etc.)
        updateIOPoller(impl->io, impl->window);

        // update screen
        //
        // the screen's onUpdate may indirectly access App/IO (e.g. to check if the
        // user is clicking anything, moving mouse, etc.)
        screen.onUpdate();

        // render screen
        //
        // draws the screen onto the currently-bound (assumed, window) framebuffer
        screen.onDraw();

        // present screen
        //
        // effectively, flips the rendered image onto the displayed window
        SDL_GL_SwapWindow(impl->window);
    }
}

void* gp::App::windowRAW() noexcept {
    return impl->window.handle;
}

void* gp::App::glRAW() noexcept {
    return impl->sdlglctx.handle;
}

void gp::App::enableRelativeMouseMode() noexcept {
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void gp::App::setMousePos(glm::ivec2 pos) noexcept {
    SDL_WarpMouseInWindow(impl->window, pos.x, pos.y);
}

void gp::App::requestQuit() noexcept {
    impl->quit = true;
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

bool gp::ImGuiOnEvent(SDL_Event const& e) noexcept {
    ImGui_ImplSDL2_ProcessEvent(&e);

    auto const& io = ImGui::GetIO();

    if (io.WantCaptureKeyboard && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)) {
        return true;
    }

    if (io.WantCaptureMouse && (e.type == SDL_MOUSEWHEEL || e.type == SDL_MOUSEMOTION ||
                                e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN)) {
        return true;
    }

    return false;
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

glm::mat4 gp::Euler_perspective_camera::viewMatrix() const noexcept {
    return glm::lookAt(pos, pos + front(), up());
}

glm::mat4 gp::Euler_perspective_camera::projectionMatrix(float aspectRatio) const noexcept {
    return glm::perspective(fov, aspectRatio, znear, zfar);
}

void gp::Euler_perspective_camera::onUpdateAsGrabbedCamera(float speed, float sensitivity) {
    auto& io = gp::App::IO();

    if (io.KeysDown[SDL_SCANCODE_ESCAPE]) {
        gp::App::cur().requestQuit();
    }

    if (io.KeysDown[SDL_SCANCODE_W]) {
        this->pos += speed * this->front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_S]) {
        this->pos -= speed * this->front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_A]) {
        this->pos -= speed * this->right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_D]) {
        this->pos += speed * this->right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_SPACE]) {
        this->pos += speed * this->up() * io.DeltaTime;
    }

    if (io.CtrlDown) {
        this->pos -= speed * this->up() * io.DeltaTime;
    }

    this->yaw += sensitivity * io.MousePosDelta.x;
    this->pitch -= sensitivity * io.MousePosDelta.y;
    this->pitch = std::clamp(this->pitch, -gp::pi_f/2.0f + 0.5f, gp::pi_f/2.0f - 0.5f);

    gp::App::cur().setMousePos(glm::ivec2{io.DisplaySize.x/2, io.DisplaySize.y/2});
    io.MousePos = {io.DisplaySize.x/2.0f, io.DisplaySize.y/2.0f};
}
