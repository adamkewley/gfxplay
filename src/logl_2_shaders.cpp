#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = []() {
            auto p = gl::Program();
            auto vs = gl::Vertex_shader::Compile(OSC_GLSL_VERSION R"(
layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aColor; // the color variable has attribute position 1

out vec3 ourColor; // output a color to the fragment shader

void main()
{
  gl_Position = vec4(aPos, 1.0);
  ourColor = aColor; // set ourColor to the input color we got from the vertex data
}
)");
            auto fs = gl::Fragment_shader::Compile(OSC_GLSL_VERSION R"(
out vec4 FragColor;
in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)");
            gl::AttachShader(p, vs);
            gl::AttachShader(p, fs);
            gl::LinkProgram(p);
            return p;
        }();

        gl::Attribute aPos = gl::Attribute{0};
        gl::Attribute aColor = gl::Attribute{1};
        gl::Array_buffer ab = {};
        gl::Vertex_array vao = {};

        Gl_State() {
            float vertices[] = {
                // positions         // colors
                 0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
                -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
                 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top
            };

            gl::BindVertexArray(vao);
            gl::BindBuffer(ab);
            gl::BufferData(ab, sizeof(vertices), vertices, GL_STATIC_DRAW);
            gl::VertexAttributePointer(aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttributePointer(aColor, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3* sizeof(float)));
            gl::EnableVertexAttribArray(aColor);
            gl::BindVertexArray();
        }

        void draw() {
            gl::UseProgram(prog);
            gl::BindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            gl::BindVertexArray();
            gl::UseProgram();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto throttle = util::Software_throttle{8ms};

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gls.draw();

        throttle.wait();

        SDL_GL_SwapWindow(s.window);
    }
}
