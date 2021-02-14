#include "logl_common.hpp"

#include <SDL.h>
#include <cairo/cairo.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>

using std::literals::operator""s;

namespace {
    template<class T>
    struct Point {
        T x;
        T y;
    };

    using Point2d = Point<int>;

    template<class T>
    std::ostream& operator<<(std::ostream& o, Point<T> const& p) {
        return o << p.x << ", " << p.y;
    }

    template<class T>
    std::string to_string(Point<T> const& p) {
        std::stringstream ss;
        ss << p;
        return ss.str();
    }

    struct Dimensions2d {
        int w;
        int h;
    };

    struct Rect {
        int x;
        int y;
        int w;
        int h;
    };

    std::ostream& operator<<(std::ostream& o, Rect const& r) {
        return o << "x = " << r.x << " y = " << r.y << " w = " << r.w << " h = " << r.h;
    }
}

namespace {
    Dimensions2d drawable_area(SDL_Renderer* r) {
        Dimensions2d rv;
        SDL_GetRendererOutputSize(r, &rv.w, &rv.h);
        return rv;
    }
}

namespace cairo {
    class Surface {
        cairo_surface_t* ptr;
    public:
        Surface(SDL_Surface* s) :
            ptr{cairo_image_surface_create_for_data(
                    static_cast<unsigned char*>(s->pixels),
                    CAIRO_FORMAT_ARGB32,
                    s->w,
                    s->h,
                    s->pitch)} {
        }
        Surface(Surface const&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface const&) = delete;
        Surface& operator=(Surface&&) = delete;
        ~Surface() noexcept {
            cairo_surface_destroy(ptr);
        }

        operator cairo_surface_t*() noexcept {
            return ptr;
        }
    };

    class Context {
        cairo_t* ptr;
    public:
        Context(Surface& s) :
            ptr{cairo_create(s)} {
        }
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            cairo_destroy(ptr);
        }

        operator cairo_t*() noexcept {
            return ptr;
        }
    };
}

namespace ui {
    // pairs the raw software drawbuffer provided by SDL with a cairo context
    // that can write into it
    class Cairo_surface final {
        sdl::Surface sdl_surf;
        cairo::Surface cairo_surf;
        cairo::Context cairo_ctx;
    public:
        Cairo_surface(Dimensions2d const& dimensions) :
            sdl_surf{sdl::CreateRGBSurface(0, dimensions.w, dimensions.h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000)},
            cairo_surf{sdl_surf},
            cairo_ctx{cairo_surf} {
        }

        operator cairo_t*() noexcept {
            return cairo_ctx;
        }

        operator cairo_surface_t*() noexcept {
            return cairo_surf;
        }

        sdl::Texture texture(SDL_Renderer* r) {
            return sdl::CreateTextureFromSurface(r, sdl_surf);
        }
    };
}

namespace {
    struct Sim_el final {
        Point2d pos;
    };

    struct Qtree_node final {
        uint32_t idx = 0;
        char n = 0;  // -1 == node, -2 == recycleable node
    };

    struct Qtree final {
        std::vector<Sim_el> data;
        std::vector<Qtree_node> nodes;
        Rect rect;
    };

    void push(Qtree& tree, size_t cur_node_idx, Rect const& bounds, Sim_el const& el) {
        // `cur` getter is necessary because tree.nodes might move memory
        // as it grows
        auto cur = [&]() -> Qtree_node& { return tree.nodes[cur_node_idx]; };

        if (cur().n == -1) {
            // `cur` is an internal node, so recurse to the correct node
            int w = bounds.w/2;
            int h = bounds.h/2;

            size_t child_idx = cur().idx;
            int x = bounds.x;
            if (el.pos.x >= (x + w)) {
                // right-hand half (indices 1/3)
                x += w;
                child_idx += 1;
            }
            int y = bounds.y;
            if (el.pos.y >= (y + h)) {
                // bottom (indices 2/3)
                y += h;
                child_idx += 2;
            }

            auto new_bounds = Rect{.x = x, .y = y, .w = w, .h = h};
            push(tree, child_idx, new_bounds, el);
        } else {
            // `cur` is a leaf node, so populate/split it

            if (cur().n == 0) {
                // `cur` is an *empty* leaf node, so allocate 4 new
                // entries in the tree data vector
                auto len = tree.data.size();
                tree.data.resize(len + 4);
                cur().idx = len;
            }

            if (cur().n == -2) {
                // TODO: this hack just enables data recycling
                cur().n = 0;
            }

            if (cur().n < 4) {
                // `cur` is a leaf node with space available
                tree.data.at(cur().idx + cur().n) = el;
                ++(cur().n);
            } else {
                // `cur` is a full leaf node, so it needs to be
                // split into an internal node.

                // save old data
                auto old_data_idx = cur().idx;

                // reassign `cur` as an internal node
                {
                    auto num_nodes = tree.nodes.size();
                    tree.nodes.resize(num_nodes + 4);
                    cur().idx = num_nodes;
                    cur().n = -1;

                    // TODO: hacky optimization: reuse the old data
                    // entries in the new leaf
                    tree.nodes[num_nodes].n = -2;
                    tree.nodes[num_nodes].idx = old_data_idx;
                }

                // TODO: optimization: reassign the leaf data...

                // add the old leafs into this (now internal) node
                for (auto i = 0u; i < 4; ++i) {
                    push(tree, cur_node_idx, bounds, tree.data.at(old_data_idx + i));
                }
                // and push this el into it also
                push(tree, cur_node_idx, bounds, el);
            }
        }
    }

    Qtree from_points(Rect const& bounds, std::vector<Sim_el> const& points) {
        auto rv = Qtree{
             .data = {},
            .nodes = {Qtree_node{}},
            .rect = bounds,
        };

        for (const Sim_el& e : points) {
            push(rv, 0, rv.rect, e);
        }

        return rv;
    }

    void draw_node(ui::Cairo_surface& csurf, Qtree const& qt, Qtree_node const& n, Rect const& r) {
        if (n.n == -2) {
            return;  // recycled data nodes containing garbage
        }
        if (n.n == -1) {
            cairo_set_source_rgba(csurf, 0, 0, 0, 0.1);
            // grid: vertical
            {
                int x_mid = r.x + r.w/2;
                cairo_move_to(csurf, x_mid, r.y);
                cairo_line_to(csurf, x_mid, r.y + r.h);
            }

            // grid: horizontal
            {
                int y_mid = r.y + r.h/2;
                cairo_move_to(csurf, r.x, y_mid);
                cairo_line_to(csurf, r.x + r.w, y_mid);
            }
            cairo_stroke(csurf);

            // recursively draw sub-trees

            int w = r.w/2;
            int h = r.h/2;

            {
                Rect top_left_rect = { .x = r.x, .y = r.y, .w = w, .h = h };
                Qtree_node const& top_left_node = qt.nodes.at(n.idx + 0);
                draw_node(csurf, qt, top_left_node, top_left_rect);
            }
            {
                Rect top_right_rect = { .x = r.x + w, .y = r.y, .w = w, .h = h };
                Qtree_node const& top_right_node = qt.nodes.at(n.idx + 1);
                draw_node(csurf, qt, top_right_node, top_right_rect);
            }
            {
                Rect bottom_left_rect = { .x = r.x, .y = r.y + h, .w = w, .h = h };
                Qtree_node const& bottom_left_node = qt.nodes.at(n.idx + 2);
                draw_node(csurf, qt, bottom_left_node, bottom_left_rect);
            }
            {
                Rect bottom_right_rect = { .x = r.x + w, .y = r.y + h, .w = w, .h = h };
                Qtree_node const& bottom_right_node = qt.nodes.at(n.idx + 3);
                draw_node(csurf, qt, bottom_right_node, bottom_right_rect);
            }
        } else {
            // draw leafs

            cairo_set_source_rgba(csurf, 0, 0, 0, 1.0);
            unsigned num_leafs = n.n;
            for (auto i = 0u; i < num_leafs; ++i) {
                Sim_el const& el = qt.data.at(n.idx + i);
                //cairo_move_to(csurf, el.pos.x, el.pos.y);
                //cairo_arc(csurf, el.pos.x, el.pos.y, 1, 0, 2*M_PI);
            }
            //cairo_stroke(csurf);
        }
    }

    void draw_qtree(ui::Cairo_surface& csurf, Qtree const& tree) {
        draw_node(csurf, tree, tree.nodes.at(0), tree.rect);
    }
}

int main() {
    auto ctx = sdl::Init(SDL_INIT_VIDEO);
    auto window_dims = Dimensions2d{512, 512};
    auto window = sdl::CreateWindoww(
            "Adam's cool app",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            window_dims.w,
            window_dims.h,
            SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    auto renderer = sdl::CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    auto [w, h] = drawable_area(renderer);
    auto csurf = ui::Cairo_surface{{w, h}};
    auto mousepos = Point2d{0, 0};
    auto device = std::random_device{};
    auto engine = std::default_random_engine{device()};
    auto x_dist = std::normal_distribution<double>{static_cast<double>(w)/2.0, 64.0};
    auto y_dist = std::uniform_int_distribution<int>{0, h};

    auto els = std::vector<Sim_el>(10000);
    for (Sim_el& e : els) {
        e.pos = {static_cast<int>(x_dist(engine)) % w, y_dist(engine)};
    }

    cairo_font_options_t* ft = cairo_font_options_create();
    cairo_font_options_set_antialias(ft, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_font_options_set_hint_metrics(ft, CAIRO_HINT_METRICS_ON);
    cairo_set_font_options(csurf, ft);
    cairo_select_font_face(csurf, "Source Code Pro for Powerline", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(csurf, 24);

    auto qtree = from_points({0, 0, w, h}, els);
    auto drawing_rect = SDL_Rect{ .x = 0, .y = 0, .w = w, .h = h };
    auto selection_area = Rect{ .x = 200, .y = 200, .w = 200, .h = 200 };
    auto last_time = std::chrono::steady_clock::now();
    size_t frame_num = 0;

    for (;;) {
        // clear
        cairo_set_source_rgb(csurf, 1, 1, 1);
        cairo_paint(csurf);

        draw_qtree(csurf, qtree);

        cairo_set_source_rgba(csurf, 1, 0, 0, 0.1);
        cairo_rectangle(csurf, selection_area.x, selection_area.y, selection_area.w, selection_area.h);
        cairo_fill(csurf);
        cairo_stroke(csurf);

        cairo_set_source_rgb(csurf, 1, 0, 0);

        // fps counter
        {
            auto t = std::chrono::steady_clock::now();
            auto dur = t - last_time;
            last_time = t;
            auto fps = std::chrono::seconds{1} / dur;

            cairo_set_source_rgb(csurf, 0, 0, 0);
            cairo_move_to(csurf, 100, 100);
            cairo_show_text(csurf, std::to_string(fps).c_str());
        }

        // frame number
        {
            cairo_set_source_rgb(csurf, 0, 0, 0);
            cairo_move_to(csurf, 100, 150);
            cairo_show_text(csurf, std::to_string(frame_num).c_str());
            ++frame_num;
        }

        sdl::Texture t = csurf.texture(renderer);
        SDL_RenderCopy(renderer, t, &drawing_rect, &drawing_rect);
        SDL_RenderPresent(renderer);

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                return 0;
            } else if (e.type == SDL_MOUSEMOTION) {
                mousepos = {e.motion.x, e.motion.y};
            }
        }
    }

    return 0;
}
