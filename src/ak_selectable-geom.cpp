#include "logl_common.hpp"

struct Shaded_textured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};
static_assert(sizeof(Shaded_textured_vert) == 8*sizeof(float));

struct Plain_vert final {
    glm::vec3 pos;
};
static_assert(sizeof(Plain_vert) == 3*sizeof(float));

struct Colored_vert final {
    glm::vec3 pos;
    glm::vec3 color;
};
static_assert(sizeof(Colored_vert) == 6*sizeof(float));

// shader that renders geometry with Blinn-Phong shading. Requires the geometry
// to have surface normals and textures
//
// only supports one light and one diffuse texture
struct Blinn_phong_textured_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("selectable.vert"),
        gl::CompileFragmentShaderResource("selectable.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat3 uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");

    gl::Uniform_sampler2d uTexture1 = gl::GetUniformLocation(p, "texture1");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(p, "viewPos");
};

static gl::Vertex_array create_vao(
        Blinn_phong_textured_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {
    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, norm)));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, uv)));
    gl::EnableVertexAttribArray(s.aTexCoords);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with basic texture mapping (no lighting etc.)
struct Plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("plain_texture_shader.vert"),
        gl::CompileFragmentShaderResource("plain_texture_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aTextureCoord = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");

    gl::Uniform_sampler2d uTexture1 = gl::GetUniformLocation(p, "texture1");
};

static gl::Vertex_array create_vao(
        Plain_texture_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, uv)));
    gl::EnableVertexAttribArray(s.aTextureCoord);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with a solid, uniform-defined, color
struct Uniform_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("uniform_color_shader.vert"),
        gl::CompileFragmentShaderResource("uniform_color_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");

    gl::Uniform_vec3 uColor = gl::GetUniformLocation(p, "color");
};

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Sized_array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Shaded_textured_vert), reinterpret_cast<void*>(offsetof(Shaded_textured_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);

    gl::BindVertexArray();

    return vao;
}

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Sized_array_buffer<Plain_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Plain_vert), reinterpret_cast<void*>(offsetof(Plain_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);

    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with an attribute-defined color
struct Attribute_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("attribute_color_shader.vert"),
        gl::CompileFragmentShaderResource("attribute_color_shader.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aColor = gl::AttributeAtLocation(1);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
};

static gl::Vertex_array create_vao(
        Attribute_color_shader& s,
        gl::Sized_array_buffer<Colored_vert>& vbo) {

    gl::Vertex_array vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Colored_vert), reinterpret_cast<void*>(offsetof(Colored_vert, pos)));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aColor, 3, GL_FLOAT, GL_FALSE, sizeof(Colored_vert), reinterpret_cast<void*>(offsetof(Colored_vert, color)));
    gl::EnableVertexAttribArray(s.aColor);

    gl::BindVertexArray();

    return vao;
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Shaded_textured_vert, 36> shaded_textured_cube_verts = {{
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

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Shaded_textured_vert, 6> shaded_textured_quad_verts = {{
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
}};

static constexpr std::array<Plain_vert, 6> plain_axes_verts = {{
    {{0.0f, 0.0f, 0.0f}}, // x origin
    {{1.0f, 0.0f, 0.0f}}, // x

    {{0.0f, 0.0f, 0.0f}}, // y origin
    {{0.0f, 1.0f, 0.0f}}, // y

    {{0.0f, 0.0f, 0.0f}}, // z origin
    {{0.0f, 0.0f, 1.0f}}  // z
}};

static constexpr std::array<Colored_vert, 6> colored_axes_verts = {{
    // x axis (red)
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

    // y axis (green)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

    // z axis (blue)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
}};

// data associated with a single instance of (in this case) a cube
struct Instance_data final {
    glm::mat4 model_mtx;
    glm::mat4 normal_mtx;

    Instance_data(glm::vec3 const& pos, float scale) :
        model_mtx{[&]() {
            glm::mat4 m = glm::identity<glm::mat4>();
            m = glm::translate(m, pos);
            m = glm::scale(m, glm::vec3{scale});
            return m;
        }()},
        normal_mtx{gl::normal_matrix(model_mtx)}
    {
    }

    Instance_data(glm::mat4 _model_mtx, glm::mat4 _normal_mtx) :
        model_mtx{std::move(_model_mtx)},
        normal_mtx{std::move(_normal_mtx)} {
    }
};

// hacky polar camera
struct Polar_camera final {
    float radius = 1.0;
    float theta = 0.0;
    float phi = 0.0;

    glm::vec3 pos() const {
        return {
            radius * sin(theta) * cos(phi),
            radius * sin(phi),
            radius * cos(theta) * cos(phi),
        };
    }

    glm::mat4 view_mtx() const {
        return glm::lookAt(pos(), glm::vec3{0.0}, {0.0,1.0,0.0});
    }

    glm::mat4 persp_mtx() const {
        return glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    }
};

struct Game_state_new final {
    // world space per millisecond
    static constexpr float movement_speed = 0.03f;
    static constexpr float mouse_sensitivity = 0.001f;

    Polar_camera camera;
    bool rotating = false;

    std::array<Instance_data, 3> cubes = {
        Instance_data{{0.0, 1.0, 0.0}, 0.5},
        Instance_data{{2.0, 0.0, 0.0}, 0.5},
        []() {
            glm::mat4 m = glm::identity<glm::mat4>();
            m = glm::translate(m, glm::vec3{-1.0f, 0.0, 0.0});
            m = glm::rotate(m, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            m = glm::scale(m, glm::vec3{0.25});
            return Instance_data{m, gl::normal_matrix(m)};
        }(),
    };

    int hovered_cube = -1;
    int selected_cube = -1;

    ui::Handle_response handle(ui::Window_state& window, SDL_Event& e) {
        if (e.type == SDL_QUIT) {
            return ui::Handle_response::should_quit;
        } else if (e.type == SDL_MOUSEBUTTONUP or e.type == SDL_MOUSEBUTTONDOWN) {
            bool is_down = e.type == SDL_MOUSEBUTTONDOWN;
            switch (e.button.button) {
            case SDL_BUTTON_MIDDLE:
                // middle mouse rotates scene
                rotating = is_down;
                break;
            case SDL_BUTTON_RIGHT:
                // right mouse (de)selects things
                if (is_down) {
                    selected_cube = hovered_cube;
                }
                break;
            }
        } else if (e.type == SDL_KEYDOWN or e.type == SDL_KEYUP) {
            bool is_button_down = e.type == SDL_KEYDOWN ? true : false;
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                return ui::Handle_response::should_quit;
            }
        } else if (e.type == SDL_MOUSEMOTION) {
            if (rotating) {
                static constexpr float sensitivity = 1.0f;
                auto [w, h] = sdl::GetWindowSize(window.window);
                float dx = -static_cast<float>(e.motion.xrel) / static_cast<float>(w);
                float dy = static_cast<float>(e.motion.yrel) / static_cast<float>(h);
                camera.theta += 2.0f * static_cast<float>(M_PI) * sensitivity * dx;
                camera.phi += 2.0f * static_cast<float>(M_PI) * sensitivity * dy;
            }
        } else if (e.type == SDL_MOUSEWHEEL) {
            static constexpr float wheel_sensitivity = 0.9f;
            if (e.wheel.y > 0 and camera.radius >= 0.1f) {
                camera.radius *= wheel_sensitivity;
            }

            if (e.wheel.y <= 0 and camera.radius < 100.0f) {
                camera.radius /= wheel_sensitivity;
            }
        }

        return ui::Handle_response::ok;
    }

    void tick(std::chrono::milliseconds const& dt) {
    }
};

struct Screen final {    
    // standard cube
    gl::Sized_array_buffer<Shaded_textured_vert> cube_vbo{
        shaded_textured_cube_verts
    };

    // standard quad
    gl::Sized_array_buffer<Shaded_textured_vert> quad_vbo{
        shaded_textured_quad_verts
    };

    // floor: standard quad with texture repeating 25x times
    gl::Sized_array_buffer<Shaded_textured_vert> floor_vbo = []() {
        auto quad_copy = shaded_textured_quad_verts;

        // repeating floor texture
        for (auto &p : quad_copy) {
            p.uv[0] *= 25.0;
            p.uv[1] *= 25.0;
        }

        return gl::Sized_array_buffer<Shaded_textured_vert>{quad_copy};
    }();

    gl::Sized_array_buffer<Colored_vert> axes_vbo{colored_axes_verts};

    Blinn_phong_textured_shader bps_shader;
    gl::Vertex_array bps_cube_vao = create_vao(bps_shader, cube_vbo);
    gl::Vertex_array bps_floor_vao = create_vao(bps_shader, floor_vbo);

    Plain_texture_shader pts_shader;
    gl::Vertex_array pts_quad_vao = create_vao(pts_shader, quad_vbo);

    Uniform_color_shader ucs_shader;
    gl::Vertex_array ucs_quad_vao = create_vao(ucs_shader, quad_vbo);
    gl::Vertex_array ucs_cube_vao = create_vao(ucs_shader, cube_vbo);

    Attribute_color_shader acs_shader;
    gl::Vertex_array acs_axes_vao = create_vao(acs_shader, axes_vbo);

    static constexpr glm::vec3 light_pos = {-2.0f, 1.0f, -1.0f};

    glm::mat4 floor_model_mtx = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, {0.0, -0.5, 0.0});
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
        m = glm::scale(m, glm::vec3{25.0f});
        return m;
    }();
    glm::mat4 floor_normal_mtx = gl::normal_matrix(floor_model_mtx);

    glm::mat4 debug_quad_model_mtx = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, glm::vec3{0.75f, 0.75f, -1.0f});
        m = glm::scale(m, glm::vec3{0.25});
        return m;
    }();

    glm::mat4 ucs_quad_mtx = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::translate(m, glm::vec3{0.75f, 0.25f, -1.0f});
        m = glm::scale(m, glm::vec3{0.25});
        return m;
    }();

    // DEBUGGING: render object ID texture to a quad so that it can be seen
    //            in realtime
    static constexpr GLsizei quad_width = 1024;
    static constexpr GLsizei quad_height = 768;
    gl::Texture_2d quad_texture = []() {
        gl::Texture_2d t = gl::GenTexture2d();
        gl::BindTexture(t);
        gl::TexImage2D(t.type, 0, GL_RGB, quad_width, quad_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(t, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(t, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        static constexpr float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTextureParameterfv(t, GL_TEXTURE_BORDER_COLOR, borderColor);
        gl::BindTexture();
        return t;
    }();
    gl::Render_buffer depthbuf = gl::GenRenderBuffer();
    gl::Frame_buffer quad_fbo = [&]() {
        gl::Frame_buffer fbo = gl::GenFrameBuffer();
        gl::BindFrameBuffer(GL_FRAMEBUFFER, fbo);

        // attach FBO color to texture
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, quad_texture.type, quad_texture, 0);

        gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

        return fbo;
    }();

    gl::Texture_2d wood_texture{
        gl::load_tex(RESOURCES_DIR "textures/wood.png", gl::TexFlag_SRGB)
    };

    void draw(ui::Window_state& w, Game_state_new& game) {
        glm::mat4 view_mtx = game.camera.view_mtx();
        glm::mat4 perspective_mtx = game.camera.persp_mtx();

        // step 1: figure out what's selected
        //
        // - draw the scene, coloring each object in a single, unique, color
        //   that encodes the object's ID
        //
        // - figure out where the mouse is w.r.t. the rendered image. Then use
        //   glReadPixels to get the color of the pixel under the mouse
        //
        // - decode that color back to an object ID. You now know what's
        //   selected
        {
            gl::UseProgram(ucs_shader.p);
            glDisable(GL_FRAMEBUFFER_SRGB);  // keep colors in linear space

            gl::Uniform(ucs_shader.uView, view_mtx);
            gl::Uniform(ucs_shader.uProjection, perspective_mtx);

            // draw, encoding object ID into color
            gl::BindVertexArray(ucs_cube_vao);
            for (size_t i = 0; i < game.cubes.size(); ++i) {
                Instance_data& cube = game.cubes[i];

                // encode ID into a non-black color
                uint32_t color_id = static_cast<uint32_t>(i + 1);
                float r = static_cast<float>(color_id & 0xff) / 255.0f;
                float g = static_cast<float>((color_id >> 8) & 0xff) / 255.0f;
                float b = static_cast<float>((color_id >> 16) & 0xff) / 255.0f;

                gl::Uniform(ucs_shader.uColor, r, g, b);
                gl::Uniform(ucs_shader.uModel, cube.model_mtx);

                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            }
            gl::BindVertexArray();


            // read pixel
            {
                // relative to top-left
                auto [xtl, ytl, st_unused] = sdl::GetMouseState();
                auto [width_unused, height] = sdl::GetWindowSize(w.window);

                GLsizei xbl = xtl;
                GLsizei ybl = height - ytl;

                GLubyte rgb[3]{};
                glReadPixels(xbl, ybl, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);

                // decode in the opposite way from how it was encoded above
                uint32_t decoded = rgb[0];
                decoded |= static_cast<uint32_t>(rgb[1]) << 8;
                decoded |= static_cast<uint32_t>(rgb[2]) << 16;

                if (decoded) {
                    --decoded;  // because we added 1 while drawing
                    if (decoded > game.cubes.size()) {
                        throw std::runtime_error{"decoded object ID is out of range?"};
                    }
                    game.hovered_cube = decoded;
                } else {
                    game.hovered_cube = -1;
                }
            }

            // DEBUG: blit the object ID render to a texture
            {
                gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, gl::window_fbo);
                gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, quad_fbo);
                auto [ww, wh] = sdl::GetWindowSize(w.window);
                gl::BlitFramebuffer(0, 0, ww, wh, 0, 0, quad_width, quad_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);
            }

            // clear the rendered data: it's served its purpose
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            glEnable(GL_FRAMEBUFFER_SRGB);
        }


        // step 2: render the scene
        //
        // - draw the scene as normal
        // - but keep in mind that we're using a stencil buffer
        {
            gl::UseProgram(bps_shader.p);

            gl::Uniform(bps_shader.uView, view_mtx);
            gl::Uniform(bps_shader.uProjection, perspective_mtx);
            {
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(wood_texture);
                gl::Uniform(bps_shader.uTexture1, gl::texture_index<GL_TEXTURE0>());
            }
            gl::Uniform(bps_shader.uLightPos, light_pos);
            gl::Uniform(bps_shader.uViewPos, game.camera.pos());

            // render floor
            gl::BindVertexArray(bps_floor_vao);
            gl::Uniform(bps_shader.uModel, floor_model_mtx);
            gl::Uniform(bps_shader.uNormalMatrix, floor_normal_mtx);
            gl::DrawArrays(GL_TRIANGLES, 0, floor_vbo.sizei());
            gl::BindVertexArray();

            // render cubes
            glClear(GL_STENCIL_BUFFER_BIT);
            gl::BindVertexArray(bps_cube_vao);
            for (auto& cube : game.cubes) {
                gl::Uniform(bps_shader.uModel, cube.model_mtx);
                gl::Uniform(bps_shader.uNormalMatrix, cube.normal_mtx);
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
            }
            gl::BindVertexArray();
        }

        // step 3: draw selection rims
        //
        // - we just did a "normal" render using the stencil buffer
        // - so if we enlarge the selected items a little and re-render them
        //   with the stencil buffer set up correctly we can add selection rims
        if (game.selected_cube != -1 or game.hovered_cube != -1){
            static constexpr glm::vec3 selected_rim_color{1.0f, 1.0f, 1.0f};
            static constexpr glm::vec3 hovered_rim_color{0.3f, 0.3f, 0.3f};

            gl::UseProgram(ucs_shader.p);

            glStencilFunc(GL_NOTEQUAL, 1, 0xff);
            glStencilMask(0x00);
            glDisable(GL_DEPTH_TEST);

            gl::Uniform(ucs_shader.uView, view_mtx);
            gl::Uniform(ucs_shader.uProjection, perspective_mtx);

            // draw selected cube (if applicable)
            if (game.selected_cube != -1) {
                Instance_data& cube = game.cubes.at(game.selected_cube);
                gl::Uniform(ucs_shader.uModel, glm::scale(cube.model_mtx, glm::vec3{1.05}));
                gl::Uniform(ucs_shader.uColor, selected_rim_color);
                gl::BindVertexArray(ucs_cube_vao);
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
                gl::BindVertexArray();
            }

            // draw hovered cube (if applicable)
            if (game.hovered_cube != -1 and
                game.hovered_cube != game.selected_cube) {

                Instance_data& cube = game.cubes.at(game.hovered_cube);
                gl::Uniform(ucs_shader.uModel, glm::scale(cube.model_mtx, glm::vec3{1.05}));
                gl::Uniform(ucs_shader.uColor, hovered_rim_color);
                gl::BindVertexArray(ucs_cube_vao);
                gl::DrawArrays(GL_TRIANGLES, 0, cube_vbo.sizei());
                gl::BindVertexArray();
            }

            glEnable(GL_DEPTH_TEST);
            glStencilMask(0xff);
            glStencilFunc(GL_ALWAYS, 1, 0xff);
        }

        // (optional2): draw a debug quad
        //
        // draws a quad on-screen that shows the depth map. Handy if the
        // shadows look like ass
        {
            glDisable(GL_DEPTH_TEST);

            gl::UseProgram(pts_shader.p);

            {
                gl::ActiveTexture(GL_TEXTURE0);
                gl::BindTexture(quad_texture);
                gl::Uniform(pts_shader.uTexture1, gl::texture_index<GL_TEXTURE0>());
            }
            gl::Uniform(pts_shader.uModel, debug_quad_model_mtx);
            gl::Uniform(pts_shader.uView, gl::identity_val);
            gl::Uniform(pts_shader.uProjection, gl::identity_val);
            gl::BindVertexArray(pts_quad_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
            gl::BindVertexArray();

            glEnable(GL_DEPTH_TEST);
        }

        // (optional 3): draw axes in bottom-right corner
        {
            glDisable(GL_DEPTH_TEST);

            gl::UseProgram(acs_shader.p);

            // the axes should be *rotated* the same way that the scene is
            // due to the camera location but shouldn't be translated
            glm::mat4 v = view_mtx;
            v[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);


            glm::mat4 m = glm::identity<glm::mat4>();
            m = glm::translate(m, glm::vec3{-0.9f, -0.9f, 0.0f});
            m = m * v;
            m = glm::scale(m, glm::vec3{0.1f});

            gl::Uniform(acs_shader.uModel, m);
            gl::Uniform(acs_shader.uView, gl::identity_val);
            gl::Uniform(acs_shader.uProjection, gl::identity_val);
            gl::BindVertexArray(acs_axes_vao);
            gl::DrawArrays(GL_LINES, 0, axes_vbo.sizei());
            gl::BindVertexArray();

            glEnable(GL_DEPTH_TEST);
        }
    }
};

int main(int, char**) {
    ui::Window_state window{};
    glEnable(GL_FRAMEBUFFER_SRGB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilMask(0xff);

    Game_state_new st{};
    Screen r{};
    util::Software_throttle throttle{8ms};

    auto t0 = util::now();
    while (true) {
        auto t1 = util::now();
        auto dt = t1 - t0;
        t0 = t1;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (st.handle(window, e) != ui::Handle_response::ok) {
                return 0;
            }
        }

        st.tick(dt);

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        r.draw(window, st);
        throttle.wait();

        SDL_GL_SwapWindow(window.window);
    }
}
