#include "logl_common.hpp"

namespace {
    static constexpr std::array<glm::vec3, 10> cubePositions = {{
        { 0.0f,  0.0f,  0.0f},
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f},
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f},
        {-1.7f,  3.0f, -7.5f},
        { 1.3f, -2.0f, -2.5f},
        { 1.5f,  2.0f, -2.5f},
        { 1.5f,  0.2f, -1.5f},
        {-1.3f,  1.0f, -1.5f}
    }};

    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
})"),
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
        gl::Uniform_mat4 uModel{prog, "uModel"};
        gl::Uniform_mat4 uView{prog, "uView"};
        gl::Uniform_mat4 uProjection{prog, "uProjection"};
        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};
        gl::Uniform_int uSampler0{prog, "uSampler0"};
        gl::Uniform_int uSampler1{prog, "uSampler1"};

        struct Vbo_data final {
            glm::vec3 pos;
            glm::vec2 uv;

            Vbo_data(glm::vec3 pos_, glm::vec2 uv_) : pos{pos_}, uv{uv_} {
            }
        };

        gl::Array_buffer<Vbo_data> ab = {
           {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
           {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
           {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
           {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
           {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
           {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},

           {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
           {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
           {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
           {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

           {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
           {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
           {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
           {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
           {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
           {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},

           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
           {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
           {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
           {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
           {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},

           {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
           {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
           {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
           {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
           {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
           {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},

           {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
           {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
           {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
           {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
           {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}}
        };

        gl::Vertex_array vao = [this]() {
            gl::BindBuffer(ab);
            gl::VertexAttribPointer(aPos, false, sizeof(Vbo_data), offsetof(Vbo_data, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(Vbo_data), offsetof(Vbo_data, uv));
            gl::EnableVertexAttribArray(aTexCoord);
        };

        void draw() {
            glm::mat4 model = glm::mat4(1.0f);
            float rot = static_cast<float>(util::now().count())/10.0f;
            model = glm::rotate(model, glm::radians(rot), glm::vec3(0.5f, 1.0f, 0.0f));

            glm::mat4 view = glm::mat4(1.0f);
            // note that we're translating the scene in the reverse direction of where we want to move
            view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

            glm::mat4 projection;
            projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

            gl::UseProgram(prog);

            gl::Uniform(uModel, model);
            gl::Uniform(uView, view);
            gl::Uniform(uProjection, projection);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall);
            gl::Uniform(uSampler0, gl::texture_index<GL_TEXTURE0>());

            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face);
            gl::Uniform(uSampler1, gl::texture_index<GL_TEXTURE1>());

            gl::BindVertexArray(vao);
            for(unsigned int i = 0; i < 10; i++) {
                glm::mat4 m = glm::mat4(1.0f);
                m = glm::translate(m, cubePositions[i]);
                float angle = 20.0f * i;
                m = glm::rotate(m, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                gl::Uniform(uModel, m);
                gl::DrawArrays(GL_TRIANGLES, 0, 36);
            }
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
