#include "logl_common.hpp"
#include <algorithm>
#include <array>

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

        glm::mat4 persp_mtx() const {
            return glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        }
    };

    struct Gl_State final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(RESOURCES_DIR "logl_blending.vert"),
            gl::CompileFragmentShaderFile(RESOURCES_DIR "logl_blending.frag"));
        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(1);
        gl::Uniform_mat4f uModel = gl::GetUniformLocation(prog, "model");
        gl::Uniform_mat4f uView = gl::GetUniformLocation(prog, "view");
        gl::Uniform_mat4f uProjection = gl::GetUniformLocation(prog, "projection");
        gl::Texture_2d tex_marble =
                gl::flipped_and_mipmapped_texture(RESOURCES_DIR "textures/marble.jpg");
        gl::Texture_2d tex_floor =
                gl::flipped_and_mipmapped_texture(RESOURCES_DIR "textures/metal.png");
        gl::Texture_2d tex_grass =
                gl::flipped_and_mipmapped_texture(RESOURCES_DIR "textures/window.png");
        gl::Array_buffer cube_vbo = []() {
            static const float cubeVertices[] = {
                // back face
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
                 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
                // front face
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
                 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
                 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
                -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
                // left face
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
                -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
                // right face
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
                 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
                 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
                 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
                // bottom face
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
                 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
                // top face
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
                -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f  // top-left
            };
            auto vbo = gl::GenArrayBuffer();
            gl::BindBuffer(vbo);
            gl::BufferData(vbo.type, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
            return vbo;
        }();

        gl::Vertex_array cube_vao = [&]() {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(cube_vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoords, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoords);
            gl::BindVertexArray();
            return vao;
        }();

        gl::Array_buffer plane_vbo = []() {
            static const float planeVertices[] = {
                // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
                 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
                -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
                -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

                 5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
                -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
                 5.0f, -0.5f, -5.0f,  2.0f, 2.0f,
            };
            auto vbo = gl::GenArrayBuffer();
            gl::BindBuffer(vbo);
            gl::BufferData(vbo.type, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
            return vbo;
        }();

        gl::Vertex_array plane_vao = [&]() {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(plane_vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoords, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoords);
            gl::BindVertexArray();
            return vao;
        }();

        gl::Array_buffer transparent_vbo = []() {
            static const float transparentVertices[] = {
                // positions         // texture Coords
                0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
                0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
                1.0f, -0.5f,  0.0f,  1.0f,  0.0f,

                0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
                1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
                1.0f,  0.5f,  0.0f,  1.0f,  1.0f,
            };
            auto vbo = gl::GenArrayBuffer();
            gl::BindBuffer(vbo);
            gl::BufferData(vbo.type, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
            return vbo;
        }();

        gl::Vertex_array transparent_vao = [&]() {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoords, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(aTexCoords);
            gl::BindVertexArray();
            return vao;
        }();

        std::array<glm::vec3, 5> vegetation = {
            glm::vec3(-1.5f, 0.0f, -0.48f),
            glm::vec3( 1.5f, 0.0f, 0.51f),
            glm::vec3( 0.0f, 0.0f, 0.7f),
            glm::vec3(-0.3f, 0.0f, -2.3f),
            glm::vec3(0.5f, 0.0f, -0.6f),
        };

        gl::Program normals_prog = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(RESOURCES_DIR "logl_geom_normals.vert"),
            gl::CompileFragmentShaderFile(RESOURCES_DIR "logl_geom_normals.frag"),
            gl::CompileGeometryShaderFile(RESOURCES_DIR "logl_geom_normals.geom"));
        static constexpr gl::Attribute normals_aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute normals_aNormal = gl::AttributeAtLocation(1);
        gl::Uniform_mat4f normals_uModel = gl::GetUniformLocation(normals_prog, "model");
        gl::Uniform_mat4f normals_uView = gl::GetUniformLocation(normals_prog, "view");
        gl::Uniform_mat4f normals_uProjection = gl::GetUniformLocation(normals_prog, "projection");
        gl::Array_buffer normals_vbo = []() {
            static const float vertices[] = {
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
            auto vbo = gl::GenArrayBuffer();
            gl::BindBuffer(vbo);
            gl::BufferData(vbo.type, sizeof(vertices), vertices, GL_STATIC_DRAW);
            return vbo;
        }();
        gl::Vertex_array normals_vao = [&]() {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(normals_vbo);
            gl::VertexAttribPointer(normals_aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), nullptr);
            gl::EnableVertexAttribArray(normals_aPos);
            gl::VertexAttribPointer(normals_aNormal, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(normals_aNormal);
            gl::BindVertexArray();
            return vao;
        }();

        void draw(App_State const& as) {
            gl::UseProgram(prog);
            gl::Uniform(uView, as.view_mtx());
            gl::Uniform(uProjection, as.persp_mtx());

            glActiveTexture(GL_TEXTURE0);

            // cubes
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glFrontFace(GL_CCW);
            gl::BindVertexArray(cube_vao);
            gl::BindTexture(tex_marble);
            {
                auto model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
                gl::Uniform(uModel, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            {
                auto model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
                gl::Uniform(uModel, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // floor
            gl::BindVertexArray(plane_vao);
            gl::BindTexture(tex_floor);
            gl::Uniform(uModel, glm::mat4(1.0f));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // transparent
            glDisable(GL_CULL_FACE);  // because we can "see through" the back
            gl::BindVertexArray(transparent_vao);
            gl::BindTexture(tex_grass);

            // sort transparent els by closeness to camera, so that they are
            // drawn in the correct order for blending
            std::sort(
                vegetation.begin(),
                vegetation.end(),
                [&](glm::vec3 const& a, glm::vec3 const& b) {
                    // TODO: doesn't need to be sqrt'ed
                    return glm::distance(as.pos, b) < glm::distance(as.pos, a);
                });

            for (auto const& loc : vegetation) {
                auto model = glm::mat4(1.0f);
                model = glm::translate(model, loc);
                gl::Uniform(uModel, model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            gl::BindVertexArray();

            gl::assert_no_errors("a");

            // normals thing
            {
                gl::UseProgram(normals_prog);
                auto model = glm::identity<glm::mat4>();
                gl::assert_no_errors("b");
                gl::Uniform(normals_uModel, model);
                gl::Uniform(normals_uView, as.view_mtx());
                gl::Uniform(normals_uProjection, as.persp_mtx());
                gl::assert_no_errors("uniform");
                gl::BindVertexArray(normals_vao);
                gl::assert_no_errors("vao");
                glDrawArrays(GL_TRIANGLES, 0, 36);
                gl::assert_no_errors("draw");
                gl::BindVertexArray();
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

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilMask(0xff);
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto throttle = util::Software_throttle{8ms};

    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    std::chrono::milliseconds dt;
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        gls.draw(as);

        throttle.wait();

        SDL_GL_SwapWindow(s.window);
    }
}
