#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = []() {
            auto p = gl::Program();
            auto vs = gl::Vertex_shader::Compile(OSC_GLSL_VERSION R"(
in vec3 aPos;

void main() {
gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)");
            auto fs = gl::Fragment_shader::Compile(OSC_GLSL_VERSION R"(
out vec4 FragColor;

void main() {
FragColor = vec4(1.0, 0.5f, 0.2f, 1.0f);
}
)");
            gl::AttachShader(p, vs);
            gl::AttachShader(p, fs);
            gl::LinkProgram(p);
            return p;
        }();

        gl::Attribute aPos = gl::Attribute{prog, "aPos"};
        gl::Array_buffer ab = {};
        gl::Element_array_buffer ebo = {};
        gl::Vertex_array vao = {};

        Gl_State() {
            float vertices[] = {
                 0.5f,  0.5f, 0.0f,  // top right
                 0.5f, -0.5f, 0.0f,  // bottom right
                -0.5f, -0.5f, 0.0f,  // bottom left
                -0.5f,  0.5f, 0.0f   // top left
            };
            unsigned int indices[] = {  // note that we start from 0!
                0, 1, 3,   // first triangle
                1, 2, 3    // second triangle
            };

            gl::BindVertexArray(vao);
            gl::BindBuffer(ab);
            gl::BufferData(ab, sizeof(vertices), vertices, GL_STATIC_DRAW);
            gl::BindBuffer(ebo);
            gl::BufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);
            gl::VertexAttributePointer(aPos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::UseProgram(gls.prog);

    auto throttle = util::Software_throttle{8ms};

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl::BindVertexArray(gls.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        gl::BindVertexArray();

        throttle.wait();

        // draw
        SDL_GL_SwapWindow(s.window);
    }
}
