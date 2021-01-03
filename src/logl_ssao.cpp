#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

struct Renderer final {
    void draw(ui::Window_state&, ui::Game_state&){}
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // game loop
    Renderer renderer;
    ui::Game_state game;
    util::Software_throttle throttle{8ms};
    std::chrono::milliseconds last_time = util::now();

    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
        }

        game.tick(dt);
        renderer.draw(sdl, game);
        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
