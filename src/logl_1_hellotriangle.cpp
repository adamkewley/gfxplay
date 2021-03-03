#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)"),
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5f, 0.2f, 1.0f);
}
)"
        ));
        gl::Attribute_vec3 aPos = gl::Attribute_vec3::with_name(prog, "aPos");

        gl::Array_buffer<glm::vec3> vbo = {
            { 0.5f,  0.5f, 0.0f},  // top right
            { 0.5f, -0.5f, 0.0f},  // bottom right
            {-0.5f, -0.5f, 0.0f},  // bottom left
            {-0.5f,  0.5f, 0.0f},  // top left
        };

        gl::Element_array_buffer<unsigned> ebo = {
            0, 1, 3,
            1, 2, 3
        };

        gl::Vertex_array vao = [this]() {
            gl::BindBuffer(vbo);
            gl::BindBuffer(ebo);
            gl::VertexAttribPointer(aPos, false, sizeof(glm::vec3), 0);
            gl::EnableVertexAttribArray(aPos);
        };

        void draw() {
            gl::BindVertexArray(vao);
            gl::DrawElements(GL_TRIANGLES, ebo.sizei(), GL_UNSIGNED_INT, nullptr);
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
