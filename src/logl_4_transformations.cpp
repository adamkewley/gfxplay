#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Program prog = []() {
            auto p = gl::Program();
            auto vs = gl::Vertex_shader::Compile(OSC_GLSL_VERSION R"(
uniform mat4 uTransform;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = uTransform * vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
}
)");
            auto fs = gl::Fragment_shader::Compile(OSC_GLSL_VERSION R"(
uniform sampler2D uSampler0;
uniform sampler2D uSampler1;

in vec2 TexCoord;
out vec4 FragColor;

void main()
{
    FragColor = mix(texture(uSampler0, TexCoord), texture(uSampler1, TexCoord), 0.2);
}
)");
            gl::AttachShader(p, vs);
            gl::AttachShader(p, fs);
            gl::LinkProgram(p);
            return p;
        }();

        gl::Texture_2d wall = util::mipmapped_texture(RESOURCES_DIR "wall.jpg");
        gl::Texture_2d face = util::mipmapped_texture(RESOURCES_DIR "awesomeface.png");
        gl::UniformMatrix4fv uTransform = {prog, "uTransform"};
        gl::Attribute aPos = {0};
        gl::Attribute aTexCoord = {1};
        gl::Uniform1i uSampler0 = {prog, "uSampler0"};
        gl::Uniform1i uSampler1 = {prog, "uSampler1"};
        gl::Array_buffer ab = {};
        gl::Element_array_buffer ebo = {};
        gl::Vertex_array vao = {};

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
            gl::BufferData(ab, sizeof(vertices), vertices, GL_STATIC_DRAW);

            gl::VertexAttributePointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
            gl::EnableVertexAttribArray(aPos);

            gl::VertexAttributePointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(6* sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoord);

            gl::BindBuffer(ebo);
            gl::BufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);

            gl::BindVertexArray();
        }

        void draw() {
            gl::UseProgram(prog);

            glm::mat4 trans = glm::mat4(1.0f);
            float angle = static_cast<float>(util::now().count()) / 10.0f;
            trans = glm::rotate(trans, glm::radians(angle), glm::vec3(0.0, 0.0, 1.0));
            trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
            util::Uniform(uTransform, trans);

            glActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wall);
            gl::Uniform(uSampler0, 0);

            glActiveTexture(GL_TEXTURE1);
            gl::BindTexture(face);
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
