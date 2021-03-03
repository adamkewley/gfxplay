#include "logl_common.hpp"

#include <array>

namespace {
    struct Instanced_quad_prog final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

out vec3 fColor;

void main() {
    vec2 pos = aPos * (gl_InstanceID / 100.0);
    gl_Position = vec4(pos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
)"),
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

in vec3 fColor;

void main() {
    FragColor = vec4(fColor, 1.0);
}
)"
        ));

        static constexpr gl::Attribute_vec3 aPos = gl::Attribute_vec3::at_location(0);
        static constexpr gl::Attribute_vec3 aColor = gl::Attribute_vec3::at_location(1);
        static constexpr gl::Attribute_vec2 aOffset = gl::Attribute_vec2::at_location(2);

        gl::Array_buffer<float> quad_vbo = {
            // positions     // colors
            -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
             0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
            -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

            -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
             0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
             0.05f,  0.05f,  0.0f, 1.0f, 1.0f,
        };

        static std::array<glm::vec2, 100> generate_translations() {
            std::array<glm::vec2, 100> rv;
            size_t i = 0;
            for(int y = -10; y < 10; y += 2) {
                for(int x = -10; x < 10; x += 2) {
                    constexpr float offset = 0.1f;

                    glm::vec2 translation;
                    translation.x = static_cast<float>(x)/10.0f + offset;
                    translation.y = static_cast<float>(y)/10.0f + offset;
                    rv[i++] = translation;
                }
            }
            return rv;
        }

        gl::Array_buffer<glm::vec2> instance_vbo{generate_translations()};

        gl::Vertex_array quad_vao = [&]() {
            gl::BindBuffer(quad_vbo);
            gl::VertexAttribPointer(aPos, false, 5*sizeof(float), 0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aColor, false, 5*sizeof(float), 2*sizeof(float));
            gl::EnableVertexAttribArray(aColor);

            gl::BindBuffer(instance_vbo);
            gl::VertexAttribPointer(aOffset, false, 2*sizeof(float), 0);
            gl::EnableVertexAttribArray(aOffset);
            gl::BindBuffer();
            gl::VertexAttribDivisor(aOffset, 1);
        };

        void draw(ui::Game_state const&) {
            gl::UseProgram(prog);
            gl::BindVertexArray(quad_vao);
            gl::DrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    //SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Extra GL setup
    auto prog = Instanced_quad_prog{};

    // Game state setup
    auto game = ui::Game_state{};

    // game loop
    auto throttle = util::Software_throttle{8ms};
    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        while (SDL_PollEvent(&e)) {
            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
        }

        game.tick(dt);

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog.draw(game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
