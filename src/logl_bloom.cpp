#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

// a shader that renders multiple lights /w basic Phong shading and also
// writes fragments brighter than some threshold (see fragment shader GLSL) to
// a separate render target
struct Thresholding_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("bloom.vert"),
        gl::CompileFragmentShaderResource("bloom.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);

    gl::Uniform_mat4 uModelMtx = gl::GetUniformLocation(prog, "uModelMtx");
    gl::Uniform_mat4 uViewMtx = gl::GetUniformLocation(prog, "uViewMtx");
    gl::Uniform_mat4 uProjMtx = gl::GetUniformLocation(prog, "uProjMtx");
    gl::Uniform_mat3 uNormalMtx = gl::GetUniformLocation(prog, "uNormalMtx");

    gl::Uniform_vec3 uLightPositions = gl::GetUniformLocation(prog, "uLightPositions");
    gl::Uniform_vec3 uLightColors = gl::GetUniformLocation(prog, "uLightColors");
    gl::Uniform_sampler2d uDiffuseTex = gl::GetUniformLocation(prog, "uDiffuseTex");
};

template<typename Vbo>
static gl::Vertex_array create_vao(Thresholding_shader& s, Vbo& vbo) {
    using T = typename Vbo::value_type;

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, norm)));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, uv)));
    gl::EnableVertexAttribArray(s.aTexCoords);
    gl::BindVertexArray();

    return vao;
}

// same as above, but for the lights
struct Thresholding_lightbox_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("bloom.vert"),
        gl::CompileFragmentShaderResource("lightbox.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    // note: aNormal from vert shader ignored
    // note: aTexCoords from vert shader ignored

    gl::Uniform_mat4 uModelMtx = gl::GetUniformLocation(prog, "uModelMtx");
    gl::Uniform_mat4 uViewMtx = gl::GetUniformLocation(prog, "uViewMtx");
    gl::Uniform_mat4 uProjMtx = gl::GetUniformLocation(prog, "uProjMtx");
    gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(prog, "uLightColor");
};

template<typename Vbo>
static gl::Vertex_array create_vao(Thresholding_lightbox_shader& s, Vbo& vbo) {
    using T = typename Vbo::value_type;

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::BindVertexArray();

    return vao;
}

struct Blur_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("blur.vert"),
        gl::CompileFragmentShaderResource("blur.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(1);

    gl::Uniform_sampler2d uImage = gl::GetUniformLocation(prog, "image");
    gl::Uniform_bool uHorizontal = gl::GetUniformLocation(prog, "horizontal");
};

template<typename Vbo>
static gl::Vertex_array create_vao(Blur_shader& s, Vbo& vbo) {
    using T = typename Vbo::value_type;

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, uv)));
    gl::EnableVertexAttribArray(s.aTexCoords);
    gl::BindVertexArray();

    return vao;
}

// shader that adds the blurred (bloom) texture to the HDR color texture
struct Bloom_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("bloom_final.vert"),
        gl::CompileFragmentShaderResource("bloom_final.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(1);

    gl::Uniform_sampler2d uSceneTex = gl::GetUniformLocation(prog, "scene");
    gl::Uniform_sampler2d uBlurTex = gl::GetUniformLocation(prog, "bloomBlur");
    gl::Uniform_bool uBloom = gl::GetUniformLocation(prog, "bloom");
    gl::Uniform_float uExposure = gl::GetUniformLocation(prog, "exposure");
};

template<typename Vbo>
static gl::Vertex_array create_vao(Bloom_shader& s, Vbo& vbo) {
    using T = typename Vbo::value_type;

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, uv)));
    gl::EnableVertexAttribArray(s.aTexCoords);
    gl::BindVertexArray();

    return vao;
}

struct Renderer final {
    gl::Texture_2d wood_tex =
        gl::load_tex(gfxplay::resource_path("textures", "wood.png"), gl::TexFlag_SRGB);

    gl::Texture_2d container_tex =
        gl::load_tex(gfxplay::resource_path("textures/container2.png"), gl::TexFlag_SRGB);

    static constexpr std::array<glm::vec3, 4> light_positions = {{
        { 0.0f, 0.5f,  1.5f},
        {-4.0f, 0.5f, -3.0f},
        { 3.0f, 0.5f,  1.0f},
        {-0.8f, 2.4f, -1.0f},
    }};

    static constexpr std::array<glm::vec3, 4> light_colors = {{
        {5.0f,   5.0f,  5.0f},
        {10.0f,  0.0f,  0.0f},
        {0.0f,   0.0f,  15.0f},
        {0.0f,   5.0f,  0.0f},
    }};

    // model matrix for floor
    glm::mat4 floor_cube_mmtx = []() {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(0.0f, -1.0f, 0.0));
        m = glm::scale(m, glm::vec3(12.5f, 0.5f, 12.5f));
        return m;
    }();

    // madel matrices for containers
    std::array<glm::mat4, 6> container_mmxs = {{
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(0.0f, 1.5f, 0.0));
            m = glm::scale(m, glm::vec3(0.5f));
            return m;
        }(),
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(2.0f, 0.0f, 1.0f));
            m = glm::scale(m, glm::vec3(0.5f));
            return m;
        }(),
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(-1.0f, -1.0f, 2.0));
            m = glm::rotate(m, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            return m;
        }(),
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(0.0f, 2.7f, 4.0));
            m = glm::rotate(m, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            m = glm::scale(m, glm::vec3(1.25));
            return m;
        }(),
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(-2.0f, 1.0f, -3.0));
            m = glm::rotate(m, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            return m;
        }(),
        []() {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, glm::vec3(-3.0f, 0.0f, 0.0));
            m = glm::scale(m, glm::vec3(0.5f));
            return m;
        }(),
    }};

    // returns fully-initialized HDR-ready texture that can be used by shaders
    // as a render target
    static gl::Texture_2d init_hdr_tex() {
        gl::Texture_2d t = gl::GenTexture2d();
        gl::BindTexture(t);
        gl::TexImage2D(t.type, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return t;
    }

    // first pass FBO: an multiple render target (MRT) FBO that writes
    // thresholded color values to a second color texture
    //
    // both color outputs written to a sample-able texture
    gl::Texture_2d hdr_color0_tex = init_hdr_tex();
    gl::Texture_2d hdr_color1_tex = init_hdr_tex();
    gl::Render_buffer depth_rbo = []() {
        gl::Render_buffer rbo = gl::GenRenderBuffer();
        gl::BindRenderBuffer(rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ui::window_width, ui::window_height);
        return rbo;
    }();
    gl::Frame_buffer hdr_mrt_fbo = [this]() {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdr_color0_tex, 0);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, hdr_color1_tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return fbo;
    }();

    // blur FBOs:
    static gl::Frame_buffer init_pingpong_fbo(gl::Texture_2d& tex) {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        // no need for depth buffer: this is effectively just blurring a 2D image
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return fbo;
    }
    gl::Texture_2d blur_ping_tex = init_hdr_tex();
    gl::Frame_buffer blur_ping_fbo = init_pingpong_fbo(blur_ping_tex);
    gl::Texture_2d blur_pong_tex = init_hdr_tex();
    gl::Frame_buffer blur_pong_fbo = init_pingpong_fbo(blur_pong_tex);

    // cube data
    gl::Sized_array_buffer<Shaded_textured_vert> cube_vbo{
        shaded_textured_cube_verts
    };

    // quad data
    gl::Sized_array_buffer<Shaded_textured_vert> debug_quad_vbo{
        shaded_textured_quad_verts
    };

    Thresholding_shader ts;
    gl::Vertex_array ts_cube_vao = create_vao(ts, cube_vbo);
    Thresholding_lightbox_shader tls;
    gl::Vertex_array tls_cube_vao = create_vao(tls, cube_vbo);
    Blur_shader bs;
    gl::Vertex_array bs_quad_vao = create_vao(bs, debug_quad_vbo);
    Bloom_shader bls;
    gl::Vertex_array bls_quad_vao = create_vao(bls, debug_quad_vbo);

    // debugging
    Plain_texture_shader debugq_shader;
    gl::Vertex_array debug_quad_vao = create_vao(debugq_shader, debug_quad_vbo);

    void draw(ui::Window_state&, ui::Game_state& s) {
        // step 1: draw scene into 2 textures:
        //
        // - hdr color (GL_COLOR_ATTACHMENT0) standard scene render /w HDR
        // - thresholded HDR color (GL_COLOR_ATTACHMENT1) only contains
        //   fragments in the scene that exceed some brightness threshold
        {
            gl::BindFrameBuffer(GL_FRAMEBUFFER, hdr_mrt_fbo);
            gl::UseProgram(ts.prog);
            gl::BindVertexArray(ts_cube_vao);

            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // invariant uniforms
            gl::Uniform(ts.uViewMtx, s.camera.view_mtx());
            gl::Uniform(ts.uProjMtx, s.camera.persp_mtx());
            gl::Uniform(ts.uLightPositions, light_positions.size(), light_positions.data());
            gl::Uniform(ts.uLightColors, light_colors.size(), light_colors.data());

            // draw floor
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wood_tex);
            gl::Uniform(ts.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(ts.uModelMtx, floor_cube_mmtx);
            gl::Uniform(ts.uNormalMtx, gl::normal_matrix(floor_cube_mmtx));
            gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());

            // draw containers
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(container_tex);
            gl::Uniform(ts.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());
            for (glm::mat4 const& m : container_mmxs) {
                gl::Uniform(ts.uModelMtx, m);
                gl::Uniform(ts.uNormalMtx, gl::normal_matrix(m));
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            }
            gl::BindVertexArray();

            // draw lights using the specialized lightbox shader
            gl::UseProgram(tls.prog);
            gl::BindVertexArray(tls_cube_vao);
            gl::Uniform(tls.uViewMtx, s.camera.view_mtx());
            gl::Uniform(tls.uProjMtx, s.camera.persp_mtx());
            for (size_t i = 0; i < light_positions.size(); ++i) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(light_positions[i]));
                model = glm::scale(model, glm::vec3(0.25f));
                gl::Uniform(tls.uModelMtx, model);
                gl::Uniform(tls.uLightColor, light_colors[i]);
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            }

            gl::BindVertexArray();
            gl::UseProgram();
            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        }

        // step2: blur the thresholded render
        {
            // implementation: two-pass Gaussian blur
            //
            // - achieved by "ping-pong"ing between two FBOs
            // - first pass (ping) blurs horizontally, second pass (pong) blurs
            //   vertically
            // - multiple "ping-pong"s increase blur amount (n * (ping + pong))

            gl::UseProgram(bs.prog);
            gl::BindVertexArray(bs_quad_vao);

            bool first = true;
            for (int i = 0; i < 2; ++i) {
                // ping
                gl::BindFrameBuffer(GL_FRAMEBUFFER, blur_ping_fbo);
                gl::Uniform(bs.uHorizontal, true);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(first ? hdr_color0_tex : blur_pong_tex);
                gl::Uniform(bs.uImage, gl::texture_index<GL_TEXTURE0>());
                gl::DrawArrays(GL_TRIANGLES, 0, debug_quad_vbo.sizei());

                // pong
                gl::BindFrameBuffer(GL_FRAMEBUFFER, blur_pong_fbo);
                gl::Uniform(bs.uHorizontal, false);
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(blur_ping_tex);
                gl::Uniform(bs.uImage, gl::texture_index<GL_TEXTURE0>());
                gl::DrawArrays(GL_TRIANGLES, 0, debug_quad_vbo.sizei());

                first = false;
            }
            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

            // assuming passes >0, blur_pong_tex now contains a blurred texture

            gl::BindVertexArray();
            gl::UseProgram();
        }

        // step 3: additively compose selectively-blurred render with "normal"
        //         render to produce "bloom"ed composition
        //
        // bloom is effectively a post-processing filter that selectively blurs
        // the bright parts of the frame.
        {
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gl::UseProgram(bls.prog);

            gl::Uniform(bls.uBloom, true);
            gl::Uniform(bls.uExposure, 0.1f);

            // normal scene HDR texture
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(hdr_color0_tex);
            gl::Uniform(bls.uSceneTex, gl::texture_index<GL_TEXTURE0>());

            // bloom HDR texture
            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(blur_pong_tex);
            gl::Uniform(bls.uBlurTex, gl::texture_index<GL_TEXTURE1>());

            gl::BindVertexArray(bls_quad_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, debug_quad_vbo.sizei());
            gl::BindVertexArray();

            gl::UseProgram();
        }
    }
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    // glEnable(GL_FRAMEBUFFER_SRGB); final frag shader does this for us

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
        }

        game.tick(dt);
        renderer.draw(sdl, game);
        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
