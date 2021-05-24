#pragma once

#include <SDL_events.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <utility>
#include <sstream>
#include <memory>
#include <string_view>
#include <chrono>
#include <vector>
#include <mutex>

// basic logging support
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

// timing/updates
namespace gp {
    using dms = std::chrono::duration<double, std::milli>;

    [[nodiscard]] constexpr dms operator""_dms(long double v) noexcept {
        return dms{v};
    }
}
#define GP_MS_PER_UPDATE 17
#define GP_DMS_PER_UPDATE gp::dms{GP_MS_PER_UPDATE}

// screen/layer support
namespace gp {
    class Screen {
    public:
        virtual ~Screen() = default;

        // called just before App starts using the screen
        virtual void onMount() {
        }

        // called just after App stops using the screen
        virtual void onUnmount() {
        }

        virtual bool onEvent(SDL_Event const&) {
            return false;
        }

        // called with an assumed fixed step of GP_MS_PER_UPDATE
        virtual void onUpdate() {
        }

        // provided a ratio of where this drawcall happens between two
        // update steps
        //
        // e.g. 0.5 means it's midway between two update steps and that
        //      it should (maybe) lerp/slerp from the current state
        virtual void onDraw(double) = 0;
    };
}

// scope guard support
//
// adds RAII-like behavior for arbitrary shutdown code
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
}

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

// application layer: sets up top-level systems (windowing, OpenGL, etc.)
namespace gp {
    struct WindowDimensions { int w; int h; };

    class App final {
    public:
        struct Impl;

    private:
        Impl* impl;
        static App* g_Current;

    public:
        [[nodiscard]] static App& cur() noexcept {
            GP_ASSERT(g_Current != nullptr && "current application not set: have you initialized an application?");
            return *g_Current;
        }

        [[nodiscard]] static WindowDimensions dims() noexcept {
            return App::cur().windowDimensions();
        }

        [[nodiscard]] static float aspectRatio() noexcept {
            auto [w, h] = dims();
            return static_cast<float>(w) / static_cast<float>(h);
        }

        App();
        App(App const&) = delete;
        App(App&&) = delete;
        ~App() noexcept;

        App& operator=(App const&) = delete;
        App& operator=(App&&) = delete;

        void show(std::unique_ptr<Screen>);

        // raw handle to underlying window implementation
        void* windowRAW() noexcept;

        // raw handle to underlying OpenGL context for window
        void* glRAW() noexcept;

        // get active window dimensions
        [[nodiscard]] WindowDimensions windowDimensions() const noexcept;

        // hide cursor
        void hideCursor() noexcept;
    };
}

// ImGui support
//
// note: it's up to each screen to init/teardown/feed ImGui because some screens
//       may not want to use it
namespace gp {
    void ImGuiInit();
    void ImGuiShutdown();
    void ImGuiNewFrame();
    void ImGuiRender();
}

namespace gp {
    static constexpr float pi_f = 3.14159265358979323846f;
}

// Camera support
//
// note: it's up to each screen to feed the camera correctly
namespace gp {

    // perspective camera with euler angles
    struct Euler_perspective_camera final {
        // position in world space
        glm::vec3 pos{0.0f, 0.0f, 0.0f};

        // head tilting up/down
        float pitch = 0.0f;

        // spinning left/right
        float yaw = -pi_f/2.0f;

        // field of view, in degrees
        float fov = 90.0;

        // Z near clipping distance
        float znear = 0.1f;

        // Z far clipping distance
        float zfar = 1000.0f;

        // state typically retained between frames
        bool moving_forward : 1 = false;
        bool moving_backward : 1 = false;
        bool moving_left : 1 = false;
        bool moving_right : 1 = false;
        bool moving_up : 1 = false;
        bool moving_down : 1 = false;

        [[nodiscard]] glm::vec3 front() const noexcept;
        [[nodiscard]] glm::vec3 up() const noexcept;
        [[nodiscard]] glm::vec3 right() const noexcept;
        [[nodiscard]] glm::mat4 viewMatrix() const noexcept;
        [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const noexcept;

        // make user keyboard events affect camera state
        bool onEvent(SDL_KeyboardEvent const& e) noexcept;

        // make user mouse events affect camera state
        bool onEvent(SDL_MouseMotionEvent const&) noexcept;

        // make continuous updates (camera motion, etc.) at the update rate
        // (i.e. GP_DMS_PER_UPDATE) affect the camera state
        void onUpdate();
    };
}
