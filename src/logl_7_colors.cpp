#include "logl_common.hpp"

static const glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

namespace {
    struct App_State final {
        glm::vec3 pos = {0.0f, 0.0f, 3.0f};
        float pitch = 0.0f;
        float yaw = -pi_f/2.0f;
        bool moving_forward = false;
        bool moving_backward = false;
        bool moving_left = false;
        bool moving_right = false;
        bool moving_up = false;
        bool moving_down = false;

        glm::vec3 front() const {
            return glm::normalize(glm::vec3{
                cos(yaw) * cos(pitch),
                sin(pitch),
                sin(yaw) * cos(pitch),
            });
        }

        glm::vec3 up() const {
            return glm::vec3(0.0f, 1.0f, 0.0f);
        }

        glm::vec3 right() const {
            return glm::normalize(glm::cross(front(), up()));
        }

        glm::mat4 view_mtx() const {
            return glm::lookAt(pos, pos + front(), up());
        }
    };

    static const char vertex_shader_src[] = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    struct Gl_State final {
        gl::Program color_prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(vertex_shader_src),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    FragColor = vec4(lightColor * objectColor, 1.0);
}
)"
        ));
        gl::Program light_prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(vertex_shader_src),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
)"
        ));
        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        gl::Uniform_mat4f uModelColorProg = gl::GetUniformLocation(color_prog, "model");
        gl::Uniform_mat4f uViewColorProg = gl::GetUniformLocation(color_prog, "view");
        gl::Uniform_mat4f uProjectionColorProg = gl::GetUniformLocation(color_prog, "projection");
        gl::Uniform_vec3f uObjectColor = gl::GetUniformLocation(color_prog, "objectColor");
        gl::Uniform_vec3f uLightColor = gl::GetUniformLocation(color_prog, "lightColor");
        gl::Uniform_mat4f uModelLightProg = gl::GetUniformLocation(light_prog, "model");
        gl::Uniform_mat4f uViewLightProg = gl::GetUniformLocation(light_prog, "view");
        gl::Uniform_mat4f uProjectionLightProg = gl::GetUniformLocation(light_prog, "projection");
        gl::Array_buffer ab = gl::GenArrayBuffer();
        gl::Vertex_array color_cube_vao = gl::GenVertexArrays();
        gl::Vertex_array light_vao = gl::GenVertexArrays();

        Gl_State() {
            float vertices[] = {
                -0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,

                -0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,

                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,

                -0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f
            };

            // color cube binding
            gl::BindVertexArray(color_cube_vao);
            {
                gl::BindBuffer(ab.type, ab);
                gl::BufferData(ab.type, sizeof(vertices), vertices, GL_STATIC_DRAW);

                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
            }

            gl::BindVertexArray(light_vao);
            {
                gl::BindBuffer(ab.type, ab);

                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
            }
        }

        void draw(App_State const& as) {
            glm::mat4 projection =
                    glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

            gl::UseProgram(color_prog);

            gl::Uniform(uViewColorProg, as.view_mtx());
            gl::Uniform(uProjectionColorProg, projection);
            gl::Uniform(uObjectColor, glm::vec3{1.0f, 0.5f, 0.31f});
            gl::Uniform(uLightColor, glm::vec3{1.0f, 1.0f, 1.0f});

            gl::BindVertexArray(color_cube_vao);
            {
                glm::mat4 model = glm::mat4{1.0f};
                gl::Uniform(uModelColorProg, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            gl::UseProgram(light_prog);
            gl::Uniform(uViewLightProg, as.view_mtx());
            gl::Uniform(uProjectionLightProg, projection);
            {
                glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, lightPos);
                model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
                gl::Uniform(uModelLightProg, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    };
}

int main(int, char**) {
    constexpr float camera_speed = 0.1f;
    constexpr float mouse_sensitivity = 0.001f;

    auto s = ui::Window_state{};
    SDL_SetWindowGrab(s.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    auto gls = Gl_State{};
    auto as = App_State{};

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    auto throttle = util::Software_throttle{8ms};

    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    std::chrono::milliseconds dt = {};
    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        dt = cur_time - last_time;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return 0;
            } else if (e.type == SDL_KEYDOWN or e.type == SDL_KEYUP) {
                bool is_button_down = e.type == SDL_KEYDOWN ? true : false;
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    as.moving_forward = is_button_down;
                    break;
                case SDLK_s:
                    as.moving_backward = is_button_down;
                    break;
                case SDLK_d:
                    as.moving_left = is_button_down;
                    break;
                case SDLK_a:
                    as.moving_right = is_button_down;
                    break;
                case SDLK_SPACE:
                    as.moving_up = is_button_down;
                    break;
                case SDLK_LCTRL:
                    as.moving_down = is_button_down;
                    break;
                case SDLK_ESCAPE:
                    return 0;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                as.yaw += e.motion.xrel * mouse_sensitivity;
                as.pitch -= e.motion.yrel * mouse_sensitivity;
                if (as.pitch > pi_f/2.0f - 0.5f) {
                    as.pitch = pi_f/2.0f - 0.5f;
                }
                if (as.pitch < -pi_f/2.0f + 0.5f) {
                    as.pitch = -pi_f/2.0f + 0.5f;
                }
            }
        }

        if (as.moving_forward) {
            as.pos += camera_speed * as.front();
        }

        if (as.moving_backward) {
            as.pos -= camera_speed * as.front();
        }

        if (as.moving_left) {
            as.pos += camera_speed * as.right();
        }

        if (as.moving_right) {
            as.pos -= camera_speed * as.right();
        }

        if (as.moving_up) {
            as.pos += camera_speed * as.up();
        }

        if (as.moving_down) {
            as.pos -= camera_speed * as.up();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gls.draw(as);

        throttle.wait();

        SDL_GL_SwapWindow(s.window);
    }
}
