#include "logl_common.hpp"

namespace {
    struct Gl_State final {
        gl::Vertex_shader vertex_shader =
                gl::CompileVertexShaderFile(gfxplay::resource_path("logl_12_light.vert"));
        gl::Program color_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::CompileFragmentShaderFile(gfxplay::resource_path("logl_12.frag")));
        gl::Program light_prog = gl::CreateProgramFrom(
            vertex_shader,
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
)"
        ));
        gl::Texture_2d container2_tex = gl::load_tex(gfxplay::resource_path("container2.png"));
        gl::Texture_2d container2_spec = gl::load_tex(gfxplay::resource_path("container2_specular.png"));
        gl::Texture_2d container2_emission = gl::load_tex(gfxplay::resource_path("matrix.jpg"));

        static constexpr gl::Attribute_vec3 aPos = gl::Attribute_vec3::at_location(0);
        static constexpr gl::Attribute_vec3 aNormal = gl::Attribute_vec3::at_location(1);
        static constexpr gl::Attribute_vec2 aTexCoords = gl::Attribute_vec2::at_location(2);
        gl::Uniform_mat4 uModel = gl::GetUniformLocation(color_prog, "model");
        gl::Uniform_mat4 uView = gl::GetUniformLocation(color_prog, "view");
        gl::Uniform_mat4 uProjection = gl::GetUniformLocation(color_prog, "projection");
        gl::Uniform_mat3 uNormalMatrix = gl::GetUniformLocation(color_prog, "normalMatrix");

        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(color_prog, "viewPos");
        gl::Uniform_vec3 uDirLightDirection = gl::GetUniformLocation(color_prog, "dirLight.direction");
        gl::Uniform_vec3 uDirLightAmbient = gl::GetUniformLocation(color_prog, "dirLight.ambient");
        gl::Uniform_vec3 uDirLightDiffuse = gl::GetUniformLocation(color_prog, "dirLight.diffuse");
        gl::Uniform_vec3 uDirLightSpecular = gl::GetUniformLocation(color_prog, "dirLight.specular");

        gl::Uniform_int uMaterialDiffuse = gl::GetUniformLocation(color_prog, "material.diffuse");
        gl::Uniform_int uMaterialSpecular = gl::GetUniformLocation(color_prog, "material.specular");
        gl::Uniform_float uMaterialShininess = gl::GetUniformLocation(color_prog, "material.shininess");

        gl::Uniform_mat4 uModelLightProg = gl::GetUniformLocation(light_prog, "model");
        gl::Uniform_mat4 uViewLightProg = gl::GetUniformLocation(light_prog, "view");
        gl::Uniform_mat4 uProjectionLightProg = gl::GetUniformLocation(light_prog, "projection");

        gl::Array_buffer<float> ab = {
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

        gl::Vertex_array color_cube_vao = [this]() {
            gl::BindBuffer(ab);
            gl::VertexAttribPointer(aPos, false, 8*sizeof(GLfloat), 0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aNormal, false, 8*sizeof(GLfloat), 3 * sizeof(GLfloat));
            gl::EnableVertexAttribArray(aNormal);
            gl::VertexAttribPointer(aTexCoords, false, 8*sizeof(GLfloat), 6 * sizeof(GLfloat));
            gl::EnableVertexAttribArray(aTexCoords);
        };

        gl::Vertex_array light_vao = [this]() {
            gl::BindBuffer(ab);
            gl::VertexAttribPointer(aPos, false, 8*sizeof(GLfloat), 0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aNormal, false, 8*sizeof(GLfloat), 3 * sizeof(GLfloat));
            gl::EnableVertexAttribArray(aNormal);
        };

        gl::Program quad_prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTextureCoords;

out vec2 TexCoords;

void main() {
    gl_Position = vec4(aPosition.x, aPosition.y, 0.0f, 1.0f);
    TexCoords = aTextureCoords;
}
)"),
            gl::CompileFragmentShaderFile(gfxplay::resource_path("logl_framebuffers.frag"))
        );

        static constexpr gl::Attribute_vec3 quadProg_aPos = gl::Attribute_vec3::at_location(0);
        static constexpr gl::Attribute_vec2 quadProg_texCoords = gl::Attribute_vec2::at_location(1);

        gl::Array_buffer<float> quadProg_ab = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        gl::Vertex_array quadProg_vao = [this]() {
            gl::BindBuffer(quadProg_ab);
            gl::VertexAttribPointer(quadProg_aPos, false, 4*sizeof(GLfloat), 0);
            gl::EnableVertexAttribArray(quadProg_aPos);
            gl::VertexAttribPointer(quadProg_texCoords, false, 4*sizeof(GLfloat), 2 * sizeof(GLfloat));
            gl::EnableVertexAttribArray(quadProg_texCoords);
        };

        gl::Texture_2d_multisample fbotex = gl::GenTexture2dMultisample();
        gl::Render_buffer depthbuf = gl::GenRenderBuffer();

        gl::Frame_buffer fbo2 = [&]() {
            gl::Frame_buffer fbo = gl::GenFrameBuffer();
            gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);

            // generate texture
            gl::BindTexture(fbotex);
            glTexImage2DMultisample(
                    fbotex.type,
                    16,
                    GL_RGB,
                    1024,
                    768,
                    GL_TRUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl::BindTexture();

            // attach color fbo to the texture
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   fbotex.type,
                                   fbotex,
                                   0);

            // generate depth + stencil rbo, so the pipeline still has a storage
            // area it can use to do those tests
            gl::BindRenderBuffer(depthbuf);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             16,
                                             GL_DEPTH24_STENCIL8,
                                             1024,
                                             768);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      depthbuf.handle);
            gl::BindRenderBuffer();

            AKGL_ASSERT_NO_ERRORS();

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

            return fbo;
        }();

        gl::Texture_2d fbotex_no_multisamp = gl::GenTexture2d();
        gl::Render_buffer depthbuf_no_multisamp = gl::GenRenderBuffer();

        gl::Frame_buffer fbo_no_multisamp = [&]() {
            gl::Frame_buffer fbo = gl::GenFrameBuffer();
            gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);

            // generate texture
            gl::BindTexture(fbotex_no_multisamp.type, fbotex_no_multisamp);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         1024, // TODO: a robust solution would lookup screen size
                         768,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl::BindTexture();

            // attach color fbo to the texture
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   fbotex_no_multisamp,
                                   0);

            // generate depth + stencil rbo, so the pipeline still has a storage
            // area it can use to do those tests
            gl::BindRenderBuffer(depthbuf_no_multisamp);
            glRenderbufferStorage(GL_RENDERBUFFER,
                                  GL_DEPTH24_STENCIL8,
                                  1024,  // TODO
                                  768);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      depthbuf_no_multisamp.handle);
            gl::BindRenderBuffer();


            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
            return fbo;
        }();

        void draw(ui::Game_state const& g) {
            gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo2);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);

            auto ticks = util::now().count() / 200.0f;

            gl::UseProgram(color_prog);

            auto projection = glm::perspective(
                        glm::radians(45.0f),
                        800.0f / 600.0f,
                        0.1f,
                        100.0f);

            gl::Uniform(uView, g.camera.view_mtx());
            gl::Uniform(uProjection, projection);
            gl::Uniform(uViewPos, g.camera.pos);

            // texture mapping
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
            gl::Uniform(uDirLightSpecular, {0.5f, 0.5f, 0.5f});

            static const glm::vec3 pointLightPositions[] = {
                glm::vec3( 0.7f,  0.2f,  2.0f),
                glm::vec3( 2.3f, -3.3f, -4.0f),
                glm::vec3(-4.0f,  2.0f, -12.0f),
                glm::vec3( 0.0f,  0.0f, -3.0f)
            };

            {
                auto setVec3 = [&](const char* name, float x, float y, float z) {
                    auto u = gl::Uniform_vec3{gl::GetUniformLocation(color_prog, name)};
                    gl::Uniform(u, {x, y, z});
                };
                auto setVec3v = [&](const char* name, glm::vec3 const& v) {
                    auto u = gl::Uniform_vec3{gl::GetUniformLocation(color_prog, name)};
                    gl::Uniform(u, v);
                };
                auto setFloat = [&](const char* name, float v) {
                    auto u = gl::Uniform_float{gl::GetUniformLocation(color_prog, name)};
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
                    gl::Uniform(uNormalMatrix, glm::transpose(glm::inverse(model)));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            gl::UseProgram(light_prog);
            gl::Uniform(uViewLightProg, g.camera.view_mtx());
            gl::Uniform(uProjectionLightProg, projection);
            for (auto const& lightPos : pointLightPositions) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, lightPos);
                model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
                gl::Uniform(uModelLightProg, model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }


            // !!! render complete !!!

            // !!! however: beware of multisampling! !!!
            //
            // The render has rendered into a multisampled fbo. This must be
            // blitted to a non-multisampled fbo for the final post-processing
            // pass.
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo2.handle);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_no_multisamp.handle);
            glBlitFramebuffer(0, 0, 1024, 768, 0, 0, 1024, 768, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            // now we have a non-multisampled texture in the no_multisamp one.
            // so now we need to draw onto the actual screen.

            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT);

            gl::UseProgram(quad_prog);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            gl::BindTexture(fbotex_no_multisamp);
            gl::BindVertexArray(quadProg_vao);
            {
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    };
}

int main(int, char**) {
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    auto gls = Gl_State{};

    auto game = ui::Game_state{};
    game.camera.pos = {0.0f, 0.0f, 3.0f};

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    auto throttle = util::Software_throttle{8ms};

    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        while (SDL_PollEvent(&e)) {
            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
        }

        game.tick(dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gls.draw(game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
