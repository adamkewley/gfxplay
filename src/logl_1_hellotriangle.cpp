#include "logl_common.hpp"

#include <array>

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(R"(
#version 330 core

in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)"),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5f, 0.2f, 1.0f);
}
)"
        ));
        gl::Attribute aPos = gl::GetAttribLocation(prog, "aPos");

        // a rectangle consists of four vertices
        static constexpr std::array<glm::vec3, 4> rect_verts = {{
            { 0.5f,  0.5f, 0.0f},  // top right
            { 0.5f, -0.5f, 0.0f},  // bottom right
            {-0.5f, -0.5f, 0.0f},  // bottom left
            {-0.5f,  0.5f, 0.0f},   // top left
        }};
        static_assert(sizeof(glm::vec3) == 3*sizeof(float));

        gl::Array_buffer vbo = []() {
            gl::Array_buffer rv = gl::GenArrayBuffer();
            gl::BindBuffer(rv);
            gl::BufferData(rv.type, sizeof(rect_verts), rect_verts.data(), GL_STATIC_DRAW);
            return rv;
        }();

        // a rectangle is drawn by drawing two triangle primatives that use a
        // combination of the rectangle's vertices
        static constexpr std::array<unsigned, 6> rect_els = {
            0, 1, 3,
            1, 2, 3,
        };

        gl::Element_array_buffer ebo = []() {
            gl::Element_array_buffer rv = gl::GenElementArrayBuffer();
            gl::BindBuffer(rv);
            gl::BufferData(rv.type, sizeof(rect_els), rect_els.data(), GL_STATIC_DRAW);
            return rv;
        }();

        gl::Vertex_array vao = [this]() {
            gl::Vertex_array rv = gl::GenVertexArrays();

            gl::BindVertexArray(rv);
            gl::BindBuffer(vbo);
            gl::BindBuffer(ebo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::BindVertexArray();

            return rv;
        }();

        void draw() {
            gl::BindVertexArray(vao);
            gl::DrawElements(GL_TRIANGLES, rect_els.size(), GL_UNSIGNED_INT, nullptr);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::UseProgram(gls.prog);

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

        // draw
        SDL_GL_SwapWindow(s.window);
    }
}
