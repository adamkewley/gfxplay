#pragma once

#include <SDL.h>
#undef main
#include <stdexcept>
#include <string>

using std::literals::string_literals::operator""s;

// Thin C++ wrappers around SDL2, so that downstream code can use SDL2 in an
// exception-safe way
namespace sdl {
    // RAII wrapper for the SDL library that calls SDL_Quit() on dtor
    //     https://wiki.libsdl.org/SDL_Quit
    class [[nodiscard]] Context final {
        Context() {}
        friend Context Init(Uint32);
    public:
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            SDL_Quit();
        }
    };

    // RAII'ed version of SDL_Init() that returns a lifetime wrapper that calls
    // SDL_Quit on destruction:
    //     https://wiki.libsdl.org/SDL_Init
    //     https://wiki.libsdl.org/SDL_Quit
    Context Init(Uint32 flags) {
        if (SDL_Init(flags) != 0) {
            throw std::runtime_error{"SDL_Init: failed: "s + SDL_GetError()};
        }
        return Context{};
    }

    // RAII wrapper around SDL_Window that calls SDL_DestroyWindow on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class [[nodiscard]] Window final {
        SDL_Window* ptr;
    public:
        Window(SDL_Window* _ptr) : ptr{_ptr} {
            if (ptr == nullptr) {
                throw std::runtime_error{"sdl::Window: window passed in was null: "s + SDL_GetError()};
            }
        }
        Window(Window const&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) = delete;
        ~Window() noexcept {
            SDL_DestroyWindow(ptr);
        }

        operator SDL_Window* () noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateWindow. The name is a typo because
    // `CreateWindow` is defined in the preprocessor:
    //     https://wiki.libsdl.org/SDL_CreateWindow
    Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags) {
        SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);

        if (win == nullptr) {
            throw std::runtime_error{"SDL_CreateWindow failed: "s + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around an SDL_Renderer that calls SDL_DestroyRenderer on dtor
    //     https://wiki.libsdl.org/SDL_Renderer
    //     https://wiki.libsdl.org/SDL_DestroyRenderer
    class [[nodiscard]] Renderer final {
        SDL_Renderer* ptr;
    public:
        Renderer(SDL_Renderer* _ptr) : ptr{ _ptr } {
            if (ptr == nullptr) {
                throw std::runtime_error{"sdl::Renderer constructor called with null argument: "s + SDL_GetError()};
            }
        }
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept {
            SDL_DestroyRenderer(ptr);
        }

        operator SDL_Renderer* () noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateRenderer
    //     https://wiki.libsdl.org/SDL_CreateRenderer
    Renderer CreateRenderer(SDL_Window* w, int index, Uint32 flags) {
        SDL_Renderer* r = SDL_CreateRenderer(w, index, flags);

        if (r == nullptr) {
            throw std::runtime_error{"SDL_CreateRenderer: failed: "s + SDL_GetError()};
        }

        return Renderer{r};
    }

    // RAII wrapper around SDL_GLContext that calls SDL_GL_DeleteContext on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
        SDL_GLContext ctx;
    public:
        GLContext(SDL_GLContext _ctx) : ctx{ _ctx } {
            if (ctx == nullptr) {
                throw std::runtime_error{"sdl::GLContext: constructor called with null context"};
            }
        }
        GLContext(GLContext const&) = delete;
        GLContext(GLContext&&) = delete;
        GLContext& operator=(GLContext const&) = delete;
        GLContext& operator=(GLContext&&) = delete;
        ~GLContext() noexcept {
            SDL_GL_DeleteContext(ctx);
        }

        operator SDL_GLContext () noexcept {
            return ctx;
        }
    };

    // RAII'ed version of SDL_GL_CreateContext:
    //     https://wiki.libsdl.org/SDL_GL_CreateContext
    GLContext GL_CreateContext(SDL_Window* w) {
        SDL_GLContext ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr) {
            throw std::runtime_error{"SDL_GL_CreateContext failed: "s + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // RAII wrapper for SDL_Surface that calls SDL_FreeSurface on dtor:
    //     https://wiki.libsdl.org/SDL_Surface
    //     https://wiki.libsdl.org/SDL_FreeSurface
    class Surface final {
        SDL_Surface* handle;
    public:
        Surface(SDL_Surface* _handle) :
            handle{_handle} {

            if (handle == nullptr) {
                throw std::runtime_error{"sdl::Surface: null handle passed into constructor"};
            }
        }
        Surface(Surface const&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface const&) = delete;
        Surface& operator=(Surface&&) = delete;
        ~Surface() noexcept {
            SDL_FreeSurface(handle);
        }

        operator SDL_Surface*() noexcept {
            return handle;
        }
        SDL_Surface* operator->() const noexcept {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateRGBSurface:
    //     https://wiki.libsdl.org/SDL_CreateRGBSurface
    Surface CreateRGBSurface(
        Uint32 flags,
        int    width,
        int    height,
        int    depth,
        Uint32 Rmask,
        Uint32 Gmask,
        Uint32 Bmask,
        Uint32 Amask) {

        SDL_Surface* handle = SDL_CreateRGBSurface(
            flags,
            width,
            height,
            depth,
            Rmask,
            Gmask,
            Bmask,
            Amask);

        if (handle == nullptr) {
            throw std::runtime_error{"SDL_CreateRGBSurface: "s + SDL_GetError()};
        }

        return Surface{handle};
    }

    // RAII wrapper around SDL_LockSurface/SDL_UnlockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    //     https://wiki.libsdl.org/SDL_UnlockSurface
    class Surface_lock final {
        SDL_Surface* ptr;
    public:
        Surface_lock(SDL_Surface* s) : ptr{s} {
            if (SDL_LockSurface(ptr) != 0) {
                throw std::runtime_error{"SDL_LockSurface failed: "s + SDL_GetError()};
            }
        }
        Surface_lock(Surface_lock const&) = delete;
        Surface_lock(Surface_lock&&) = delete;
        Surface_lock& operator=(Surface_lock const&) = delete;
        Surface_lock& operator=(Surface_lock&&) = delete;
        ~Surface_lock() noexcept {
            SDL_UnlockSurface(ptr);
        }
    };

    // RAII'ed version of SDL_LockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    Surface_lock LockSurface(SDL_Surface* s) {
        return Surface_lock{s};
    }

    // RAII wrapper around SDL_Texture that calls SDL_DestroyTexture on dtor:
    //     https://wiki.libsdl.org/SDL_Texture
    //     https://wiki.libsdl.org/SDL_DestroyTexture
    class Texture final {
        SDL_Texture* handle;
    public:
        Texture(SDL_Texture* _handle) : handle{_handle} {
            if (handle == nullptr) {
                throw std::runtime_error{"sdl::Texture: null handle passed into constructor"};
            }
        }
        Texture(Texture const&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept {
            SDL_DestroyTexture(handle);
        }

        operator SDL_Texture*() {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateTextureFromSurface:
    //     https://wiki.libsdl.org/SDL_CreateTextureFromSurface
    Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s) {
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        if (t == nullptr) {
            throw std::runtime_error{"SDL_CreateTextureFromSurface failed: "s + SDL_GetError()};
        }
        return Texture{t};
    }

    // https://wiki.libsdl.org/SDL_RenderCopy
    void RenderCopy(SDL_Renderer* r, SDL_Texture* t, SDL_Rect* src, SDL_Rect* dest) {
        int rv = SDL_RenderCopy(r, t, src, dest);
        if (rv != 0) {
            throw std::runtime_error{"SDL_RenderCopy failed: "s + SDL_GetError()};
        }
    }

    // https://wiki.libsdl.org/SDL_RenderPresent
    void RenderPresent(SDL_Renderer* r) {
        // this method exists just so that the namespace-based naming is
        // consistent
        SDL_RenderPresent(r);
    }

    // https://wiki.libsdl.org/SDL_GetWindowSize
    void GetWindowSize(SDL_Window* window, int* w, int* h) {
        SDL_GetWindowSize(window, w, h);
    }

    void GL_SetSwapInterval(int interval) {
        int rv = SDL_GL_SetSwapInterval(interval);

        if (rv != 0) {
            throw std::runtime_error{"SDL_GL_SetSwapInterval failed: "s + SDL_GetError()};
        }
    }

    using Event = SDL_Event;
    using Rect = SDL_Rect;

    class Timer final {
        SDL_TimerID handle;
    public:
        Timer(SDL_TimerID _handle) : handle{_handle} {
            if (handle == 0) {
                throw std::runtime_error{"0 timer handle passed to sdl::Timer"};
            }
        }
        Timer(Timer const&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer const&) = delete;
        Timer& operator=(Timer&&) = delete;
        ~Timer() noexcept {
            SDL_RemoveTimer(handle);
        }
    };

    Timer AddTimer(Uint32 interval,
                         SDL_TimerCallback callback,
                         void* param) {
        SDL_TimerID handle = SDL_AddTimer(interval, callback, param);
        if (handle == 0) {
            throw std::runtime_error{"SDL_AddTimer failed: "s + SDL_GetError()};
        }

        return Timer{handle};
    }
}
