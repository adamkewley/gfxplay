#include "cairo.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#include "sdl.hpp"

using std::string_literals::operator""s;
struct State {
    sdl::Surface s;
    cairo::surface_t cs;
};

void draw_clock(sdl::Surface& s) {
    constexpr double radius = 0.4;
    constexpr double line_width = 0.05;

    auto l = sdl::LockSurface(s);
    cairo::surface_t cairosurf = cairo::image_surface_create_for_data(
        static_cast<unsigned char*>(s->pixels),
        CAIRO_FORMAT_RGB24,
        s->w,
        s->h,
        s->pitch);
    cairo::t cr = cairo::create(cairosurf);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);

    // scale to unit circle and center
    cairo_scale(cr, s->w, s->h);
    cairo_translate(cr, 0.5, 0.5);
    cairo_set_line_width(cr, line_width);

    // green background
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.337, 0.612, 0.117, 0.9);
    cairo_paint(cr);
    cairo_restore(cr);

    // clock
    cairo_arc(cr, 0, 0, radius, 0, 2 * M_PI);
    cairo_save(cr);
    cairo_fill_preserve(cr);
    cairo_restore(cr);
    cairo_clip(cr);

    //clock ticks
    cairo_set_source_rgb(cr, 0, 0, 0);
    for (int i = 0; i < 12; i++) {
        double inset = 0.05;
        cairo_save(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        if (i % 3 != 0) {
            inset *= 0.8;
            cairo_set_line_width(cr, 0.03);
        }

        cairo_move_to(cr,
                      (radius - inset) * cos (i * M_PI / 6),
                      (radius - inset) * sin (i * M_PI / 6));
        cairo_line_to(cr,
                      radius * cos (i * M_PI / 6),
                      radius * sin (i * M_PI / 6));
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    // store the current time
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = localtime (&rawtime);

    // compute the angles of the indicators of our clock
    double minutes = timeinfo->tm_min * M_PI / 30;
    double hours = timeinfo->tm_hour * M_PI / 6;
    double seconds= timeinfo->tm_sec * M_PI / 30;

    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // draw the seconds hand
    cairo_save(cr);
    cairo_set_line_width(cr, line_width / 3);
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.8); // gray
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, sin(seconds) * (radius * 0.9), -cos(seconds) * (radius * 0.9));
    cairo_stroke(cr);
    cairo_restore(cr);

    // draw the minutes hand
    cairo_set_source_rgba(cr, 0.117, 0.337, 0.612, 0.9);   // blue
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, sin(minutes + seconds / 60) * (radius * 0.8), -cos(minutes + seconds / 60) * (radius * 0.8));
    cairo_stroke(cr);

    // draw the hours hand
    cairo_set_source_rgba(cr, 0.337, 0.612, 0.117, 0.9);   // green
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, sin(hours + minutes / 12.0) * (radius * 0.5), -cos(hours + minutes / 12.0) * (radius * 0.5));
    cairo_stroke(cr);
    cairo_restore(cr);

    // draw a little dot in the middle
    cairo_arc(cr, 0, 0, line_width / 3.0, 0, 2 * M_PI);
    cairo_fill(cr);
}

Uint32 tick(Uint32, void*) {
    // https://wiki.libsdl.org/SDL_AddTimer?highlight=%28%5CbCategoryAPI%5Cb%29%7C%28SDLFunctionTemplate%29
    SDL_Event e;
    e.type = SDL_USEREVENT;
    SDL_PushEvent(&e);

    return 1000;  // milliseconds until next tick
}

int main() {
    constexpr int w = 512;
    constexpr int h = 512;
    auto ctx = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    auto win = sdl::CreateWindoww("w", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    auto r = sdl::CreateRenderer(win, -1, 0);
    auto s = sdl::CreateRGBSurface(0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
    auto t = sdl::AddTimer(1000, &tick, nullptr);

    sdl::Event e;
    for (e.type = SDL_FIRSTEVENT;; SDL_WaitEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return 0;
        }

        if (e.type == SDL_USEREVENT || e.type == SDL_FIRSTEVENT) {
            draw_clock(s);
            auto t = sdl::CreateTextureFromSurface(r, s);
            auto rect = SDL_Rect{.x = 0, .y = 0, .w = w, .h = h};
            sdl::RenderCopy(r, t, &rect, &rect);
            sdl::RenderPresent(r);
        }
    }
}
