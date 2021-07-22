#include "app.hpp"

#include <SDL.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <sstream>
#include <mutex>

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

[[nodiscard]] static bool isOpenGlInDebugMode() {
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        return false;
    }

    GLboolean b = false;
    glGetBooleanv(GL_DEBUG_OUTPUT, &b);
    if (!b) {
        return false;
    }

    glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &b);
    if (!b) {
        return false;
    }

    return true;
}

[[nodiscard]] constexpr static gp::log::level::LevelEnum mapGlSeverityToLogLevelSeverity(GLenum severity) noexcept {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: return gp::log::level::err;
    case GL_DEBUG_SEVERITY_MEDIUM: return gp::log::level::warn;
    case GL_DEBUG_SEVERITY_LOW: return gp::log::level::info;
    case GL_DEBUG_SEVERITY_NOTIFICATION: return gp::log::level::trace;
    default: return gp::log::level::debug;
    }
}

[[nodiscard]] constexpr static char const* mapGlSeverityToString(GLenum severity) noexcept {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: return "GL_DEBUG_SEVERITY_HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return "GL_DEBUG_SEVERITY_MEDIUM";
    case GL_DEBUG_SEVERITY_LOW: return "GL_DEBUG_SEVERITY_LOW";
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
    default: return "GL_DEBUG_SEVERITY_UNKNOWN_TO_GP";
    }
}

[[nodiscard]] constexpr static char const* mapGlSourceToString(GLenum source) noexcept {
    switch (source) {
    case GL_DEBUG_SOURCE_API: return "GL_DEBUG_SOURCE_API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "GL_DEBUG_SOURCE_SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "GL_DEBUG_SOURCE_THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION: return "GL_DEBUG_SOURCE_APPLICATION";
    case GL_DEBUG_SOURCE_OTHER: return "GL_DEBUG_SOURCE_OTHER";
    default: return "GL_DEBUG_SOURCE_UNKNOWN_TO_GP";
    }
}

[[nodiscard]] constexpr static char const* mapGlDebugTypeToString(GLenum type) noexcept {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR: return "GL_DEBUG_TYPE_ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY: return "GL_DEBUG_TYPE_PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE: return "GL_DEBUG_TYPE_PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER: return "GL_DEBUG_TYPE_MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "GL_DEBUG_TYPE_PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP: return "GL_DEBUG_TYPE_POP_GROUP";
    case GL_DEBUG_TYPE_OTHER: return "GL_DEBUG_TYPE_OTHER";
    default: return "GL_DEBUG_TYPE_UNKNOWN_TO_GP";
    }
}

// called by the OpenGL driver whenever it wants to emit a debug message
static void onOpenGlDebugMessage(
        GLenum source,
        GLenum type,
        unsigned int id,
        GLenum severity,
        GLsizei,
        const char* message,
        const void*) {

    // ignore non-significant error/warning codes
    // if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    //    return;

    gp::log::level::LevelEnum lvl = mapGlSeverityToLogLevelSeverity(severity);
    char const* srcStr = mapGlSourceToString(source);
    char const* typeStr = mapGlDebugTypeToString(type);
    char const* sevStr = mapGlSeverityToString(severity);

    gp::log::log(lvl, R"(OpenGL debug message:
    id = %u
    message = %s
    source = %s
    type = %s
    severity = %s)", id, message, srcStr, typeStr, sevStr);
}

static void enableOpenGlDebugMode() {
    if (isOpenGlInDebugMode()) {
        gp::log::error("OpenGL is already in debug mode: skipping");
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(onOpenGlDebugMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

static void disableOpenGlDebugMode() {
    if (!isOpenGlInDebugMode()) {
        gp::log::error("OpenGL is not in debug mode: cannot disable it: skipping");
        return;
    }

    glDisable(GL_DEBUG_OUTPUT);
}

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

    // (edge-case)
    //
    // if the caller wants to set the mouse position, then it should be
    // set. However, to ensure that Delta == Cur-Prev, we need to create
    // a "fake"  *prev* that behaves "as if" the mouse moved from some
    // location to the warp location
    if (io.WantMousePosWarpTo && curWindowHasFocus) {
        SDL_WarpMouseInWindow(window, static_cast<int>(io.MousePosWarpTo.x), static_cast<int>(io.MousePosWarpTo.y));
        io.MousePosPrevious = io.MousePos - io.MousePosDelta;
        io.MousePos = io.MousePosWarpTo;
    }
}

static void updateIOPoller(gp::Io& io, SDL_Window* window) {
    GP_ASSERT(window != nullptr && "application is not initialized correctly: not showing a window?");

    // DisplaySize
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    io.DisplaySize = {w, h};

    // Ticks, (IO ctor: TickFrequency), DeltaTime
    uint64_t currentTicks = SDL_GetPerformanceCounter();
    double dTicks = static_cast<double>(currentTicks - io.Ticks);
    io.DeltaTime = static_cast<float>(dTicks/static_cast<double>(io.TickFrequency));
    io.Ticks = currentTicks;

    // MousePos, MousePosPrevious, MousePosDelta, MousePressed
    updateMousePosAndButtons(io, window);

    // (KeysDown, Shift-/Ctrl-/Alt-Down handled by events)

    // KeysDownDurationPrev
    std::copy(io.KeysDownDuration.begin(), io.KeysDownDuration.end(), io.KeysDownDurationPrev.begin());

    // KeysDownDuration
    static_assert(std::tuple_size<decltype(io.KeysDown)>::value == std::tuple_size<decltype(io.KeysDown)>::value);
    for (size_t i = 0; i < io.KeysDown.size(); ++i) {
        if (!io.KeysDown[i]) {
            io.KeysDownDuration[i] = -1.0f;
        } else {
            io.KeysDownDuration[i] = io.KeysDownDuration[i] < 0.0f ? 0.0f : io.KeysDownDuration[i] + io.DeltaTime;
        }
    }
}

gp::Io::Io(SDL_Window* window) :
    DisplaySize{-1.0f, -1.0f},
    Ticks{SDL_GetPerformanceCounter()},
    TickFrequency{SDL_GetPerformanceFrequency()},
    DeltaTime{0.0f},
    MousePos{0.0f, 0.0f},
    MousePosPrevious{0.0f, 0.0f},
    MousePosDelta{0.0f, 0.0f},
    WantMousePosWarpTo{false},
    MousePosWarpTo{-1.0f, -1.0f},
    MousePressed{false, false, false},
    // KeysDown: handled below
    ShiftDown{false},
    CtrlDown{false},
    AltDown{false}
    // KeysDownDuration: handled below
    // KeysDownDurationPrev: handled below
    {

    std::fill(KeysDown.begin(), KeysDown.end(), false);
    std::fill(KeysDownDuration.begin(), KeysDownDuration.end(), -1.0f);
    std::fill(KeysDownDurationPrev.begin(), KeysDownDurationPrev.end(), -1.0f);

    updateIOPoller(*this, window);
}

struct gp::App::Impl final {
    SdlCtx sdlctx{SDL_INIT_VIDEO};
    SdlWindow window = initMainWindow();
    SdlGLContext sdlglctx = initWindowOpenGLContext(window);
    Io io{window};
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

void gp::App::requestQuit() noexcept {
    impl->quit = true;
}

bool gp::App::isOpenGLDebugModeEnabled() noexcept {
    return ::isOpenGlInDebugMode();
}

void gp::App::enableOpenGLDebugMode() {
    ::enableOpenGlDebugMode();
}

void gp::App::disableOpenGLDebugMode() {
    ::disableOpenGlDebugMode();
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

void gp::Euler_perspective_camera::onUpdate(float speed, float sensitivity) {
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

    io.WantMousePosWarpTo = true;
    io.MousePosWarpTo = io.DisplaySize/2.0f;
}

// standard textured cube
//
// - dimensions [-1, +1] in xyz
// - uv coords of (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<gp::ShadedTexturedVert, 36> g_CubeVerts = {{
    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}}, // top-left
    // front face
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    // left face
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    {{-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    // right face
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-left
    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    // top face
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{ 1.0f,  1.0f , 1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
//
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that the quad faces toward the camera
static constexpr std::array<gp::ShadedTexturedVert, 6> g_QuadVerts = {{
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
}};

// a cube wire mesh, suitable for GL_LINES drawing
//
// a pair of verts per edge of the cube. The cube has 12 edges, so 24 lines
static constexpr std::array<gp::ShadedVert, 24> g_CubeWireMesh = {{
    // back

    // back bottom left -> back bottom right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back bottom right -> back top right
    {{+1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top right -> back top left
    {{+1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // back top left -> back bottom left
    {{-1.0f, +1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},

    // front

    // front bottom left -> front bottom right
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front bottom right -> front top right
    {{+1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top right -> front top left
    {{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front top left -> front bottom left
    {{-1.0f, +1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},
    {{-1.0f, -1.0f, +1.0f}, {0.0f, 0.0f, +1.0f}},

    // front-to-back edges

    // front bottom left -> back bottom left
    {{-1.0f, -1.0f, +1.0f}, {-1.0f, -1.0f, +1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}},

    // front bottom right -> back bottom right
    {{+1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}},
    {{+1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}},

    // front top left -> back top left
    {{-1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}},
    {{-1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f}},

    // front top right -> back top right
    {{+1.0f, +1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}},
    {{+1.0f, +1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}}
}};

std::ostream& gp::operator<<(std::ostream& o, glm::vec3 const& v) {
    return o << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}


template<typename TVert>
static std::array<TVert, 36> generateCubeImpl() noexcept {
    std::array<TVert, g_CubeVerts.size()> rv;

    for (size_t i = 0; i < rv.size(); ++i) {
        rv[i] = TVert{g_CubeVerts[i]};
    }

    return rv;
}

template<>
std::array<gp::ShadedTexturedVert, 36> gp::generateCube() noexcept {
    return generateCubeImpl<ShadedTexturedVert>();
}

template<>
std::array<gp::TexturedVert, 36> gp::generateCube() noexcept {
    return generateCubeImpl<TexturedVert>();
}

template<>
std::array<gp::PlainVert, 36> gp::generateCube() noexcept {
    return generateCubeImpl<PlainVert>();
}

static std::vector<gp::ShadedVert> generateShadedUVSphereVerts() noexcept {
    using namespace gp;

    // this is a shitty alg that produces a shitty UV sphere. I don't have
    // enough time to implement something better, like an isosphere, or
    // something like a patched sphere:
    //
    // https://www.iquilezles.org/www/articles/patchedsphere/patchedsphere.htm
    //
    // This one is adapted from:
    //    http://www.songho.ca/opengl/gl_sphere.html#example_cubesphere

    size_t sectors = 12;
    size_t stacks = 12;

    // polar coords, with [0, 0, -1] pointing towards the screen with polar
    // coords theta = 0, phi = 0. The coordinate [0, 1, 0] is theta = (any)
    // phi = PI/2. The coordinate [1, 0, 0] is theta = PI/2, phi = 0
    std::vector<ShadedVert> points;

    float theta_step = 2.0f * pi_f / sectors;
    float phi_step = pi_f / stacks;

    for (size_t stack = 0; stack <= stacks; ++stack) {
        float phi = pi_f / 2.0f - static_cast<float>(stack) * phi_step;
        float y = sinf(phi);

        for (unsigned sector = 0; sector <= sectors; ++sector) {
            float theta = sector * theta_step;
            float x = sinf(theta) * cosf(phi);
            float z = -cosf(theta) * cosf(phi);
            glm::vec3 pos{x, y, z};
            glm::vec3 normal{pos};
            points.push_back({pos, normal});
        }
    }

    // the points are not triangles. They are *points of a triangle*, so the
    // points must be triangulated

    std::vector<ShadedVert> out;
    for (size_t stack = 0; stack < stacks; ++stack) {
        size_t k1 = stack * (sectors + 1);
        size_t k2 = k1 + sectors + 1;

        for (size_t sector = 0; sector < sectors; ++sector, ++k1, ++k2) {
            // 2 triangles per sector - excluding the first and last stacks
            // (which contain one triangle, at the poles)
            ShadedVert p1 = points.at(k1);
            ShadedVert p2 = points.at(k2);
            ShadedVert p1_plus1 = points.at(k1 + 1u);
            ShadedVert p2_plus1 = points.at(k2 + 1u);

            if (stack != 0) {
                out.push_back(p1);
                out.push_back(p1_plus1);
                out.push_back(p2);
            }

            if (stack != (stacks - 1)) {
                out.push_back(p1_plus1);
                out.push_back(p2_plus1);
                out.push_back(p2);
            }
        }
    }
    return out;
}

template<>
void gp::generateUVSphere(std::vector<PlainVert>& out) {
    std::vector<ShadedVert> shadedVerts = generateShadedUVSphereVerts();

    out.clear();
    out.reserve(shadedVerts.size());
    for (ShadedVert const& sv : shadedVerts) {
        out.push_back(PlainVert{sv.pos});
    }
}

std::vector<gp::PlainVert> gp::generateCubeWireMesh() {
    std::vector<PlainVert> rv;
    rv.reserve(g_CubeWireMesh.size());
    for (ShadedVert const& v : g_CubeWireMesh) {
        rv.push_back(PlainVert{v.pos});
    }
    return rv;
}

template<>
std::array<gp::PlainVert, 6> gp::generateQuad() noexcept {
    std::array<PlainVert, 6> rv;
    for (size_t i = 0; i < g_QuadVerts.size(); ++i) {
        rv[i] = PlainVert{g_QuadVerts[i].pos};
    }
    return rv;
}

template<>
void gp::generateCircle(size_t segments, std::vector<PlainVert>& out) {
    float fsegments = static_cast<float>(segments);
    float step = (2 * pi_f) / fsegments;

    out.clear();
    out.reserve(3*segments);
    for (int i = 0; i < segments; ++i) {
        float theta1 = i * step;
        float theta2 = (i+1) * step;

        out.emplace_back(0.0f, 0.0f, 0.0f);
        out.emplace_back(sinf(theta1), cosf(theta1), 0.0f);
        out.emplace_back(sinf(theta2), cosf(theta2), 0.0f);
    }
}

std::ostream& gp::operator<<(std::ostream& o, AABB const& aabb) {
    return o << "min = " << aabb.min << ", max = " << aabb.max;
}

std::string gp::str(glm::vec3 const& v) {
    std::stringstream ss;
    ss << v;
    return std::move(ss).str();
}

std::ostream& gp::operator<<(std::ostream& o, Line const& l) {
    return o << "origin = " << l.o << ", direction = " << l.d;
}

std::ostream& gp::operator<<(std::ostream& o, Sphere const& s) {
    return o << "origin = " << s.origin << ", radius = " << s.radius;
}


struct QuadraticFormulaResult final {
    bool computeable;
    float x0;
    float x1;
};

// solve a quadratic formula
//
// only real-valued results supported - no complex-plane results
static QuadraticFormulaResult solveQuadratic(float a, float b, float c) {
    QuadraticFormulaResult res;

    // b2 - 4ac
    float discriminant = b*b - 4.0f*a*c;

    if (discriminant < 0.0f) {
        res.computeable = false;
        return res;
    }

    // q = -1/2 * (b +- sqrt(b2 - 4ac))
    float q = -0.5f * (b + copysign(sqrt(discriminant), b));

    // you might be wondering why this doesn't just compute a textbook
    // version of the quadratic equation (-b +- sqrt(disc))/2a
    //
    // the reason is because `-b +- sqrt(b2 - 4ac)` can result in catastrophic
    // cancellation if `-b` is close to `sqrt(disc)`
    //
    // so, instead, we use two similar, complementing, quadratics:
    //
    // the textbook one:
    //
    //     x = (-b +- sqrt(disc)) / 2a
    //
    // and the "Muller's method" one:
    //
    //     x = 2c / (-b -+ sqrt(disc))
    //
    // the great thing about these two is that the "+-" part of their
    // equations are complements, so you can have:
    //
    // q = -0.5 * (b + sign(b)*sqrt(disc))
    //
    // which, handily, will only *accumulate* the sum inside those
    // parentheses. If `b` is positive, you end up with a positive
    // number. If `b` is negative, you end up with a negative number. No
    // catastropic cancellation. By multiplying it by "-0.5" you end up
    // with:
    //
    //     -b - sqrt(disc)
    //
    // or, if B was negative:
    //
    //     -b + sqrt(disc)
    //
    // both of which are valid terms of both the quadratic equations above
    //
    // see:
    //
    //     https://math.stackexchange.com/questions/1340267/alternative-quadratic-formula
    //     https://en.wikipedia.org/wiki/Quadratic_equation


    res.computeable = true;
    res.x0 = q/a;  // textbook "complete the square" equation
    res.x1 = c/q;  // Muller's method equation
    return res;
}

static gp::LineSphereHittestResult computeIntersectionAnalytic(gp::Sphere const& s, gp::Line const& l) noexcept {
    // see:
    //     https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    gp::LineSphereHittestResult rv;

    glm::vec3 L = l.o - s.origin;

    // coefficients of the quadratic implicit:
    //
    //     P2 - R2 = 0
    //     (O + tD)2 - R2 = 0
    //     (O + tD - C)2 - R2 = 0
    //
    // where:
    //
    //     P    a point on the surface of the sphere
    //     R    the radius of the sphere
    //     O    origin of line
    //     t    scaling factor for line direction (we want this)
    //     D    direction of line
    //     C    center of sphere
    //
    // if the quadratic has solutions, then there must exist one or two
    // `t`s that are points on the sphere's surface.

    float a = glm::dot(l.d, l.d);  // always == 1.0f if d is normalized
    float b = 2.0f * glm::dot(l.d, L);
    float c = glm::dot(L, L) - glm::dot(s.radius, s.radius);

    auto [ok, t0, t1] = solveQuadratic(a, b, c);

    if (!ok) {
        rv.intersected = false;
        return rv;
    }

    // ensure X0 < X1
    if (t0 > t1) {
        std::swap(t0, t1);
    }

    // ensure it's in front
    if (t0 < 0.0f) {
        t0 = t1;
        if (t0 < 0.0f) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.t0 = t0;
    rv.t1 = t1;
    rv.intersected = true;

    return rv;
}

static gp::LineSphereHittestResult computeIntersectionGeometric(gp::Sphere const& s, gp::Line const& l) noexcept {
    // see:
    //     https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

    gp::LineSphereHittestResult rv;

    glm::vec3 L = s.origin - l.o;  // line origin to sphere origin
    float tca = glm::dot(L, l.d);  // projected line from middle of hitline to sphere origin

    if (tca < 0.0f) {
        // line is pointing away from the sphere
        rv.intersected = false;
        return rv;
    }

    float d2 = glm::dot(L, L) - tca*tca;
    float r2 = s.radius * s.radius;

    if (d2 > r2) {
        // line is not within the sphere's radius
        rv.intersected = false;
        return rv;
    }

    // the collision points are on the sphere's surface (R), and D
    // is how far the hitline midpoint is from the radius. Can use
    // Pythag to figure out the midpoint length (thc)
    float thc = glm::sqrt(r2 - d2);

    rv.t0 = tca - thc;
    rv.t1 = tca + thc;
    rv.intersected = true;
    return rv;
}

gp::LineSphereHittestResult gp::lineIntersectsSphere(Sphere const& s, Line const& l) noexcept {
    constexpr bool useGeometricSolution = true;

    if (useGeometricSolution) {
        return computeIntersectionGeometric(s, l);
    } else {
        return computeIntersectionAnalytic(s, l);
    }
}

glm::mat4 gp::quadToPlaneXform(Plane const& p) noexcept {
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {100000.0f, 100000.0f, 1.0f});

    glm::vec3 quadNormal = {0.0f, 0.0f, 1.0f};  // just how the generator defines it
    float angle = glm::acos(glm::dot(quadNormal, p.normal));  // dot(a, b) == |a||b|cos(ang)
    glm::vec3 axis = glm::cross(quadNormal, p.normal);
    glm::mat4 rotator = glm::rotate(glm::mat4{1.0f}, angle, axis);

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, p.origin);

    return translator * rotator * scaler;
}

glm::mat4 gp::circleToDiscXform(Disc const& d) noexcept {
    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {d.radius, d.radius, 1.0f});

    glm::vec3 discNormal = {0.0f, 0.0f, 1.0f};     // just how the generator defines it
    float angle = glm::acos(glm::dot(discNormal, d.normal));  // dot(a, b) == |a||b|cos(ang)
    glm::vec3 axis = glm::cross(discNormal, d.normal);

    glm::mat4 rotator = glm::rotate(glm::mat4{1.0f}, angle, axis);

    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, d.origin);

    return translator * rotator * scaler;
}

glm::mat4 gp::cubeToAABBXform(AABB const& aabb) noexcept {
    glm::vec3 center = aabbCenter(aabb);
    glm::vec3 halfWidths = aabbDimensions(aabb) / 2.0f;

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
    glm::mat4 translator = glm::translate(glm::mat4{1.0f}, center);

    return translator * scaler;
}

gp::LineAABBHittestResult gp::lineIntersectsAABB(AABB const& aabb, Line const& l) noexcept {
    gp::LineAABBHittestResult rv;

    float t0 = -FLT_MAX;
    float t1 = FLT_MAX;

    // intersect the ray with each axis-aligned slab for each dimension
    //
    // i.e. figure out where the line intersects the front+back of the AABB
    //      in (e.g.) X, then Y, then Z, and intersect those interactions such
    //      that if the intersection is ever empty (or, negative here) then there
    //      is no intersection
    for (int i = 0; i < 3; ++i) {
        float invDir = 1.0f / l.d[i];
        float tNear = (aabb.min[i] - l.o[i]) * invDir;
        float tFar = (aabb.max[i] - l.o[i]) * invDir;
        if (tNear > tFar) {
            std::swap(tNear, tFar);
        }
        t0 = std::max(t0, tNear);
        t1 = std::min(t1, tFar);

        if (t0 > t1) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.t0 = t0;
    rv.t1 = t1;
    rv.intersected = true;
    return rv;
}

gp::LinePlaneHittestResult gp::lineIntersectsPlane(Plane const& p, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
    //
    // effectively, this is evaluating:
    //
    //     P, a point on the plane
    //     P0, the plane's origin (distance from world origin)
    //     N, the plane's normal
    //
    // against: dot(P-P0, N)
    //
    // which must equal zero for any point in the plane. Given that, a line can
    // be parameterized as `P = O + tD` where:
    //
    //     P, point along the line
    //     O, origin of line
    //     t, distance along line direction
    //     D, line direction
    //
    // sub the line equation into the plane equation, rearrange for `t` and you
    // can figure out how far a plane is along a line
    //
    // equation: t = dot(P0 - O, n) / dot(D, n)

    LinePlaneHittestResult rv;

    float denominator = glm::dot(p.normal, l.d);

    if (std::abs(denominator) > 1e-6) {
        float numerator = glm::dot(p.origin - l.o, p.normal);
        rv.intersected = true;
        rv.t = numerator / denominator;
        return rv;
    } else {
        // the line is *very* parallel to the plane, which could cause
        // some divide-by-zero havok: pretend it didn't intersect
        rv.intersected = false;
        return rv;
    }
}

gp::LineDiscHittestResult gp::lineIntersectsDisc(Disc const& d, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection

    // think of this as a ray-plane intersection test with the additional
    // constraint that the ray has to be within the radius of the disc

    LineDiscHittestResult rv;

    Plane p;
    p.origin = d.origin;
    p.normal = d.normal;

    auto [ok, t] = lineIntersectsPlane(p, l);

    if (!ok) {
        rv.intersected = false;
        return rv;
    }

    glm::vec3 pos = l.o + t*l.d;
    glm::vec3 v = pos - d.origin;

    float d2 = glm::dot(v, v);
    float r2 = glm::dot(d.radius, d.radius);

    if (d2 > r2) {
        rv.intersected = false;
        return rv;
    }

    rv.intersected = true;
    rv.t = t;
    return rv;
}

gp::LineTriangleHittestResult gp::lineIntersectsTriangle(glm::vec3 const* v, Line const& l) noexcept {
    // see: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution

    LineTriangleHittestResult rv;

    // compute triangle normal
    glm::vec3 N = glm::normalize(glm::cross(v[0] - v[1], v[0] - v[2]));

    // compute dot product between normal and ray
    float NdotR = glm::dot(N, l.d);

    // if the dot product is small, then the ray is probably very parallel to
    // the triangle and doesn't intersect
    if (std::abs(NdotR) < std::numeric_limits<float>::epsilon()) {
        rv.intersected = false;
        return rv;
    }

    float D = glm::dot(N, v[0]);
    float t =  - (glm::dot(N, l.o) + D) / glm::dot(N, l.d);

    // if triangle plane is behind line then return early
    if (t < 0.0f) {
        rv.intersected = false;
        return rv;
    }

    // intersection point on triangle plane, computed from line equation
    glm::vec3 P = l.o + t*l.d;

    // figure out if that point is inside the triangle's bounds using the
    // "inside-outside" test

    // test each triangle edge: {0, 1}, {1, 2}, {2, 0}
    for (int i = 0; i < 3; ++i) {
        glm::vec3 const& start = v[i];
        glm::vec3 const& end = v[(i+1)%3];

        // corner[n] to corner[n+1]
        glm::vec3 e = end - start;

        // corner[n] to P
        glm::vec3 c = P - start;

        // cross product of the above indicates whether the vectors are
        // clockwise or anti-clockwise with respect to eachover. It's a
        // right-handed coord system, so anti-clockwise produces a vector
        // that points in same direction as normal
        glm::vec3 ax = glm::cross(e, c);

        // if the dot product of that axis with the normal is <0.0f then
        // the point was "outside"
        if (glm::dot(ax, N) < 0.0f) {
            rv.intersected = false;
            return rv;
        }
    }

    rv.intersected = true;
    rv.t = t;
    return rv;
}

glm::vec3 gp::triangleNormal(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& c) noexcept {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 perpendiular = glm::cross(ab, ac);
    return glm::normalize(perpendiular);
}
