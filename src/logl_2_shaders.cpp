#include "logl_common.hpp"

#include <array>

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(R"(
#version 330 core

layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aColor; // the color variable has attribute position 1

out vec3 ourColor; // output a color to the fragment shader

void main() {
  gl_Position = vec4(aPos, 1.0);
  ourColor = aColor; // set ourColor to the input color we got from the vertex data
}
)"),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;
in vec3 ourColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)"
        ));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aColor = gl::AttributeAtLocation(1);

        struct Vert final {
            glm::vec3 pos;
            glm::vec3 color;
        };
        static_assert(sizeof(Vert) == 6*sizeof(float));

        static constexpr std::array<Vert, 3> triangle = {{
            // positions           // colors
            {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }};

        gl::Array_buffer vbo = []() {
            gl::Array_buffer rv = gl::GenArrayBuffer();
            gl::BindBuffer(rv);
            gl::BufferData(rv.type, sizeof(triangle), triangle.data(), GL_STATIC_DRAW);
            return rv;
        }();

        gl::Vertex_array vao = [this]() {
            gl::Vertex_array rv = gl::GenVertexArrays();
            gl::BindVertexArray(rv);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), reinterpret_cast<void*>(offsetof(Vert, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aColor, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), reinterpret_cast<void*>(offsetof(Vert, color)));
            gl::EnableVertexAttribArray(aColor);
            gl::BindVertexArray();
            return rv;
        }();

        void draw() {
            gl::UseProgram(prog);
            gl::BindVertexArray(vao);
            gl::DrawArrays(GL_TRIANGLES, 0, triangle.size());
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
