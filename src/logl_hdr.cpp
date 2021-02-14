#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

#include "runtime_config.hpp"

struct Multilight_textured_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("multilight.vert"),
        gl::CompileFragmentShaderResource("multilight.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);

    gl::Uniform_mat4 uModelMtx = gl::GetUniformLocation(prog, "uModelMtx");
    gl::Uniform_mat4 uViewMtx = gl::GetUniformLocation(prog, "uViewMtx");
    gl::Uniform_mat4 uProjMtx = gl::GetUniformLocation(prog, "uProjMtx");
    gl::Uniform_mat3 uNormalMtx = gl::GetUniformLocation(prog, "uNormalMtx");

    gl::Uniform_sampler2d uDiffuseTex = gl::GetUniformLocation(prog, "uDiffuseTex");
    gl::Uniform_vec3 uLightPositions = gl::GetUniformLocation(prog, "uLightPositions");
    gl::Uniform_vec3 uLightColors = gl::GetUniformLocation(prog, "uLightColors");
};

template<typename Vbo>
gl::Vertex_array create_vao(Multilight_textured_shader& s, Vbo& vbo) {
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

// shader that performs tone mapping on a HDR color texture
struct Hdr_shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("hdr.vert"),
        gl::CompileFragmentShaderResource("hdr.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(1);

    gl::Uniform_sampler2d hdrBuffer = gl::GetUniformLocation(prog, "hdrBuffer");
    gl::Uniform_bool hdr = gl::GetUniformLocation(prog, "hdr");
    gl::Uniform_float exposure = gl::GetUniformLocation(prog, "exposure");
};

template<typename Vbo>
gl::Vertex_array create_vao(Hdr_shader& s, Vbo& vbo) {
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
    gl::Sized_array_buffer<Shaded_textured_vert> cube_vbo = []() {
        auto copy = shaded_textured_cube_verts;
        // invert normals: we're inside the cube
        for (Shaded_textured_vert& v : copy) {
            v.norm = -v.norm;
        }
        return gl::Sized_array_buffer<Shaded_textured_vert>{copy};
    }();

    gl::Sized_array_buffer<Shaded_textured_vert> quad_vbo{
        shaded_textured_quad_verts
    };

    gl::Texture_2d wood{
        gl::load_tex(gfxplay::resource_path("textures", "wood.png"), gl::TexFlag_SRGB),
    };

    Multilight_textured_shader bs;
    gl::Vertex_array cube_vao = create_vao(bs, cube_vbo);
    Hdr_shader hs;
    gl::Vertex_array hs_quad_vao = create_vao(hs, quad_vbo);

    glm::mat4 tunnel_model_mtx = []() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
        model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
        return model;
    }();

    static constexpr std::array<glm::vec3, 4> light_positions = {{
        { 0.0f,  0.0f, 49.5f},
        {-1.4f, -1.9f, 9.0f},
        { 0.0f, -1.8f, 4.0f},
        { 0.8f, -1.7f, 6.0f},
    }};

    static constexpr std::array<glm::vec3, 4> light_colors = {{
        {200.0f, 200.0f, 200.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 2.0f},
        {0.0f, 1.0f, 0.0f},
    }};

    gl::Texture_2d hdr_colorbuf = []() {
        gl::Texture_2d t = gl::GenTexture2d();
        gl::BindTexture(t);
        gl::TexImage2D(t.type, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(t.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(t.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return t;
    }();

    gl::Render_buffer depth_rbo = []() {
        gl::Render_buffer rbo = gl::GenRenderBuffer();
        gl::BindRenderBuffer(rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ui::window_width, ui::window_height);
        return rbo;
    }();

    gl::Frame_buffer hdr_fbo = [this]() {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdr_colorbuf, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return fbo;
    }();

    bool use_hdr = true;
    float exposure = 1.0f;

    void draw(ui::Window_state& w, ui::Game_state& s) {
        gl::BindFrameBuffer(GL_FRAMEBUFFER, hdr_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            gl::UseProgram(bs.prog);

            gl::Uniform(bs.uModelMtx, tunnel_model_mtx);
            gl::Uniform(bs.uViewMtx, s.camera.view_mtx());
            gl::Uniform(bs.uProjMtx, s.camera.persp_mtx());
            gl::Uniform(bs.uNormalMtx, gl::normal_matrix(tunnel_model_mtx));

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wood);
            gl::Uniform(bs.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(bs.uLightPositions, light_positions.size(), light_positions.data());
            gl::Uniform(bs.uLightColors, light_colors.size(), light_colors.data());

            gl::BindVertexArray(cube_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            gl::BindVertexArray();
        }

        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            gl::UseProgram(hs.prog);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(hdr_colorbuf);
            gl::Uniform(hs.hdrBuffer, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(hs.hdr, use_hdr);
            gl::Uniform(hs.exposure, exposure);

            gl::BindVertexArray(hs_quad_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            gl::BindVertexArray();
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

            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_e) {
                renderer.exposure -= 0.005f;
            }

            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_q) {
                renderer.exposure += 0.005f;
            }
        }

        game.tick(dt);
        renderer.draw(sdl, game);
        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
