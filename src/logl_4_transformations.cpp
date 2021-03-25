#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

uniform mat4 uTransform;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main() {
    gl_Position = uTransform * vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
}
)"),
            gl::Fragment_shader::from_source(R"(
#version 330 core

uniform sampler2D uSampler0;
uniform sampler2D uSampler1;

in vec2 TexCoord;
out vec4 FragColor;

void main() {
    FragColor = mix(texture(uSampler0, TexCoord), texture(uSampler1, TexCoord), 0.2);
})"
        ));
        gl::Texture_2d wall = gl::load_tex(gfxplay::resource_path("wall.jpg"));
        gl::Texture_2d face = gl::load_tex(gfxplay::resource_path("awesomeface.png"));
        gl::Uniform_mat4 uTransform{prog, "uTransform"};
        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};
        gl::Uniform_int uSampler0{prog, "uSampler0"};
        gl::Uniform_int uSampler1{prog, "uSampler1"};

        gl::Array_buffer<float> ab = {
            // positions          // colors           // texture coords
             0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
             0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
        };

        gl::Element_array_buffer<GLuint> ebo = {
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
        };

        gl::Vertex_array vao = [this]() {
            gl::BindBuffer(ab);
            gl::VertexAttribPointer(aPos, false, 8*sizeof(GLfloat), 0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, 8*sizeof(GLfloat), 6* sizeof(float));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindBuffer(ebo);
        };

        void draw() {
            gl::UseProgram(prog);

            glm::mat4 trans = glm::mat4(1.0f);
            float angle = static_cast<float>(util::now().count()) / 10.0f;
            trans = glm::rotate(trans, glm::radians(angle), glm::vec3(0.0, 0.0, 1.0));
            trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
            gl::Uniform(uTransform, trans);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall);
            gl::Uniform(uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face);
            gl::Uniform(uSampler1, gl::texture_index<GL_TEXTURE1>());

            gl::BindVertexArray(vao);
            gl::DrawElements(GL_TRIANGLES, ebo.sizei(), gl::index_type(ebo), nullptr);
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
