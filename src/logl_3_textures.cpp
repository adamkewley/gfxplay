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
        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aColor = gl::AttributeAtLocation(1);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(2);
        gl::Uniform_int uSampler0 = gl::GetUniformLocation(prog, "uSampler0");
        gl::Uniform_int uSampler1 = gl::GetUniformLocation(prog, "uSampler1");

        struct Vbo_data final {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 tex_coord;
        };
        static_assert(sizeof(Vbo_data) == 8*sizeof(float), "unexpected size: this code relies on a strict struct layout for attrib pointers");

        static constexpr std::array<Vbo_data, 4> cube_verts = {{
            // pos                 // color            // tex_coord
            {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // top right
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom right
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // bottom left
            {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // top left
        }};

        gl::Array_buffer vbo = []() {
            gl::Array_buffer rv = gl::GenArrayBuffer();
            gl::BindBuffer(rv);
            gl::BufferData(rv.type, sizeof(cube_verts), cube_verts.data(), GL_STATIC_DRAW);
            return rv;
        }();

        static constexpr std::array<unsigned, 6> cube_els = {
            0, 1, 3,  // first triangle
            1, 2, 3,  // second triangle
        };

        gl::Element_array_buffer ebo = []() {
            gl::Element_array_buffer rv = gl::GenElementArrayBuffer();
            gl::BindBuffer(rv);
            gl::BufferData(rv.type, sizeof(cube_els), cube_els.data(), GL_STATIC_DRAW);
            return rv;
        }();

        gl::Vertex_array vao = [this]() {
            gl::Vertex_array rv = gl::GenVertexArrays();

            gl::BindVertexArray(rv);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vbo_data), reinterpret_cast<void*>(offsetof(Vbo_data, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aColor, 3, GL_FLOAT, GL_FALSE, sizeof(Vbo_data), reinterpret_cast<void*>(offsetof(Vbo_data, color)));
            gl::EnableVertexAttribArray(aColor);
            gl::VertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vbo_data), reinterpret_cast<void*>(offsetof(Vbo_data, tex_coord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindBuffer(ebo);
            gl::BindVertexArray();

            return rv;
        }();

        gl::Texture_2d wall =
            gl::load_tex(gfxplay::resource_path("wall.jpg"));
        gl::Texture_2d face =
            gl::load_tex(gfxplay::resource_path("awesomeface.png"), gl::TexFlag_Flip_Pixels_Vertically);

        void draw() {
            gl::UseProgram(prog);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall.type, wall);
            gl::Uniform(uSampler0, 0);

            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face.type, face);
            gl::Uniform(uSampler1, 1);

            gl::BindVertexArray(vao);
            gl::DrawElements(GL_TRIANGLES, cube_els.size(), GL_UNSIGNED_INT, nullptr);
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
