#include "logl_common.hpp"

#include <array>

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aColor; // the color variable has attribute position 1

out vec3 ourColor; // output a color to the fragment shader

void main() {
  gl_Position = vec4(aPos, 1.0);
  ourColor = aColor; // set ourColor to the input color we got from the vertex data
}
)"),
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;
in vec3 ourColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)"
        ));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec3 aColor{1};

        struct Vert final {
            glm::vec3 pos;
            glm::vec3 color;
        };
        static_assert(sizeof(Vert) == 6*sizeof(float));

        gl::Array_buffer<Vert> vbo = {
            // positions           // colors
            {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        };

        gl::Vertex_array vao = [&vbo = this->vbo]() {
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(Vert), offsetof(Vert, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aColor, false, sizeof(Vert), offsetof(Vert, color));
            gl::EnableVertexAttribArray(aColor);
        };

        void draw() {
            gl::UseProgram(prog);
            gl::BindVertexArray(vao);
            gl::DrawArrays(GL_TRIANGLES, 0, vbo.sizei());
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto throttle = util::Software_throttle{8ms};

    while (true) {
        for (SDL_Event e; SDL_PollEvent(&e);) {
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gls.draw();

        throttle.wait();

        SDL_GL_SwapWindow(s.window);
    }
}
