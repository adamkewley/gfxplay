#include "logl_common.hpp"
#include "ak_common-shaders.hpp"
#include "logl_model.hpp"

#include <random>

// renders geometry into gbuffers for deferred rendering
//
// MRT shader: assumes 3 FBOs are attached (albedo+spec, position, normals)
struct Gbuffer_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("deferred1.vert"),
        gl::CompileFragmentShaderResource("deferred1.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aNormal {1};
    static constexpr gl::Attribute_vec2 aTexCoords{2};

    gl::Uniform_mat4 uModelMtx{prog, "uModelMtx"};
    gl::Uniform_mat4 uViewMtx{prog, "uViewMtx"};
    gl::Uniform_mat4 uProjMtx{prog, "uProjMtx"};
    gl::Uniform_mat3 uNormalMtx{prog, "uNormalMtx"};
    gl::Uniform_sampler2d uDiffuseTex{prog, "uDiffuseTex"};
    gl::Uniform_sampler2d uSpecularTex{prog, "uSpecularTex"};

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo, gl::Element_array_buffer<unsigned>* ebo = nullptr) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        if (ebo) {
            gl::BindBuffer(*ebo);
        }
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aNormal, false, sizeof(T), offsetof(T, norm));
        gl::EnableVertexAttribArray(aNormal);
        gl::VertexAttribPointer(aTexCoords, false, sizeof(T), offsetof(T, uv));
        gl::EnableVertexAttribArray(aTexCoords);
        gl::BindVertexArray();

        return vao;
    }
};

// Blinn-Phong deferred shading shader: uses info in gbuffer to render scene
struct Deferred2_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("deferred2.vert"),
        gl::CompileFragmentShaderResource("deferred2.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoords{1};

    gl::Uniform_sampler2d gPosition{prog, "gPosition"};
    gl::Uniform_sampler2d gNormal{prog, "gNormal"};
    gl::Uniform_sampler2d gAlbedoSpec{prog, "gAlbedoSpec"};
    // TODO: deal with bullshit light bindings: LearnOpenGL uses some horrendous
    //       string construction to deal with this
    gl::Uniform_vec3 viewPos{prog, "viewPos"};

    template<typename Vbo, typename T = typename Vbo::value_type>
    static gl::Vertex_array create_vao(Vbo& vbo) {
        gl::Vertex_array vao;

        gl::BindVertexArray(vao);
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
        gl::EnableVertexAttribArray(aPos);
        gl::VertexAttribPointer(aTexCoords, false, sizeof(T), offsetof(T, uv));
        gl::EnableVertexAttribArray(aTexCoords);
        gl::BindVertexArray();

        return vao;
    }
};

struct Light { glm::vec3 pos; glm::vec3 color; };

template<size_t N>
static std::array<Light, N> generate_lights() {
    std::default_random_engine engine{std::random_device{}()};
    std::uniform_real_distribution pos_dist{-3.0f, 3.0f};
    std::uniform_real_distribution color_dist{0.6f, 1.0f};

    std::array<Light, N> rv;
    for (Light& l : rv) {
        l.pos = {pos_dist(engine), pos_dist(engine), pos_dist(engine)};
        l.color = {color_dist(engine), color_dist(engine), color_dist(engine)};
    }
    return rv;
}

struct Renderer final {
    gl::Texture_2d container_diff{
        gl::load_tex(gfxplay::resource_path("textures/container2.png").c_str(), gl::TexFlag_SRGB)
    };

    gl::Texture_2d container_spec{
        gl::load_tex(gfxplay::resource_path("textures/container2_specular.png").c_str())
    };

    gl::Array_buffer<Shaded_textured_vert> cube_vbo{
        shaded_textured_cube_verts
    };

    gl::Array_buffer<Shaded_textured_vert> quad_vbo{
        shaded_textured_quad_verts
    };

    static constexpr int nr_lights = 32;
    std::array<Light, nr_lights> lights = generate_lights<nr_lights>();

    gl::Texture_2d gPosition_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        glTexImage2D(t.type, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        gl::TextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return t;
    }();

    gl::Texture_2d gNormal_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        gl::TextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return t;
    }();

    gl::Texture_2d gAlbedoSpec_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ui::window_width, ui::window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::TextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return t;
    }();

    gl::Render_buffer gDepth_rbo = []() {
        gl::Render_buffer rbo;
        gl::BindRenderBuffer(rbo);
        gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ui::window_width, ui::window_height);
        return rbo;
    }();

    gl::Frame_buffer gbuffer_fbo = [this]() {
        gl::Frame_buffer fbo;
        gl::BindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition_tex.raw_handle(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal_tex.raw_handle(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec_tex.raw_handle(), 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepth_rbo.raw_handle());
        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2);

        gl::assert_current_fbo_complete();

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);

        return fbo;
    }();

    Gbuffer_shader gbs;
    gl::Vertex_array gbs_cube_vao = Gbuffer_shader::create_vao(cube_vbo);

    std::shared_ptr<model::Model> backpack = model::load_model_cached(gfxplay::resource_path("backpack/backpack.obj").c_str());
    std::vector<gl::Vertex_array> backpack_vaos = [this]() {
        std::vector<gl::Vertex_array> rv;
        for (model::Mesh& mesh : backpack->meshes) {
            rv.push_back(Gbuffer_shader::create_vao<gl::Array_buffer<model::Mesh_vert>, model::Mesh_vert>(mesh.vbo, &mesh.ebo));
        }
        return rv;
    }();

    static constexpr std::array<glm::vec3, 9> backpack_positions = {{
        {-3.0, -0.5, -3.0},
        { 0.0, -0.5, -3.0},
        { 3.0, -0.5, -3.0},
        {-3.0, -0.5,  0.0},
        { 0.0, -0.5,  0.0},
        { 3.0, -0.5,  0.0},
        {-3.0, -0.5,  3.0},
        { 0.0, -0.5,  3.0},
        { 3.0, -0.5,  3.0},
    }};

    Plain_texture_shader pts;
    gl::Vertex_array pts_quad_vao = create_vao(pts, quad_vbo);
    Deferred2_shader d2s;
    gl::Vertex_array d2s_quad_vao = Deferred2_shader::create_vao(quad_vbo);
    Uniform_color_shader ucs;
    gl::Vertex_array ucs_cube_vao = create_vao(ucs, cube_vbo);

    bool debug_mode = false;

    void draw(ui::Window_state&, ui::Game_state& s) {
        gl::BindFramebuffer(GL_FRAMEBUFFER, gbuffer_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::UseProgram(gbs.prog);


        gl::Uniform(gbs.uViewMtx, s.camera.view_mtx());
        gl::Uniform(gbs.uProjMtx, s.camera.persp_mtx());


        // render cube
        if (false) {
            glm::mat4 cube_model = glm::identity<glm::mat4>();
            gl::Uniform(gbs.uModelMtx, cube_model);
            gl::Uniform(gbs.uNormalMtx, gl::normal_matrix(cube_model));

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(container_diff);
            gl::Uniform(gbs.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());
            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(container_spec);
            gl::Uniform(gbs.uSpecularTex, gl::texture_index<GL_TEXTURE1>());

            gl::BindVertexArray(gbs_cube_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            gl::BindVertexArray();
        }

        // render backpacks
        {
            for (size_t i = 0; i < backpack->meshes.size(); ++i) {
                model::Mesh& mesh = backpack->meshes[i];

                // bind to first diffuse texture
                for (std::shared_ptr<model::Mesh_tex>& t : mesh.textures) {
                    if (t->type == model::Tex_type::diffuse) {
                        gl::ActiveTexture(GL_TEXTURE0);
                        gl::BindTexture(t->handle);
                        gl::Uniform(gbs.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());
                        break;
                    }
                }

                // bind to first specular texture
                for (std::shared_ptr<model::Mesh_tex>& t : mesh.textures) {
                    if (t->type == model::Tex_type::specular) {
                        gl::ActiveTexture(GL_TEXTURE1);
                        gl::BindTexture(t->handle);
                        gl::Uniform(gbs.uSpecularTex, gl::texture_index<GL_TEXTURE1>());
                        break;
                    }
                }

                gl::BindVertexArray(backpack_vaos[i]);
                for (glm::vec3 const& pos : backpack_positions) {
                    glm::mat4 model{1.0f};
                    model = glm::translate(model, pos);
                    model = glm::scale(model, glm::vec3(0.25f));
                    gl::Uniform(gbs.uModelMtx, model);
                    gl::Uniform(gbs.uNormalMtx, gl::normal_matrix(model));
                    gl::DrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
                }
                gl::BindVertexArray();
            }
        }


        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (not debug_mode) {
            gl::UseProgram(d2s.prog);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(gPosition_tex);
            gl::Uniform(d2s.gPosition, gl::texture_index<GL_TEXTURE0>());
            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(gNormal_tex);
            gl::Uniform(d2s.gNormal, gl::texture_index<GL_TEXTURE1>());
            gl::ActiveTexture(GL_TEXTURE2);
            gl::BindTexture(gAlbedoSpec_tex);
            gl::Uniform(d2s.gAlbedoSpec, gl::texture_index<GL_TEXTURE2>());
            gl::Uniform(d2s.viewPos, s.camera.pos);

            for (size_t i = 0; i < lights.size(); ++i) {

                Light const& light = lights[i];

                // TODO: this bs should be replaced with arrays and/or direct
                // struct memory mapping
                char buf[64];  // swap space for string formatting

                std::snprintf(buf, sizeof(buf), "lights[%zu].Position", i);
                gl::Uniform_vec3 uLightPos{d2s.prog, buf};

                std::snprintf(buf, sizeof(buf), "lights[%zu].Color", i);
                gl::Uniform_vec3 uLightColor{d2s.prog, buf};

                std::snprintf(buf, sizeof(buf), "lights[%zu].Linear", i);
                gl::Uniform_float uLightLinear{d2s.prog, buf};

                std::snprintf(buf, sizeof(buf), "lights[%zu].Quadratic", i);
                gl::Uniform_float uLightQuadratic{d2s.prog, buf};

                std::snprintf(buf, sizeof(buf), "lights[%zu].Radius", i);
                gl::Uniform_float uLightRadius{d2s.prog, buf};

                // perform any necessary per-light calcs
                const float constant = 1.0f;
                const float linear = 0.3f;
                const float quadratic = 0.8f;
                glm::vec3 const& color = light.color;
                const float maxBrightness = std::fmaxf(std::fmaxf(color.r, color.g), color.b);
                float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);

                // set uniforms
                gl::Uniform(uLightPos, light.pos);
                gl::Uniform(uLightColor, light.color);
                gl::Uniform(uLightLinear, linear);
                gl::Uniform(uLightQuadratic, quadratic);
                gl::Uniform(uLightRadius, radius);   
            }

            gl::BindVertexArray(d2s_quad_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            gl::BindVertexArray();

            // render lights
            {
                // use gBuffer's depth buffer in screen FBO, so that lights
                // obey depth information (remember, the 3D scene being rendered
                // before this point is just a flat quad)
                gl::BindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer_fbo);
                gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, gl::window_fbo);
                glBlitFramebuffer(0, 0, ui::window_width, ui::window_height, 0, 0, ui::window_width, ui::window_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);

                gl::UseProgram(ucs.p);
                gl::Uniform(ucs.uView, s.camera.view_mtx());
                gl::Uniform(ucs.uProjection, s.camera.persp_mtx());
                gl::BindVertexArray(ucs_cube_vao);
                for (Light const& l : lights) {
                    glm::mat4 model{1.0f};
                    model = glm::translate(model, l.pos);
                    model = glm::scale(model, glm::vec3(0.1f));
                    gl::Uniform(ucs.uModel, model);
                    gl::Uniform(ucs.uColor, l.color);
                    gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
                }
                gl::BindVertexArray();
            }
        } else {
            // in debug mode, draw each gbuffer texture into a separate quad

            gl::UseProgram(pts.p);
            gl::Uniform(pts.uView, gl::identity_val);
            gl::Uniform(pts.uProjection, gl::identity_val);

            gl::BindVertexArray(pts_quad_vao);

            // top-left: albedo (color)
            {
                glm::mat4 tl = glm::identity<glm::mat4>();
                tl = glm::translate(tl, glm::vec3{-0.5f, 0.5f, 0.0f});
                tl = glm::scale(tl, glm::vec3{0.5f});
                gl::Uniform(pts.uModel, tl);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(gAlbedoSpec_tex);
                gl::Uniform(pts.uTexture1, gl::texture_index<GL_TEXTURE0>());
                gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());

                // albedo is encoded in the RGB channels, specular in A, so
                // identity-map RGB and discard A
                glm::mat4 ignore_alpha = {
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, INFINITY,
                //  r     g     b     a  (column-major)
                };

                gl::Uniform(pts.uSamplerMultiplier, ignore_alpha);
            }

            // top-right: albedo (spec)
            {
                glm::mat4 tl = glm::identity<glm::mat4>();
                tl = glm::translate(tl, glm::vec3{+0.5f, 0.5f, 0.0f});
                tl = glm::scale(tl, glm::vec3{0.5f});
                gl::Uniform(pts.uModel, tl);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(gAlbedoSpec_tex);
                gl::Uniform(pts.uTexture1, gl::texture_index<GL_TEXTURE0>());

                // specular is encoded in the alpha (A) channel, albedo in RGB,
                // so map the A channel equally onto RGB (essentially:
                // greyscale) and ignore the A channel
                glm::mat4 put_alpha_into_rgb = {
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    1.0f, 1.0f, 1.0f, INFINITY,
                //  r     g     b     a  (column-major)
                };
                gl::Uniform(pts.uSamplerMultiplier, put_alpha_into_rgb);
                gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            }

            // bottom-left: normals
            {
                glm::mat4 tr = glm::identity<glm::mat4>();
                tr = glm::translate(tr, glm::vec3{-0.5f, -0.5f, 0.0f});
                tr = glm::scale(tr, glm::vec3{0.5f});
                gl::Uniform(pts.uModel, tr);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(gNormal_tex);
                gl::Uniform(pts.uTexture1, gl::texture_index<GL_TEXTURE0>());
                gl::Uniform(pts.uSamplerMultiplier, glm::identity<glm::mat4>());
                gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            }

            // bottom-right: position
            {
                glm::mat4 bm = glm::identity<glm::mat4>();
                bm = glm::translate(bm, glm::vec3{+0.5f, -0.5f, 0.0f});
                bm = glm::scale(bm, glm::vec3{0.5f});
                gl::Uniform(pts.uModel, bm);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(gPosition_tex);
                gl::Uniform(pts.uTexture1, gl::texture_index<GL_TEXTURE0>());
                gl::Uniform(pts.uSamplerMultiplier, glm::identity<glm::mat4>());
                gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            }
            gl::BindVertexArray();
        }
    }
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    // IMPORTANT: because the gbuffer writes into all channels of the textures
    //            (e.g. specular is written into the alpha channel)
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // game loop
    Renderer renderer;
    ui::Game_state game;
    util::Software_throttle throttle{8ms};
    std::chrono::milliseconds last_time = util::now();

    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_e) {
                renderer.debug_mode = not renderer.debug_mode;
            }
        }

        game.tick(dt);
        renderer.draw(sdl, game);
        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
