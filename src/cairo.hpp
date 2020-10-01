#pragma once

#include <cairo/cairo.h>
#include <stdexcept>

namespace cairo {
    class surface_t final {
        cairo_surface_t* ptr;
    public:
        surface_t(cairo_surface_t* _ptr) : ptr{_ptr} {
            if (ptr == nullptr) {
                throw std::runtime_error{"cairo::surface_t: initialized with nullptr"};
            }
        }
        surface_t(surface_t const&) = delete;
        surface_t(surface_t&&) = delete;
        surface_t& operator=(surface_t const&) = delete;
        surface_t& operator=(surface_t&&) = delete;
        ~surface_t() noexcept {
            cairo_surface_destroy(ptr);
        }

        operator cairo_surface_t* () noexcept {
            return ptr;
        }
    };

    surface_t image_surface_create_for_data(
            unsigned char *data,
            cairo_format_t format,
            int	width,
            int	height,
            int	stride) {
        cairo_surface_t* ptr = cairo_image_surface_create_for_data(
                    data,
                    format,
                    width,
                    height,
                    stride);
        if (not ptr) {
            throw std::runtime_error{"cairo_image_surface_create_for_data: returned null"};
        }

        return surface_t{ptr};
    }

    class t final {
        cairo_t* ptr;
    public:
        t(cairo_t* _ptr) : ptr{_ptr} {
            if (ptr == nullptr) {
                throw std::runtime_error{"cairo::cairo_t: initialized with nullptr"};
            }
        }
        t(cairo_t const&) = delete;
        t(cairo_t&&) = delete;
        t& operator=(t&&) = delete;
        t& operator=(t const&) = delete;
        ~t() noexcept {
            cairo_destroy(ptr);
        }

        operator cairo_t* () noexcept {
            return ptr;
        }
    };

    t create(cairo_surface_t* target) {
        return t{cairo_create(target)};
    }
}
