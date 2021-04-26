#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
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
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D uSampler0;
uniform sampler2D uSampler1;

void main() {
    FragColor = mix(texture(uSampler0, TexCoord), texture(uSampler1, TexCoord), 0.2);
})"));
        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec3 aColor{1};
        static constexpr gl::Attribute_vec2 aTexCoord{2};
        gl::Uniform_int uSampler0{prog, "uSampler0"};
        gl::Uniform_int uSampler1{prog, "uSampler1"};

        struct Vbo_data final {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 tex_coord;
        };

        gl::Array_buffer<Vbo_data> vbo = {
            // pos                 // color            // tex_coord
            {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top right
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom right
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom left
            {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top left
        };

        gl::Element_array_buffer<GLuint> ebo = {
            0, 1, 3,  // first triangle
            1, 2, 3,  // second triangle
        };

        gl::Vertex_array vao = [this]() {
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(Vbo_data), offsetof(Vbo_data, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aColor, false, sizeof(Vbo_data), offsetof(Vbo_data, color));
            gl::EnableVertexAttribArray(aColor);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(Vbo_data), offsetof(Vbo_data, tex_coord));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindBuffer(ebo);
        };

        gl::Texture_2d wall =
            gl::load_tex(gfxplay::resource_path("wall.jpg"));
        gl::Texture_2d face =
            gl::load_tex(gfxplay::resource_path("awesomeface.png"), gl::TexFlag_Flip_Pixels_Vertically);

        void draw() {
            gl::UseProgram(prog);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall);
            gl::Uniform(uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face);
            gl::Uniform(uSampler1, gl::texture_index<GL_TEXTURE1>());

            gl::BindVertexArray(vao);
            gl::DrawElements(GL_TRIANGLES, ebo.sizei(), gl::index_type(ebo), nullptr);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    auto s = ui::Window_state{};
    auto gls = Gl_State{};

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

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
