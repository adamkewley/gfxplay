#pragma once

#include <SDL_events.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <iostream>
#include <utility>
#include <memory>
#include <string_view>
#include <chrono>
#include <vector>
#include <array>

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

        // sink a printf-style log message
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

// layer support
//
// separates layer-level concerns (handle events, update, draw) from the
// rest of the application's concerns (app init, gameloop maintenance, polling,
// etc.)
namespace gp {
    class Screen {
    public:
        virtual ~Screen() noexcept = default;

        // called just before App starts using the screen
        virtual void onMount() {
        }

        virtual void onUnmount() {
        }

        virtual void onEvent(SDL_Event const&) {
        }

        // callers should use their own recording system (e.g. poller)
        // internally to compute time delta as this method is entered
        virtual void onUpdate() {
        }

        virtual void onDraw() = 0;
    };
}

// input polling support
//
// gp::App automatically maintains these in the top-level gameloop so
// that the rest of the system can just ask for these values whenever
// they need them, rather than having to maintain their own state
// machines
namespace gp {
    struct Io final {
        // drawable size of display
        glm::vec2 DisplaySize;

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

        // mouse position delta from previous update (== MousePos - MousePosPrevious)
        glm::vec2 MousePosDelta;

        // indicates that the backend should set the OS mouse pos
        //
        // the backend will warp to the MousePosWarpTo location, but will
        // ensure that MousePosDelta behaves "as if" the user moved their
        // mouse from MousePosPrevious to MousePosWarpTo
        //
        // the backend will reset WantMousePosWarpTo to `false` after
        // performing the warp
        bool WantMousePosWarpTo;
        glm::vec2 MousePosWarpTo;

        // mouse button states (0: left, 1: right, 2: middle)
        std::array<bool, 3> MousePressed;

        // keyboard keys that are currently pressed
        std::array<bool, 512> KeysDown;
        bool ShiftDown;
        bool CtrlDown;
        bool AltDown;

        // duration, in seconds, that each key has been pressed for
        //
        // == -1.0f if key is not down this frame
        // ==  0.0f if the key was pressed this frame
        // >   0.0f if the key was pressed in a previous frame
        std::array<float, 512> KeysDownDuration;

        // as above, but the *previous* frame's values
        //
        // the utility of this is for detecting when a key was released.
        // e.g. if a value in here is >= 0.0f && !KeysDown[key] then the
        // key must have been released this frame
        std::array<float, 512> KeysDownDurationPrev;

        // initialize this struct
        //
        // must be initialized after (or during) app initialization
        Io(SDL_Window*);
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

        template<typename TScreen, typename std::enable_if_t<std::is_base_of_v<Screen, TScreen>, TScreen>>
        void show(std::unique_ptr<TScreen> p) {
            show(static_cast<std::unique_ptr<Screen>&&>(std::move(p)));
        }

        template<typename TScreen, typename... Args>
        void show(Args&&... args) {
            this->show(std::make_unique<TScreen>(std::forward(args)...));
        }

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

        // requst the app quits
        //
        // the app will only check for this at the *start* of a frame
        void requestQuit() noexcept;

        // returns true if OpenGL debugging is enabled
        bool isOpenGLDebugModeEnabled() noexcept;
        void enableOpenGLDebugMode();
        void disableOpenGLDebugMode();

        float aspectRatio() const noexcept {
            return IO().DisplaySize.x / IO().DisplaySize.y;
        }
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

        void onUpdate(float movementSpeed, float mouseSensitivity);
    };
}

// basic 3d datatype/primitives support
//
// just some safe, commonly-used datastructures + data
namespace gp {

    // print a glm::vec3 to an output stream
    std::ostream& operator<<(std::ostream&, glm::vec3 const&);

    // returns a string representation of a glm::vec3
    std::string str(glm::vec3 const&);

    struct ShadedTexturedVert final {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
    };

    struct ShadedVert final {
        glm::vec3 pos;
        glm::vec3 norm;

        ShadedVert() = default;

        constexpr ShadedVert(glm::vec3 const& pos_, glm::vec3 const& norm_) noexcept :
            pos{pos_},
            norm{norm_} {
        }

        explicit constexpr ShadedVert(ShadedTexturedVert const& v) noexcept :
            pos{v.pos},
            norm{v.norm} {
        }
    };

    struct TexturedVert final {
        glm::vec3 pos;
        glm::vec2 uv;

        TexturedVert() = default;

        constexpr TexturedVert(glm::vec3 const& pos_, glm::vec2 const& uv_) noexcept :
            pos{pos_},
            uv{uv_} {
        }

        explicit constexpr TexturedVert(ShadedTexturedVert const& v) noexcept :
            pos{v.pos},
            uv{v.uv} {
        }
    };

    struct PlainVert final {
        glm::vec3 pos;

        PlainVert() = default;

        constexpr PlainVert(float x, float y, float z) noexcept :
            pos{x, y, z} {
        }

        constexpr PlainVert(glm::vec3 const& pos_) noexcept :
            pos{pos_} {
        }

        explicit constexpr PlainVert(ShadedTexturedVert const& v) noexcept :
            pos{v.pos} {
        }

        explicit constexpr PlainVert(ShadedVert const& v) noexcept :
            pos{v.pos} {
        }

        explicit constexpr PlainVert(TexturedVert const& v) noexcept :
            pos{v.pos} {
        }
    };

    template<typename TVert>
    [[nodiscard]] std::array<TVert, 36> generateCube() noexcept;

    template<>
    [[nodiscard]] std::array<ShadedTexturedVert, 36> generateCube() noexcept;

    template<>
    [[nodiscard]] std::array<TexturedVert, 36> generateCube() noexcept;

    template<>
    [[nodiscard]] std::array<PlainVert, 36> generateCube() noexcept;

    template<typename TVert>
    void generateUVSphere(std::vector<TVert>&);

    template<>
    void generateUVSphere(std::vector<PlainVert>&);

    template<typename TVert>
    [[nodiscard]] std::vector<TVert> generateUVSphere() {
        std::vector<TVert> rv;
        generateUVSphere(rv);
        return rv;
    }

    [[nodiscard]] std::vector<PlainVert> generateCubeWireMesh();

    // generate quad verts (i.e. two triangles forming a rectangle)
    //
    // - [-1, +1] in XY
    // - [0, 0] in Z
    // - normal == (0, 0, 1)
    template<typename TVert>
    [[nodiscard]] std::array<TVert, 6> generateQuad() noexcept;

    template<>
    [[nodiscard]] std::array<PlainVert, 6> generateQuad() noexcept;

    // generate 2D circle verts for a circle with a specified number of
    // triangle segments
    //
    // - [-1, +1] in XY (r = 1.0)
    // - [0, 0] in Z
    // - normal == (0, 0, 1)
    template<typename TVert>
    void generateCircle(size_t, std::vector<TVert>&);

    template<>
    void generateCircle(size_t, std::vector<PlainVert>&);

    template<typename TVert>
    [[nodiscard]] std::vector<TVert> generateCircle(size_t segments) {
        std::vector<TVert> rv;
        generateCircle(segments, rv);
        return rv;
    }

    // axis-aligned bounding box (AABB)
    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;
    };

    // print an AABB to an output stream
    std::ostream& operator<<(std::ostream&, AABB const&);

    // returns the center point of an AABB
    [[nodiscard]] inline constexpr glm::vec3 aabbCenter(AABB const& a) noexcept {
        return (a.min+a.max)/2.0f;
    }

    // returns the dimensions of an AABB
    [[nodiscard]] inline constexpr glm::vec3 aabbDimensions(AABB const& a) noexcept {
        return a.max - a.min;
    }

    // returns a vec that has the smallest X, Y, and Z found in the provided vecs
    [[nodiscard]] inline constexpr glm::vec3 vecMin(glm::vec3 const& a, glm::vec3 const& b) noexcept {
        glm::vec3 rv{};
        rv.x = std::min(a.x, b.x);
        rv.y = std::min(a.y, b.y);
        rv.z = std::min(a.z, b.z);
        return rv;
    }

    // returns a vec that has the largest X, Y, and Z found in the provided vecs
    [[nodiscard]] inline constexpr glm::vec3 vecMax(glm::vec3 const& a, glm::vec3 const& b) noexcept {
        glm::vec3 rv{};
        rv.x = std::max(a.x, b.x);
        rv.y = std::max(a.y, b.y);
        rv.z = std::max(a.z, b.z);
        return rv;
    }

    // returns the smallest AABB that spans the provided verts
    template<typename Vert>
    [[nodiscard]] inline constexpr AABB aabbFromVerts(Vert const* vs, size_t n) noexcept {
        AABB rv;

        if (n == 0) {
            rv.min = {0.0f, 0.0f, 0.0f};
            rv.max = {0.0f, 0.0f, 0.0f};
            return rv;
        }

        rv.min = {FLT_MAX, FLT_MAX, FLT_MAX};
        rv.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (size_t i = 0; i < n; ++i) {
            glm::vec3 const& pos = vs[i].pos;
            rv.min = vecMin(rv.min, pos);
            rv.max = vecMax(rv.max, pos);
        }

        return rv;
    }

    template<typename Container>
    [[nodiscard]] inline AABB aabbFromVerts(Container const& c) noexcept {
        return aabbFromVerts(c.data(), c.size());
    }

    // returns an AABB that spans the provided AABB and the provided point
    [[nodiscard]] inline constexpr AABB aabbUnion(AABB const& a, glm::vec3 const& p) noexcept {
        AABB rv{};
        rv.min = vecMin(a.min, p);
        rv.max = vecMax(a.max, p);
        return rv;
    }

    // returns the smallest AABB that fully spans the provided AABBs
    [[nodiscard]] inline constexpr AABB aabbUnion(AABB const& a, AABB const& b) noexcept {
        AABB rv{};
        rv.min = vecMin(a.min, b.min);
        rv.max = vecMax(a.max, b.max);
        return rv;
    }

    // returns true if the provided AABB is empty (i.e. a point with a volume of zero)
    [[nodiscard]] inline constexpr bool aabbIsEmpty(AABB const& a) noexcept {
        return a.min == a.max;
    }

    // returns the *index* of the longest dimension of an AABB
    [[nodiscard]] inline constexpr glm::vec3::length_type aabbLongestDimension(AABB const& a) noexcept {
        glm::vec3 dims = aabbDimensions(a);

        if (dims.x > dims.y && dims.x > dims.z) {
            return 0;  // X is longest
        } else if (dims.y > dims.z) {
            return 1;  // Y is longest
        } else {
            return 2;  // Z is longest
        }
    }

    // parametric line
    struct Line final {
        glm::vec3 o;  // origin
        glm::vec3 d;  // direction - should be normalized
    };

    // print a line to an output stream
    std::ostream& operator<<(std::ostream&, Line const&);

    // parametric sphere
    struct Sphere final {
        glm::vec3 origin;
        float radius;
    };

    // print a sphere to an output stream
    std::ostream& operator<<(std::ostream&, Sphere const&);

    // compute a sphere that spans the supplied vertices
    template<typename Vert>
    [[nodiscard]] inline Sphere boundingSphereFromVerts(Vert const* vs, size_t n) noexcept {
        AABB aabb = aabbFromVerts(vs, n);

        Sphere rv;
        rv.origin = aabbCenter(aabb);
        rv.radius = 0.0f;

        float biggest_r2 = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            glm::vec3 const& pos = vs[i].pos;
            float r2 = glm::dot(pos, pos);
            biggest_r2 = std::max(r2, biggest_r2);
        }

        rv.radius = glm::sqrt(biggest_r2);
        return rv;
    }

    // compute a sphere that spans the supplied vertices
    template<typename Container>
    [[nodiscard]] inline Sphere boundingSphereFromVerts(Container const& c) noexcept {
        return boundingSphereFromVerts(c.data(), c.size());
    }

    // compute an AABB that spans the sphere
    [[nodiscard]] inline AABB sphereAABB(Sphere const& s) noexcept {
        AABB rv;
        rv.min = s.origin - s.radius;
        rv.max = s.origin + s.radius;
        return rv;
    }

    struct LineSphereHittestResult final {
        bool intersected;

        // direction factors in equation P = O + tD
        float t0;
        float t1;
    };

    [[nodiscard]] LineSphereHittestResult lineIntersectsSphere(Sphere const&, Line const&) noexcept;

    struct LineAABBHittestResult final {
        bool intersected;

        // direction factors in equation P = O + tD
        float t0;
        float t1;
    };

    [[nodiscard]] LineAABBHittestResult lineIntersectsAABB(AABB const&, Line const&) noexcept;

    struct Plane final {
        glm::vec3 origin;
        glm::vec3 normal;
    };

    struct LinePlaneHittestResult final {
        bool intersected;
        float t;
    };

    [[nodiscard]] LinePlaneHittestResult lineIntersectsPlane(Plane const&, Line const&) noexcept;

    struct Disc final {
        glm::vec3 origin;
        glm::vec3 normal;
        float radius;
    };

    struct LineDiscHittestResult final {
        bool intersected;
        float t;
    };

    [[nodiscard]] LineDiscHittestResult lineIntersectsDisc(Disc const&, Line const&) noexcept;

    struct LineTriangleHittestResult final {
        bool intersected;
        float t;
    };

    // assumes vec argument points to three vectors
    [[nodiscard]] LineTriangleHittestResult lineIntersectsTriangle(glm::vec3 const*, Line const&) noexcept;

    // generate a model matrix that transforms a generated quad (above) to
    // match the position and orientation of an analytic plane
    [[nodiscard]] glm::mat4 quadToPlaneXform(Plane const&) noexcept;

    // generate a model matrix that transforms the generated circle verts to
    // match the position and orientation of an analytic disc
    [[nodiscard]] glm::mat4 circleToDiscXform(Disc const&) noexcept;

    // generate a model matrix that transforms the generate cube (wireframe)
    // verts to match the position and orientation of an AABB
    [[nodiscard]] glm::mat4 cubeToAABBXform(AABB const&) noexcept;

    // compute a normal from the points of a triangle
    //
    // effectively, (B-A) x (C-A)
    [[nodiscard]] glm::vec3 triangleNormal(glm::vec3 const&, glm::vec3 const&, glm::vec3 const&) noexcept;
}
