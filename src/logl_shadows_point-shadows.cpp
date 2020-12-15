#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

static constexpr GLsizei shadow_width = 1024;
static constexpr GLsizei shadow_height = 1024;
static constexpr float near_plane = 1.0f;
static constexpr float far_plane = 25.0f;

// Shader that populates a cubemap depthmap
struct Depthmap_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("point_shadows_depthmap.vert"),
        gl::CompileFragmentShaderResource("point_shadows_depthmap.frag"),
        gl::CompileGeometryShaderResource("point_shadows_depthmap.geom"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uShadowMatrices = gl::GetUniformLocation(p, "shadowMatrices");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_float uFar_plane = gl::GetUniformLocation(p, "far_plane");
};

gl::Vertex_array create_vao(Depthmap_shader& s, gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();
    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::BindBuffer();
    return vao;
}

struct Blinn_phong_cubemap_shadowmap final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("point_shadows.vert"),
        gl::CompileFragmentShaderResource("point_shadows.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(2);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat3 uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");
    gl::Uniform_sampler2d uDiffuseTexture = gl::GetUniformLocation(p, "diffuseTexture");
    gl::Uniform_samplerCube uDepthMap = gl::GetUniformLocation(p, "depthMap");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(p, "viewPos");
    gl::Uniform_float uFar_plane = gl::GetUniformLocation(p, "far_plane");
};

static gl::Vertex_array create_vao(
        Blinn_phong_cubemap_shadowmap& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo.data());
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, norm)));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, uv)));
    gl::EnableVertexAttribArray(s.aTexCoord);

    gl::BindVertexArray();

    return vao;
}

static std::array<glm::mat4, 6> generate_shadow_matrices(glm::vec3 light_pos) {
    float aspect_ratio = static_cast<float>(shadow_width)/static_cast<float>(shadow_height);

    glm::mat4 projection =
        glm::perspective(glm::radians(90.0f), aspect_ratio, near_plane, far_plane);

    return std::array<glm::mat4, 6>{{
        projection * glm::lookAt(light_pos, light_pos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    }};
}

struct Renderer final {
    // scene: skybox cube with some cubes dangling around inside
    glm::vec3 lightPos{0.0f, 0.0f, 0.0f};

    // world -> lightspace matrices for each face of the cubemap
    std::array<glm::mat4, 6> shadow_matrices = generate_shadow_matrices(lightPos);

    glm::mat4 skybox_model = []() {
        glm::mat4 m{1.0f};
        m = glm::scale(m, glm::vec3{5.0f});
        return m;
    }();

    glm::mat4 cube1 = []() {
        glm::mat4 m{1.0f};
        m = glm::translate(m, glm::vec3{4.0f, -3.5f, 0.0});
        m = glm::scale(m, glm::vec3{0.5f});
        return m;
    }();

    glm::mat4 cube2 = []() {
        glm::mat4 m{1.0f};
        m = glm::translate(m, glm::vec3{2.0f, 3.0f, 1.0});
        m = glm::scale(m, glm::vec3{0.75f});
        return m;
    }();

    glm::mat4 cube3 = []() {
        glm::mat4 m{1.0f};
        m = glm::translate(m, glm::vec3{-3.0f, -1.0f, 0.0});
        m = glm::scale(m, glm::vec3{0.5f});
        return m;
    }();

    glm::mat4 cube4 = []() {
        glm::mat4 m{1.0f};
        m = glm::translate(m, glm::vec3{-1.5f, 1.0f, 1.5f});
        m = glm::scale(m, glm::vec3{0.5f});
        return m;
    }();

    glm::mat4 cube5 = []() {
        glm::mat4 m{1.0f};
        m = glm::translate(m, glm::vec3{-1.5f, 2.0f, -3.0f});
        m = glm::rotate(m, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        m = glm::scale(m, glm::vec3{0.75f});
        return m;
    }();



    // standard quad
    gl::Sized_array_buffer<Shaded_textured_vert> quad_vbo{
        shaded_textured_quad_verts
    };

    gl::Sized_array_buffer<Shaded_textured_vert> cube_vbo{
        shaded_textured_cube_verts
    };

    // cube that has norms that point inwards, so it can be used as
    // a skybox
    gl::Sized_array_buffer<Shaded_textured_vert> inner_cube_vbo = []() {
        auto copy = shaded_textured_cube_verts;

        // invert norms because they should point inwards
        for (Shaded_textured_vert& v : copy) {
            v.norm = -v.norm;
        }

        return gl::Sized_array_buffer<Shaded_textured_vert>(copy);
    }();

    Depthmap_shader dm_shader;
    gl::Vertex_array dm_cube_vao = create_vao(dm_shader, cube_vbo);
    gl::Vertex_array dm_inner_cube_vao = create_vao(dm_shader, inner_cube_vbo);

    Blinn_phong_cubemap_shadowmap bp_shader;
    gl::Vertex_array pts_cube_vao = create_vao(bp_shader, cube_vbo);
    gl::Vertex_array pts_inner_cube_vao = create_vao(bp_shader, inner_cube_vbo);



    gl::Texture_2d wood_texture{
        gl::flipped_and_mipmapped_texture(RESOURCES_DIR "textures/wood.png", true)
    };

    gl::Texture_cubemap depth_cubemap = []() {
        gl::Texture_cubemap t = gl::GenTextureCubemap();
        gl::BindTexture(t);
        for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++face) {
            // allocate cubemap face data (undefined until populated)
            gl::TexImage2D(face, 0, GL_DEPTH_COMPONENT, shadow_width, shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        return t;
    }();

    gl::Frame_buffer depth_map_fbo = [&]() {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_cubemap.handle, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return fbo;
    }();

    void draw(ui::Window_state& ws, ui::Game_state& s) {
        auto cubes = {cube1, cube2, cube3, cube4, cube5};
        auto [sw, sh] = sdl::GetWindowSize(ws.window);

        // step 1: render scene from light's PoV to populate the depthmap
        {
            gl::Viewport(0, 0, shadow_width, shadow_height);
            gl::BindFrameBuffer(GL_FRAMEBUFFER, depth_map_fbo);
            gl::Clear(GL_DEPTH_BUFFER_BIT);

            gl::UseProgram(dm_shader.p);

            gl::Uniform(dm_shader.uLightPos, lightPos);
            gl::Uniform(dm_shader.uFar_plane, far_plane);
            gl::Uniform(dm_shader.uShadowMatrices, shadow_matrices.size(), shadow_matrices.data());

            // skybox
            gl::Uniform(dm_shader.uModel, skybox_model);
            gl::BindVertexArray(dm_inner_cube_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, inner_cube_vbo.sizei());
            gl::BindVertexArray();

            // other cubes
            gl::BindVertexArray(dm_cube_vao);
            for (glm::mat4 const& m : cubes) {
                gl::Uniform(dm_shader.uModel, m);
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            }
            gl::BindVertexArray();

            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
            gl::Viewport(0, 0, sw, sh);
        }

        // step 2: render the scene as normal
        {
            gl::UseProgram(bp_shader.p);

            gl::Uniform(bp_shader.uView, s.camera.view_mtx());
            gl::Uniform(bp_shader.uProjection, s.camera.persp_mtx());
            gl::Uniform(bp_shader.uLightPos, lightPos);
            gl::Uniform(bp_shader.uViewPos, s.camera.pos);
            gl::Uniform(bp_shader.uFar_plane, far_plane);

            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(wood_texture);
            gl::Uniform(bp_shader.uDiffuseTexture, gl::texture_index<GL_TEXTURE0>());

            gl::ActiveTexture(GL_TEXTURE1);
            gl::BindTexture(depth_cubemap);
            gl::Uniform(bp_shader.uDepthMap, gl::texture_index<GL_TEXTURE1>());

            // skybox
            gl::Uniform(bp_shader.uModel, skybox_model);
            gl::Uniform(bp_shader.uNormalMatrix, gl::normal_matrix(skybox_model));
            gl::BindVertexArray(pts_inner_cube_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, inner_cube_vbo.sizei());
            gl::BindVertexArray();

            // other cubes
            gl::BindVertexArray(pts_cube_vao);
            for (glm::mat4 const& m : cubes) {
                gl::Uniform(bp_shader.uModel, m);
                gl::Uniform(bp_shader.uNormalMatrix, gl::normal_matrix(m));
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
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
    glEnable(GL_FRAMEBUFFER_SRGB);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    Renderer renderer;

    // Game state setup
    auto game = ui::Game_state{};

    // game loop
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

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.draw(sdl, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
