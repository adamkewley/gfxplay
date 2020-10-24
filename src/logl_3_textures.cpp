#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
})"),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D uSampler0;
uniform sampler2D uSampler1;

void main() {
    FragColor = mix(texture(uSampler0, TexCoord), texture(uSampler1, TexCoord), 0.2);
})"));
        gl::Texture_2d wall = gl::flipped_and_mipmapped_texture(RESOURCES_DIR "wall.jpg");
        gl::Texture_2d face = gl::flipped_and_mipmapped_texture(RESOURCES_DIR "awesomeface.png");
        gl::Attribute aPos = 0;
        gl::Attribute aColor = 1;
        gl::Attribute aTexCoord = 2;
        gl::Uniform_1i uSampler0 = gl::GetUniformLocation(prog, "uSampler0");
        gl::Uniform_1i uSampler1 = gl::GetUniformLocation(prog, "uSampler1");
        gl::Array_buffer ab = gl::GenArrayBuffer();
        gl::Element_array_buffer ebo = gl::GenElementArrayBuffer();
        gl::Vertex_array vao = gl::GenVertexArrays();

        Gl_State() {
            float vertices[] = {
                // positions          // colors           // texture coords
                 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
                 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
                -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
                -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
            };

            unsigned int indices[] = {
                0, 1, 3,   // first triangle
                1, 2, 3    // second triangle
            };

            gl::BindVertexArray(vao);

            gl::BindBuffer(ab);
            gl::BufferData(ab.type, sizeof(vertices), vertices, GL_STATIC_DRAW);

            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
            gl::EnableVertexAttribArray(aPos);

            gl::VertexAttribPointer(aColor, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(3* sizeof(float)));
            gl::EnableVertexAttribArray(aColor);

            gl::VertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(6* sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoord);

            gl::BindBuffer(ebo);
            gl::BufferData(ebo.type, sizeof(indices), indices, GL_STATIC_DRAW);

            gl::BindVertexArray();
        }

        void draw() {
            gl::UseProgram(prog);

            glActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall.type, wall);
            gl::Uniform(uSampler0, 0);

            glActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face.type, face);
            gl::Uniform(uSampler1, 1);

            gl::BindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            gl::BindVertexArray();

            gl::UseProgram();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

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
