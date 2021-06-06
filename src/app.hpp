#pragma once

#include <SDL_events.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <iostream>
#include <utility>
#include <sstream>
#include <memory>
#include <string_view>
#include <chrono>
#include <vector>
#include <mutex>

// logging support
//
// enables any part of the codebase to dispatch log messages to the application
namespace gp::log {

    namespace level {
        enum LevelEnum { trace = 0, debug, info, warn, err, critical, off, NUM_LEVELS };

#define LOG_LVL_NAMES                                                                                             \
    { "trace", "debug", "info", "warning", "error", "critical", "off" }

        extern std::string_view const g_nameViews[];
        extern char const* const g_nameCStrings[];
    }

    [[nodiscard]] inline std::string_view const& toStringView(level::LevelEnum lvl) noexcept {
        return level::g_nameViews[lvl];
    }

    [[nodiscard]] inline char const* toCString(level::LevelEnum lvl) noexcept {
        return level::g_nameCStrings[lvl];
    }

    // a log message
    //
    // to prevent needless runtime allocs, this does not own its data. See below if you need an
    // owning version
    struct Msg final {
        std::string_view loggerName;
        std::chrono::system_clock::time_point t;
        std::string_view payload;
        level::LevelEnum level;

        Msg() = default;

        Msg(std::string_view loggerName_, std::string_view payload_, level::LevelEnum level_) :
            loggerName{loggerName_},
            t{std::chrono::system_clock::now()},
            payload{payload_},
            level{level_} {
        }
    };

    // a log message that owns all its data
    //
    // useful if you need to persist a log message somewhere in memory
    struct OwnedMsg final {
        std::string loggerName;
        std::chrono::system_clock::time_point t;
        std::string payload;
        level::LevelEnum level;

        OwnedMsg() = default;

        OwnedMsg(Msg const& msg) :
            loggerName{msg.loggerName},
            t{msg.t},
            payload{msg.payload},
            level{msg.level} {
        }
    };

    // a log sink that consumes log messages
    class Sink {
        level::LevelEnum level_{level::info};

    public:
        virtual ~Sink() noexcept = default;
        virtual void log(Msg const&) = 0;

        void setLevel(level::LevelEnum level) noexcept {
            level_ = level;
        }

        [[nodiscard]] level::LevelEnum level() const noexcept {
            return level_;
        }

        [[nodiscard]] bool shouldLog(level::LevelEnum level) const noexcept {
            return level >= level_;
        }
    };

    class Logger final {
        std::string name_;
        std::vector<std::shared_ptr<Sink>> sinks_;
        level::LevelEnum level_{level::trace};

    public:
        Logger(std::string name) : name_{std::move(name)}, sinks_() {
        }

        Logger(std::string name, std::shared_ptr<Sink> sink) : name_{std::move(name)}, sinks_{sink} {
        }

        template<typename... Args>
        void log(level::LevelEnum msgLevel, char const* fmt, ...) {
            if (msgLevel < level_) {
                return;
            }

            // create the log message
            char buf[512];
            size_t n = 0;
            {
                va_list args;
                va_start(args, fmt);
                int rv = std::vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);

                if (rv < 0) {
                    return;
                }
                n = std::min(static_cast<size_t>(rv), sizeof(buf));
            }

            Msg msg{name_, std::string_view{buf, n}, msgLevel};

            // sink the message
            for (auto& sink : sinks_) {
                if (sink->shouldLog(msg.level)) {
                    sink->log(msg);
                }
            }
        }

        template<typename... Args>
        void trace(char const* fmt, Args const&... args) {
            log(level::trace, fmt, args...);
        }

        template<typename... Args>
        void debug(char const* fmt, Args const&... args) {
            log(level::debug, fmt, args...);
        }

        template<typename... Args>
        void info(char const* fmt, Args const&... args) {
            log(level::info, fmt, args...);
        }

        template<typename... Args>
        void warn(char const* fmt, Args const&... args) {
            log(level::warn, fmt, args...);
        }

        template<typename... Args>
        void error(char const* fmt, Args const&... args) {
            log(level::err, fmt, args...);
        }

        template<typename... Args>
        void critical(char const* fmt, Args const&... args) {
            log(level::critical, fmt, args...);
        }

        [[nodiscard]] std::vector<std::shared_ptr<Sink>> const& sinks() const noexcept {
            return sinks_;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Sink>>& sinks() noexcept {
            return sinks_;
        }
    };

    // global logging API

    // get the default (typically, stdout) log
    std::shared_ptr<Logger> defaultLogger() noexcept;
    Logger* defaultLoggerRaw() noexcept;

    template<typename... Args>
    inline void log(level::LevelEnum level, char const* fmt, Args const&... args) {
        defaultLoggerRaw()->log(level, fmt, args...);
    }

    template<typename... Args>
    inline void trace(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->trace(fmt, args...);
    }

    template<typename... Args>
    inline void debug(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->debug(fmt, args...);
    }

    template<typename... Args>
    void info(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->info(fmt, args...);
    }

    template<typename... Args>
    void warn(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->warn(fmt, args...);
    }

    template<typename... Args>
    void error(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->error(fmt, args...);
    }

    template<typename... Args>
    void critical(char const* fmt, Args const&... args) {
        defaultLoggerRaw()->critical(fmt, args...);
    }
}

// assertions support
//
// enables in-source logic checks /w relevant error messages, debugging, whatever
namespace gp {
    [[noreturn]] void onAssertFailed(char const* failingSourceCode, char const* file, unsigned int line) noexcept;
}
#define GP_ASSERT_ALWAYS(expr)                                                                                       \
    (static_cast<bool>(expr) ? (void)0 : gp::onAssertFailed(#expr, __FILE__, __LINE__))

#ifdef GP_FORCE_ASSERTS_ENABLED
#define GP_ASSERT(expr) GP_ASSERT_ALWAYS(expr)
#elif !defined(NDEBUG)
#define GP_ASSERT(expr) GP_ASSERT_ALWAYS(expr)
#else
#define GP_ASSERT(expr)
#endif

// screen support
//
// separates screen-level concerns (handle events, update, draw) from the
// rest of the application's concerns (app init, gameloop maintenance, etc.)
//
// lifecycle:
//
// 1. (app receives screen)
// 2. app calls onMount
// 3. (screen is now part of app's gameloop)
// 4. app calls onEvent until all events pumped
// 5. app calls onUpdate with timedelta since last call to onUpdate
// 6. app calls onDraw
// 7a1. if no new screen, GOTO 4
// 7b1. if new screen received, call onUnmount
// 7b2. destroy old screen
// 7b3. GOTO 2 with new screen
namespace gp {
    class Screen {
    public:
        virtual ~Screen() noexcept = default;

        // called just before App starts using the screen
        virtual void onMount() {
        }

        virtual bool onEvent(SDL_Event const&) {
            return false;
        }

        // callers should use their own recording system (e.g. poller)
        // internally to compute time delta as this method is entered
        virtual void onUpdate() {
        }

        virtual void onDraw() = 0;

        virtual void onUnmount() {
        }
    };
}

// input polling support
//
// App automatically maintains these in the top-level gameloop so
// that the rest of the system can just ask for these values whenever
// they need them
namespace gp {
    struct Io final {
        // drawable size of display
        glm::ivec2 DisplaySize;

        // number of ticks since the last call to update
        uint64_t Ticks;

        // ticks per per second
        uint64_t TickFrequency;

        // seconds since last update
        float DeltaTime;

        // current mouse position, in pixels, relative to top-left corner of screen
        glm::vec2 MousePos;

        // previous mouse position
        glm::vec2 MousePosPrevious;

        // mouse position delta from previous update
        glm::vec2 MousePosDelta;

        // mouse button states (0: left, 1: right, 2: middle)
        bool MousePressed[3];

        // keyboard keys that are currently pressed
        bool KeysDown[512];
        bool ShiftDown;
        bool CtrlDown;
        bool AltDown;

        [[nodiscard]] constexpr float aspectRatio() const noexcept {
            return static_cast<float>(DisplaySize.x) / static_cast<float>(DisplaySize.y);
        }
    };
}

// application support
//
// top-level appplication system that initializes all major subsytems
// (e.g. video, windowing, OpenGL, input) and maintains the top-level
// gameloop
namespace gp {
    class App final {
    public:
        struct Impl;

    private:
        Impl* impl;
        static App* g_Current;
        static Io* g_CurrentIO;

    public:
        [[nodiscard]] static App& cur() noexcept {
            GP_ASSERT(g_Current != nullptr && "current application not set: have you initialized an application?");
            return *g_Current;
        }

        [[nodiscard]] static Io& IO() noexcept {
            GP_ASSERT(g_Current != nullptr && "current IO not set: have you initialized an application?");
            return *g_CurrentIO;
        }

        App();
        App(App const&) = delete;
        App(App&&) = delete;
        ~App() noexcept;

        App& operator=(App const&) = delete;
        App& operator=(App&&) = delete;

        // enters gameloop with supplied screen
        void show(std::unique_ptr<Screen>);

        // raw handle to underlying window implementation
        void* windowRAW() noexcept;

        // raw handle to underlying OpenGL context for window
        void* glRAW() noexcept;

        // "grabs" the mouse in the screen, hiding it and making it "stick"
        // to the inner area of the screen
        //
        // note: the mouse is still *somewhere* in the screen and can get
        // stuck in (e.g.) the corner
        void enableRelativeMouseMode() noexcept;

        // sets mouse position somewhere in current window
        void setMousePos(glm::ivec2) noexcept;

        // requst the app quits
        //
        // the app will only check for this at the *start* of a frame
        void requestQuit() noexcept;
    };
}

// ImGui support
//
// enables support for ImGui UI rendering: handy for debugging a screen
// as it is developed
namespace gp {
    // init ImGui context
    void ImGuiInit();

    // shutdown ImGui context
    void ImGuiShutdown();

    // returns true if ImGui has handled the event
    bool ImGuiOnEvent(SDL_Event const&) noexcept;

    // should be called at the start of `draw()`
    void ImGuiNewFrame();

    // should be called at the end of `draw()`
    void ImGuiRender();
}


// scope guard support
//
// enables ad-hoc RAII behavior in source code without having to write
// custom wrapper classes etc. - useful for little cleanup routines or
// using C APIs. Example:
//
//     FILE* f = fopen("path");
//     GP_SCOPEGUARD(fclose(f));
namespace gp {
    template<typename Dtor>
    class ScopeGuard final {
        Dtor dtor;

    public:
        ScopeGuard(Dtor&& _dtor) noexcept : dtor{std::move(_dtor)} {
        }
        ScopeGuard(ScopeGuard const&) = delete;
        ScopeGuard(ScopeGuard&&) = delete;
        ScopeGuard& operator=(ScopeGuard const&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) = delete;
        ~ScopeGuard() noexcept {
            dtor();
        }
    };

    // example usage: `GP_SCOPEGUARD({ SomeDtor(); });`
    #define GP_TOKENPASTE(x, y) x##y
    #define GP_TOKENPASTE2(x, y) GP_TOKENPASTE(x, y)
    #define GP_SCOPEGUARD(action) gp::ScopeGuard GP_TOKENPASTE2(guard_, __LINE__){[&]() action};
    #define GP_SCOPEGUARD_IF(cond, action)                                                                              \
        GP_SCOPEGUARD({                                                                                                 \
            if (cond) {                                                                                                    \
                action                                                                                                     \
            }                                                                                                              \
        }) \
    }
}

// constants support
//
// just some constants that are annoying to have to grab from somewhere
// else
namespace gp {
    static constexpr float pi_f = 3.14159265358979323846f;
}

// camera support
//
// cameras that can be updated over time and produce OpenGL-ready
// view/projection transforms
namespace gp {

    // perspective camera with Euler angles
    //
    // it is up to the caller to "integrate" the camera's motion
    struct Euler_perspective_camera final {
        // position in world space
        glm::vec3 pos{0.0f, 0.0f, 0.0f};

        // head tilting up/down in radians
        //
        // the implementation should
        float pitch = 0.0f;

        // spinning left/right in radians
        float yaw = -pi_f/2.0f;

        // field of view, in radians
        float fov = pi_f * 70.0f/180.f;

        // Z near clipping distance
        float znear = 0.1f;

        // Z far clipping distance
        float zfar = 1000.0f;

        [[nodiscard]] glm::vec3 front() const noexcept {
            return glm::normalize(glm::vec3{
                cosf(yaw) * cosf(pitch),
                sinf(pitch),
                sinf(yaw) * cosf(pitch),
            });
        }

        [[nodiscard]] glm::vec3 up() const noexcept {
            return glm::vec3(0.0f, 1.0f, 0.0f);
        }

        [[nodiscard]] glm::vec3 right() const noexcept {
            return glm::normalize(glm::cross(front(), up()));
        }

        [[nodiscard]] glm::mat4 viewMatrix() const noexcept;

        [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const noexcept;

        // assumes app is running in relative mode (cursor hidden) and that the
        // camera should warp the app's (hidden) cursor accordingly to ensure
        // it does not hit the edges of the window and stop
        void onUpdateAsGrabbedCamera(float movementSpeed, float mouseSensitivity);
    };
}
