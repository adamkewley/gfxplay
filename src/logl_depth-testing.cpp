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
        gl::Vertex_shader vertex_shader =
                gl::CompileVertexShaderFile(RESOURCES_DIR "logl_12_light.vert");
        gl::Program color_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

float near = 0.1;
float far  = 100.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
    FragColor = vec4(vec3(depth), 1.0);
}
)"
        ));
        gl::Program light_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
)"
        ));
        gl::Texture_2d container2_tex = gl::mipmapped_texture(RESOURCES_DIR "container2.png");
        gl::Texture_2d container2_spec = gl::mipmapped_texture(RESOURCES_DIR "container2_specular.png");
        gl::Texture_2d container2_emission = gl::mipmapped_texture(RESOURCES_DIR "matrix.jpg");

        gl::Attribute aPos = 0;
        gl::Attribute aNormal = 1;
        gl::Attribute aTexCoords = 2;
        gl::Uniform_mat4f uModel = gl::GetUniformLocation(color_prog, "model");
        gl::Uniform_mat4f uView = gl::GetUniformLocation(color_prog, "view");
        gl::Uniform_mat4f uProjection = gl::GetUniformLocation(color_prog, "projection");

        gl::Uniform_mat4f uModelLightProg = gl::GetUniformLocation(light_prog, "model");
        gl::Uniform_mat4f uViewLightProg = gl::GetUniformLocation(light_prog, "view");
        gl::Uniform_mat4f uProjectionLightProg = gl::GetUniformLocation(light_prog, "projection");
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

            gl::BindBuffer(ab);
            gl::BufferData(ab.type, sizeof(vertices), vertices, GL_STATIC_DRAW);

            gl::BindVertexArray(color_cube_vao);
            {
                gl::BindBuffer(ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
                gl::VertexAttribPointer(aTexCoords, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aTexCoords);
            }

            gl::BindVertexArray(light_vao);
            {
                gl::BindBuffer(ab);
                gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), nullptr);
                gl::EnableVertexAttribArray(aPos);
                gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
                gl::EnableVertexAttribArray(aNormal);
            }
        }

        void draw(App_State const& as) {
            glDepthFunc(GL_ALWAYS);

            auto ticks = util::now().count() / 200.0f;

            gl::UseProgram(color_prog);

            auto projection = glm::perspective(
                        glm::radians(45.0f),
                        800.0f / 600.0f,
                        0.1f,
                        100.0f);

            gl::Uniform(uView, as.view_mtx());
            gl::Uniform(uProjection, projection);
            //gl::Uniform(uViewPos, as.pos);

            /* texture mapping
            {
                gl::Uniform(uMaterialDiffuse, 0);
                glActiveTexture(GL_TEXTURE0);
                gl::BindTexture(container2_tex);
            }

            {
                gl::Uniform(uMaterialSpecular, 1);
                glActiveTexture(GL_TEXTURE1);
                gl::BindTexture(container2_spec);
            }
            gl::Uniform(uMaterialShininess, 32.0f);

            gl::Uniform(uDirLightDirection, {-0.2f, -1.0f, -0.3f});
            gl::Uniform(uDirLightAmbient, {0.05f, 0.05f, 0.05f});
            gl::Uniform(uDirLightDiffuse, {0.4f, 0.4f, 0.4f});
            gl::Uniform(uDirLightSpecular, {0.5f, 0.5f, 0.5f}); */

            static const glm::vec3 pointLightPositions[] = {
                glm::vec3( 0.7f,  0.2f,  2.0f),
                glm::vec3( 2.3f, -3.3f, -4.0f),
                glm::vec3(-4.0f,  2.0f, -12.0f),
                glm::vec3( 0.0f,  0.0f, -3.0f)
            };

            /*
            {
                auto setVec3 = [&](const char* name, float x, float y, float z) {
                    auto u = gl::UniformVec3f{color_prog, name};
                    gl::Uniform(u, {x, y, z});
                };
                auto setVec3v = [&](const char* name, glm::vec3 const& v) {
                    auto u = gl::UniformVec3f{color_prog, name};
                    gl::Uniform(u, v);
                };
                auto setFloat = [&](const char* name, float v) {
                    auto u = gl::Uniform1f{color_prog, name};
                    gl::Uniform(u, v);
                };

                // directional light
                setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
                setVec3("dirLight.ambient", 0.3f, 0.05f, 0.05f);
                setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
                setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
                // point light 1
                setVec3v("pointLights[0].position", pointLightPositions[0]);
                setVec3("pointLights[0].ambient", 0.05f, 0.5f, 0.05f);
                setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
                setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
                setFloat("pointLights[0].constant", 1.0f);
                setFloat("pointLights[0].linear", 0.09);
                setFloat("pointLights[0].quadratic", 0.032);
                // point light 2
                setVec3v("pointLights[1].position", pointLightPositions[1]);
                setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
                setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
                setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
                setFloat("pointLights[1].constant", 1.0f);
                setFloat("pointLights[1].linear", 0.09);
                setFloat("pointLights[1].quadratic", 0.032);
                // point light 3
                setVec3v("pointLights[2].position", pointLightPositions[2]);
                setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
                setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
                setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
                setFloat("pointLights[2].constant", 1.0f);
                setFloat("pointLights[2].linear", 0.09);
                setFloat("pointLights[2].quadratic", 0.032);
                // point light 4
                setVec3v("pointLights[3].position", pointLightPositions[3]);
                setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
                setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
                setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
                setFloat("pointLights[3].constant", 1.0f);
                setFloat("pointLights[3].linear", 0.09);
                setFloat("pointLights[3].quadratic", 0.032);
                // spotLight
                //setVec3v("spotLight.position", as.pos);
                //setVec3v("spotLight.direction", as.front());
                //setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
                //setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
                //setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
                //setFloat("spotLight.constant", 1.0f);
                //setFloat("spotLight.linear", 0.09);
                //setFloat("spotLight.quadratic", 0.032);
                //setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
                //setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
            }
            */


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
                    gl::Uniform(uModel, model);
                    //gl::Uniform(uNormalMatrix, glm::transpose(glm::inverse(model)));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            gl::UseProgram(light_prog);
            gl::Uniform(uViewLightProg, as.view_mtx());
            gl::Uniform(uProjectionLightProg, projection);
            for (auto const& lightPos : pointLightPositions) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, lightPos);
                model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
                gl::Uniform(uModelLightProg, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

    public:
        gl::Uniform_mat4f getUProjectionColorProg() const;
        void setUProjectionColorProg(const gl::Uniform_mat4f &value);
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
