#include "logl_common.hpp"

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

    struct Gl_State final {
        gl::Vertex_shader vertex_shader = gl::CompileVertexShader(R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Normal = normalMatrix * aNormal;
    FragPos = vec3(model * vec4(aPos, 1.0));
})");
        gl::Program color_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShader(R"(
#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
struct Light {
    vec3 pos;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

void main() {
    // ambient
    vec3 ambient = light.ambient * material.ambient;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.pos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)"
        ));

        gl::Program light_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
)"
        ));
        gl::Attribute aPos = 0;
        gl::Attribute aNormal = 1;
        gl::Uniform_mat4f uModelColorProg = gl::GetUniformLocation(color_prog, "model");
        gl::Uniform_mat4f uViewColorProg = gl::GetUniformLocation(color_prog, "view");
        gl::Uniform_mat4f uProjectionColorProg = gl::GetUniformLocation(color_prog, "projection");
        gl::Uniform_vec3f uViewPosColorProg = gl::GetUniformLocation(color_prog, "viewPos");
        gl::Uniform_mat3f uNormalMatrix = gl::GetUniformLocation(color_prog, "normalMatrix");

        gl::Uniform_vec3f uMaterialAmbient = gl::GetUniformLocation(color_prog, "material.ambient");
        gl::Uniform_vec3f uMaterialDiffuse = gl::GetUniformLocation(color_prog, "material.diffuse");
        gl::Uniform_vec3f uMaterialSpecular = gl::GetUniformLocation(color_prog, "material.specular");
        gl::Uniform_1f uMaterialShininess = gl::GetUniformLocation(color_prog, "material.shininess");

        gl::Uniform_vec3f uLightPos = gl::GetUniformLocation(color_prog, "light.pos");
        gl::Uniform_vec3f uLightAmbient = gl::GetUniformLocation(color_prog, "light.ambient");
        gl::Uniform_vec3f uLightDiffuse = gl::GetUniformLocation(color_prog, "light.diffuse");
        gl::Uniform_vec3f uLightSpecular = gl::GetUniformLocation(color_prog, "light.specular");

        gl::Uniform_mat4f uModelLightProg = gl::GetUniformLocation(light_prog, "model");
        gl::Uniform_mat4f uViewLightProg = gl::GetUniformLocation(light_prog, "view");
        gl::Uniform_mat4f uProjectionLightProg = gl::GetUniformLocation(light_prog, "projection");
        gl::Array_buffer ab = gl::GenArrayBuffer();
        gl::Vertex_array color_cube_vao = gl::GenVertexArrays();
        gl::Vertex_array light_vao = gl::GenVertexArrays();

        Gl_State() {
            float vertices[] = {
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
                 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
                 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
                 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
            };

            gl::BindBuffer(ab.type, ab);
            gl::BufferData(ab.type, sizeof(vertices), vertices, GL_STATIC_DRAW);

            gl::BindVertexArray(color_cube_vao);
            {
                gl::BindBuffer(ab.type, ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
            }

            gl::BindVertexArray(light_vao);
            {
                gl::BindBuffer(ab.type, ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
            }
        }

        void draw(App_State const& as) {
            auto ticks = util::now().count() / 200.0f;
            glm::vec3 lightPos(sin(ticks) * 2.4f, 1.0f, cos(ticks) * 4.0f);
            glm::mat4 projection =
                    glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

            gl::UseProgram(color_prog);

            gl::Uniform(uViewColorProg, as.view_mtx());
            gl::Uniform(uProjectionColorProg, projection);
            gl::Uniform(uViewPosColorProg, as.pos);

            gl::Uniform(uMaterialAmbient, 0.3f * glm::vec3{1.0f, 0.5f, 0.31f});
            gl::Uniform(uMaterialDiffuse, glm::vec3{1.0f, 0.5f, 0.31f});
            gl::Uniform(uMaterialSpecular, glm::vec3{0.5f, 0.5f, 0.5f});
            gl::Uniform(uMaterialShininess, 32.0f);

            gl::Uniform(uLightPos, lightPos);
            auto lightColor = glm::vec3{
                    sin(ticks) * 2.0f,
                    sin(ticks) * 0.7f,
                    sin(ticks) * 1.3f
            };
            gl::Uniform(uLightAmbient, 0.5f * lightColor);
            gl::Uniform(uLightDiffuse, 0.2f * lightColor);
            gl::Uniform(uLightSpecular, glm::vec3{1.0f, 1.0f, 1.0f});

            gl::BindVertexArray(color_cube_vao);
            {
                static const glm::vec3 positions[] = {
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{1.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 1.0f, 0.0f},
                    glm::vec3{0.0f, 0.0f, 1.0f},
                };
                for (auto const& pos : positions) {
                    glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), pos);
                    gl::Uniform(uModelColorProg, model);
                    gl::Uniform(uNormalMatrix, glm::transpose(glm::inverse(model)));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            gl::UseProgram(light_prog);
            gl::Uniform(uViewLightProg, as.view_mtx());
            gl::Uniform(uProjectionLightProg, projection);
            {
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
