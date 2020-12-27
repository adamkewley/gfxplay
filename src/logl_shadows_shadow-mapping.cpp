#include "logl_common.hpp"

// shader for calculating shadowmap's depthmap
struct Depthmap_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "shadows_shadow-maps_depth-maps.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "shadows_shadow-maps_depth-maps.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

    gl::Uniform_mat4 uLightSpaceMatrix = gl::GetUniformLocation(p, "lightSpaceMatrix");
    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
};

struct Shadowmap_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "shadows_shadow-maps.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "shadows_shadow-maps.frag"));
    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(2);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
    //gl::Uniform_mat3f uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");
    gl::Uniform_mat4 uLightSpaceMatrix = gl::GetUniformLocation(p, "lightSpaceMatrix");

    gl::Uniform_sampler2d uTexture = gl::GetUniformLocation(p, "diffuseTexture");
    gl::Uniform_sampler2d uShadowMap = gl::GetUniformLocation(p, "shadowMap");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(p, "viewPos");
};

// debugging: basic texture shader /w no lighting calcs
//
// used to sample the depthmap onto a quad that can be viewed in-ui
struct Basic_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "shadows_shadow-maps_basic-tex.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "shadows_shadow-maps_basic-tex.frag"));
    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

    gl::Uniform_sampler2d texture = gl::GetUniformLocation(p, "tex");
    //gl::Uniform_1f uNear_plane = gl::GetUniformLocation(p, "near_plane");
    //gl::Uniform_1f uFar_plane = gl::GetUniformLocation(p, "far_plane");
};

struct Mesh_el final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};
static_assert(sizeof(glm::vec2) == 2*sizeof(float));
static_assert(sizeof(Mesh_el) == 8*sizeof(float));

struct Plane final {
    static constexpr std::array<Mesh_el, 6> data = {{
        {{ 25.0f, -0.5f,  25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f,  0.0f}},
        {{-25.0f, -0.5f,  25.0f}, {0.0f, 1.0f, 0.0f}, { 0.0f,  0.0f}},
        {{-25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, { 0.0f, 25.0f}},

        {{ 25.0f, -0.5f,  25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f,  0.0f}},
        {{-25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, { 0.0f, 25.0f}},
        {{ 25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f, 25.0f}},
    }};

    gl::Array_buffer vbo = []() {
        gl::Array_buffer vbo = gl::GenArrayBuffer();
        gl::BindBuffer(vbo);
        gl::BufferData(vbo.type, sizeof(data), data.data(), GL_STATIC_DRAW);
        return vbo;
    }();
};

struct Cube final {
    static constexpr std::array<Mesh_el, 36> data = {{
        // back face
        {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
        {{ 1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
        {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{-1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}}, // top-left
        // front face
        {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{ 1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
        {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
        {{-1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}, // top-left
        {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
        // left face
        {{-1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
        {{-1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
        {{-1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
        {{-1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
        {{-1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
        {{-1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
        // right face
        {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
        {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
        {{ 1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
        {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
        {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
        {{ 1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-left
        // bottom face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
        {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
        // top face
        {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
        {{ 1.0f,  1.0f , 1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
        {{ 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
        {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
        {{-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}  // bottom-left
    }};

    gl::Array_buffer vbo = []() {
        gl::Array_buffer vbo = gl::GenArrayBuffer();
        gl::BindBuffer(vbo);
        gl::BufferData(vbo.type, sizeof(data), data.data(), GL_STATIC_DRAW);
        return vbo;
    }();
};

gl::Vertex_array create_vao(Shadowmap_shader& s, gl::Array_buffer& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_el), reinterpret_cast<void*>(offsetof(Mesh_el, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_el), reinterpret_cast<void*>(offsetof(Mesh_el, norm)));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_el), reinterpret_cast<void*>(offsetof(Mesh_el, uv)));
    gl::EnableVertexAttribArray(s.aTexCoord);

    gl::BindVertexArray();

    return vao;
}

gl::Vertex_array create_vao(Depthmap_shader& s, gl::Array_buffer& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_el), reinterpret_cast<void*>(offsetof(Mesh_el, pos)));
    gl::EnableVertexAttribArray(s.aPos);

    gl::BindVertexArray();

    return vao;
}

struct App final {
    Shadowmap_shader shader;

    Plane plane;
    gl::Vertex_array plane_vao = create_vao(shader, plane.vbo);

    Cube cube;
    gl::Vertex_array cube_vao = create_vao(shader, cube.vbo);

    gl::Texture_2d wood =
        gl::load_tex(RESOURCES_DIR "textures/wood.png");

    glm::mat4 cube1 = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, glm::vec3{0.0, 1.5, 0.0});
        m = glm::scale(m, glm::vec3{0.5});
        return m;
    }();

    glm::mat4 cube2 = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, glm::vec3{2.0f, 0.0, 0.0});
        m = glm::scale(m, glm::vec3{0.5});
        return m;
    }();

    glm::mat4 cube3 = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, glm::vec3{-1.0f, 0.0, 0.0});
        m = glm::rotate(m, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        m = glm::scale(m, glm::vec3{0.25});
        return m;
    }();

    static constexpr glm::vec3 light_pos = {-2.0f, 4.0f, -1.0f};

    Basic_texture_shader quad_shader;

    // debug quad in NDC
    struct Quad_el final {
        glm::vec3 pos;
        glm::vec2 uv;
    };
    static constexpr std::array<Quad_el, 6> quad_data = {{
        {{0.6, 0.6, -1.0}, {0.0, 0.0}},  // bl
        {{1.0, 1.0, -1.0}, {1.0, 1.0}},  // tr
        {{1.0, 0.6, -1.0}, {1.0, 0.0}},  // br

        {{1.0, 1.0, -1.0}, {1.0, 1.0}},  // tr
        {{0.6, 0.6, -1.0}, {0.0, 0.0}},  // bl
        {{0.6, 1.0, -1.0}, {0.0, 1.0}},  // tl
    }};

    // quad for debug vbo
    gl::Array_buffer quad_vbo = []() {
        gl::Array_buffer vbo = gl::GenArrayBuffer();
        gl::BindBuffer(vbo);
        gl::BufferData(vbo.type, sizeof(quad_data), quad_data.data(), GL_STATIC_DRAW);
        return vbo;
    }();

    gl::Vertex_array quad_vao = [&]() {
        gl::Vertex_array vao = gl::GenVertexArrays();
        gl::BindVertexArray(vao);
        gl::BindBuffer(quad_vbo);
        gl::VertexAttribPointer(quad_shader.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Quad_el), reinterpret_cast<void*>(offsetof(Quad_el, pos)));
        gl::EnableVertexAttribArray(quad_shader.aPos);
        gl::VertexAttribPointer(quad_shader.aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Quad_el), reinterpret_cast<void*>(offsetof(Quad_el, uv)));
        gl::EnableVertexAttribArray(quad_shader.aTexCoord);
        gl::BindVertexArray();
        return vao;
    }();

    static constexpr GLsizei shadow_width = 1024;
    static constexpr GLsizei shadow_height = 1024;

    gl::Texture_2d depth_map = []() {
        gl::Texture_2d t = gl::GenTexture2d();

        gl::BindTexture(t);
        glTexImage2D(
            t.type,
            0,
            GL_DEPTH_COMPONENT,
            shadow_width,
            shadow_height,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        static constexpr float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        return t;
    }();

    // an FBO that renders to the depth map
    gl::Frame_buffer depth_fbo = [&]() {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return fbo;
    }();

    Depthmap_shader dm_shader;
    gl::Vertex_array dm_plane_vao = create_vao(dm_shader, plane.vbo);
    gl::Vertex_array dm_cube_vao = create_vao(dm_shader, cube.vbo);

    void draw(ui::Window_state& w, ui::Game_state& game) {
        // lsm: maps world --> light perspective for depth mapping
        glm::mat4 lightSpaceMatrix = []() {
            float near_plane = 1.0f;
            float far_plane = 7.5;

            glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
            glm::mat4 lightView = glm::lookAt(light_pos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
            return lightProjection * lightView;
        }();

        // pass 1: depth mapping: create a depth map from light's perspective
        //
        // This populates a texture with depth values. Later passes can then
        // use this depthmap to test whether a fragment (being rendered) was
        // viewable from the light. If it wasn't, then the fragment must be
        // in shadow.
        {
            gl::UseProgram(dm_shader.p);

            gl::Viewport(0, 0, shadow_width, shadow_height);
            gl::BindFrameBuffer(GL_FRAMEBUFFER, depth_fbo);
            gl::Clear(GL_DEPTH_BUFFER_BIT);

            gl::Uniform(dm_shader.uLightSpaceMatrix, lightSpaceMatrix);

            // draw plane
            gl::BindVertexArray(dm_plane_vao);
            gl::Uniform(dm_shader.uModel, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, plane.data.size());
            gl::BindVertexArray();

            // draw cubes
            glCullFace(GL_FRONT);  // peter-panning: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
            gl::BindVertexArray(dm_cube_vao);
            gl::Uniform(dm_shader.uModel, cube1);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::Uniform(dm_shader.uModel, cube2);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::Uniform(dm_shader.uModel, cube3);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::BindVertexArray();
            glCullFace(GL_BACK);

            gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
            auto [width, height] = sdl::GetWindowSize(w.window);
            gl::Viewport(0, 0, width, height);
        }

        // pass 2: normal rendering: draw scene from camera's perspective
        //
        // this uses the depth map (created above) to figure out if a rendered
        // fragment should be in shadow or not
        {
            gl::UseProgram(shader.p);

            gl::Uniform(shader.uView, game.camera.view_mtx());
            gl::Uniform(shader.uProjection, game.camera.persp_mtx());
            gl::Uniform(shader.uLightSpaceMatrix, lightSpaceMatrix);
            {
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(wood);
                gl::Uniform(shader.uTexture, 0);
            }
            gl::Uniform(shader.uLightPos, light_pos);
            gl::Uniform(shader.uViewPos, game.camera.pos);
            {
                gl::ActiveTexture(GL_TEXTURE1);
                gl::BindTexture(depth_map);
                gl::Uniform(shader.uShadowMap, 1);
            }

            // draw plane
            gl::BindVertexArray(plane_vao);
            gl::Uniform(shader.uModel, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, plane.data.size());
            gl::BindVertexArray();

            // draw cubes
            gl::BindVertexArray(cube_vao);
            gl::Uniform(shader.uModel, cube1);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::Uniform(shader.uModel, cube2);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::Uniform(shader.uModel, cube3);
            gl::DrawArrays(GL_TRIANGLES, 0, cube.data.size());
            gl::BindVertexArray();
        }

        // (optional): draw a debug quad
        //
        // draws a quad on-screen that shows the depth map. Handy if the
        // shadows look like ass
        {

            gl::UseProgram(quad_shader.p);

            {
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(depth_map);
                gl::Uniform(quad_shader.texture, 0);
            }
            gl::BindVertexArray(quad_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, quad_data.size());
            gl::BindVertexArray();
        }
    }
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    auto app = App{};
    auto s = Depthmap_shader{};
    auto s2 = Basic_texture_shader{};

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
        app.draw(sdl, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
