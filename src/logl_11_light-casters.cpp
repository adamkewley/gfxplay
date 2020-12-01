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
layout (location = 2) in vec2 aTexCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Normal = normalMatrix * aNormal;
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;
}
)");
        gl::Program color_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShader(R"(
#version 330 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
};
struct Light {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // attenuation
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

void main() {
    vec3 lightDir = normalize(light.position - FragPos);
    float theta     = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // do lighting calculations
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // diffuse
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    vec3 emission = vec3(texture(material.emission, TexCoords));

    vec3 result = attenuation * (ambient + intensity*(diffuse + specular)) + 0.2f*emission;
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
        gl::Texture_2d container2_tex = gl::flipped_and_mipmapped_texture(RESOURCES_DIR "container2.png");
        gl::Texture_2d container2_spec = gl::flipped_and_mipmapped_texture(RESOURCES_DIR "container2_specular.png");
        gl::Texture_2d container2_emission = gl::flipped_and_mipmapped_texture(RESOURCES_DIR "matrix.jpg");

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
        static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);
        gl::Uniform_mat4 uModelColorProg = gl::GetUniformLocation(color_prog, "model");
        gl::Uniform_mat4 uViewColorProg = gl::GetUniformLocation(color_prog, "view");
        gl::Uniform_mat4 uProjectionColorProg = gl::GetUniformLocation(color_prog, "projection");
        gl::Uniform_vec3 uViewPosColorProg = gl::GetUniformLocation(color_prog, "viewPos");
        gl::Uniform_mat3 uNormalMatrix = gl::GetUniformLocation(color_prog, "normalMatrix");

        gl::Uniform_int uMaterialDiffuse = gl::GetUniformLocation(color_prog, "material.diffuse");
        gl::Uniform_int uMaterialSpecular = gl::GetUniformLocation(color_prog, "material.specular");
        gl::Uniform_int uMaterialEmission = gl::GetUniformLocation(color_prog, "material.emission");
        gl::Uniform_1f uMaterialShininess = gl::GetUniformLocation(color_prog, "material.shininess");

        gl::Uniform_vec3 uLightPosition = gl::GetUniformLocation(color_prog, "light.position");
        gl::Uniform_vec3 uLightDirection = gl::GetUniformLocation(color_prog, "light.direction");
        gl::Uniform_1f uLightCutOff = gl::GetUniformLocation(color_prog, "light.cutOff");
        gl::Uniform_1f uLightOuterCutOff = gl::GetUniformLocation(color_prog, "light.outerCutOff");
        gl::Uniform_vec3 uLightAmbient = gl::GetUniformLocation(color_prog, "light.ambient");
        gl::Uniform_vec3 uLightDiffuse = gl::GetUniformLocation(color_prog, "light.diffuse");
        gl::Uniform_vec3 uLightSpecular = gl::GetUniformLocation(color_prog, "light.specular");
        gl::Uniform_1f uLightConstant = gl::GetUniformLocation(color_prog, "light.constant");
        gl::Uniform_1f uLightLinear = gl::GetUniformLocation(color_prog, "light.linear");
        gl::Uniform_1f uLightQuadratic = gl::GetUniformLocation(color_prog, "light.quadratic");

        gl::Uniform_mat4 uModelLightProg = gl::GetUniformLocation(light_prog, "model");
        gl::Uniform_mat4 uViewLightProg = gl::GetUniformLocation(light_prog, "view");
        gl::Uniform_mat4 uProjectionLightProg = gl::GetUniformLocation(light_prog, "projection");
        gl::Array_buffer ab = gl::GenArrayBuffer();
        gl::Vertex_array color_cube_vao = gl::GenVertexArrays();
        gl::Vertex_array light_vao = gl::GenVertexArrays();

        Gl_State() {
            float vertices[] = {
                // positions          // normals           // texture coords
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
                -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
                 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
            };

            gl::BindBuffer(ab.type, ab);
            gl::BufferData(ab.type, sizeof(vertices), vertices, GL_STATIC_DRAW);

            gl::BindVertexArray(color_cube_vao);
            {
                gl::BindBuffer(ab.type, ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
                gl::VertexAttribPointer(aTexCoords, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aTexCoords);
            }

            gl::BindVertexArray(light_vao);
            {
                gl::BindBuffer(ab.type, ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
            }
        }

        void draw(App_State const& as) {
            auto ticks = util::now().count() / 200.0f;

            glm::mat4 projection =
                    glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

            gl::UseProgram(color_prog);

            gl::Uniform(uViewColorProg, as.view_mtx());
            gl::Uniform(uProjectionColorProg, projection);
            gl::Uniform(uViewPosColorProg, as.pos);

            {
                gl::Uniform(uMaterialDiffuse, 0);
                glActiveTexture(GL_TEXTURE0);
                gl::BindTexture(container2_tex.type, container2_tex);
            }

            {
                gl::Uniform(uMaterialSpecular, 1);
                glActiveTexture(GL_TEXTURE1);
                gl::BindTexture(container2_spec.type, container2_spec);
            }

            {
                gl::Uniform(uMaterialEmission, 2);
                glActiveTexture(GL_TEXTURE2);
                gl::BindTexture(container2_emission.type, container2_emission);
            }
            gl::Uniform(uMaterialShininess, 32.0f);

            gl::Uniform(uLightPosition, as.pos);
            gl::Uniform(uLightDirection, as.front());
            gl::Uniform(uLightCutOff, glm::cos(glm::radians(12.5f)));
            gl::Uniform(uLightOuterCutOff, glm::cos(glm::radians(13.5f)));

            auto lightColor = glm::vec3{1.0f, 1.0f, 1.0f};
            gl::Uniform(uLightAmbient, 0.2f * lightColor);
            gl::Uniform(uLightDiffuse, 0.4f * lightColor);
            gl::Uniform(uLightSpecular, glm::vec3{1.0f, 1.0f, 1.0f});
            gl::Uniform(uLightConstant, 1.0f);
            gl::Uniform(uLightLinear, 0.09f);
            gl::Uniform(uLightQuadratic, 0.032f);

            gl::BindVertexArray(color_cube_vao);
            {
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
                int i = 0;
                for (auto const& pos : cubePositions) {
                    glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), pos);
                    float angle = 20.0f * i++;
                    model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                    gl::Uniform(uModelColorProg, model);
                    gl::Uniform(uNormalMatrix, glm::transpose(glm::inverse(model)));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
/*
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
            */
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

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
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
