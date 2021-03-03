#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

struct Tangentspace_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

template<size_t N>
static std::array<Tangentspace_vert, N> compute_tangents_and_bitangents(std::array<Shaded_textured_vert, N> const& verts) {
    static_assert(N%3 == 0);  // i.e. it's an array of triangles

    std::array<Tangentspace_vert, N> rv;

    for (size_t i = 0; i < N; i +=3) {
        Shaded_textured_vert const& v1 = verts[i];
        Shaded_textured_vert const& v2 = verts[i+1];
        Shaded_textured_vert const& v3 = verts[i+2];

        glm::vec3 e1 = v2.pos - v1.pos;
        glm::vec3 e2 = v3.pos - v2.pos;
        glm::vec2 duv1 = v2.uv - v1.uv;
        glm::vec2 duv2 = v3.uv - v2.uv;

        // see: https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        float f = 1.0f / (duv1.x * duv2.y  -  duv2.x * duv1.y);

        glm::vec3 tangent{
            f * (duv2.y * e1.x  -  duv1.y * e2.x),
            f * (duv2.y * e1.y  -  duv1.y * e2.y),
            f * (duv2.y * e1.z  -  duv1.y * e2.z),
        };

        glm::vec3 bitangent{
            f * (-duv2.x * e1.x  +  duv1.x * e2.x),
            f * (-duv2.x * e1.y  +  duv1.x * e2.y),
            f * (-duv2.x * e1.z  +  duv1.x * e2.z),
        };

        rv[i] = Tangentspace_vert{v1.pos, v1.norm, v1.uv, tangent, bitangent};
        rv[i+1] = Tangentspace_vert{v2.pos, v2.norm, v2.uv, tangent, bitangent};
        rv[i+2] = Tangentspace_vert{v3.pos, v3.norm, v3.uv, tangent, bitangent};
    }

    return rv;
}

struct Parallax_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("parallax_shader.vert"),
        gl::CompileFragmentShaderResource("parallax_shader.frag"));

    static constexpr gl::Attribute_vec3 aPos = gl::Attribute_vec3::at_location(0);
    static constexpr gl::Attribute_vec3 aNormal = gl::Attribute_vec3::at_location(1);
    static constexpr gl::Attribute_vec2 aTexCoords = gl::Attribute_vec2::at_location(2);
    static constexpr gl::Attribute_vec3 aTangent = gl::Attribute_vec3::at_location(3);
    static constexpr gl::Attribute_vec3 aBitangent = gl::Attribute_vec3::at_location(4);

    gl::Uniform_mat4 uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat4 uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");
    gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(p, "viewPos");

    gl::Uniform_sampler2d uDiffuseTex = gl::GetUniformLocation(p, "DiffuseTex");
    gl::Uniform_sampler2d uNormalTex = gl::GetUniformLocation(p, "NormalTex");
    gl::Uniform_sampler2d uDepthTex = gl::GetUniformLocation(p, "DepthTex");
    gl::Uniform_float uHeightScale = gl::GetUniformLocation(p, "HeightScale");
};

template<typename T>
static gl::Vertex_array create_vao(Parallax_texture_shader& s, gl::Array_buffer<T>& vbo) {

    gl::Vertex_array vao;
    gl::BindVertexArray(vao);

    gl::BindBuffer(vbo);

    gl::VertexAttribPointer(s.aPos, false, sizeof(T), offsetof(T, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, false, sizeof(T), offsetof(T, norm));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoords, false, sizeof(T), offsetof(T, uv));
    gl::EnableVertexAttribArray(s.aTexCoords);
    gl::VertexAttribPointer(s.aTangent, false, sizeof(T), offsetof(T, tangent));
    gl::EnableVertexAttribArray(s.aTangent);
    gl::VertexAttribPointer(s.aBitangent, false, sizeof(T), offsetof(T, bitangent));
    gl::EnableVertexAttribArray(s.aBitangent);

    gl::BindVertexArray();

    return vao;
}

struct Renderer final {
    gl::Array_buffer<Tangentspace_vert> quad_vbo =
        compute_tangents_and_bitangents(shaded_textured_quad_verts);

    Parallax_texture_shader bs;
    gl::Vertex_array bs_quad_vao = create_vao(bs, quad_vbo);

    struct {
        gl::Texture_2d diffuse{
            gl::load_tex(gfxplay::resource_path("textures", "wood.png"), gl::TexFlag_SRGB)
        };
        gl::Texture_2d normals{
            gl::load_tex(gfxplay::resource_path("textures", "toy_box_normal.png"))
        };
        gl::Texture_2d depth{
            gl::load_tex(gfxplay::resource_path("textures", "toy_box_disp.png"))
        };
    } wood;

    struct {
        gl::Texture_2d diffuse{
            gl::load_tex(gfxplay::resource_path("textures" , "bricks2.jpg"), gl::TexFlag_SRGB)
        };
        gl::Texture_2d normals{
            gl::load_tex(gfxplay::resource_path("textures", "bricks2_normal.jpg"))
        };
        gl::Texture_2d depth{
            gl::load_tex(gfxplay::resource_path("textures", "bricks2_disp.jpg"))
        };
    } brick;

    bool use_wood = true;

    glm::vec3 light_pos{0.0f, 0.1f, 1.0f};

    glm::mat4 model_mtx = []() {
        glm::mat4 m = glm::identity<glm::mat4>();
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
        return m;
    }();

    static constexpr float height_scale = 0.05f;

    void tick(std::chrono::milliseconds cur) {
        auto x = static_cast<float>(cur.count()) / 1000.0f;
        light_pos = {sin(x), light_pos.y, cos(x)};
    }

    void draw(ui::Window_state& w, ui::Game_state& s) {
        glUseProgram(bs.p.get());

        gl::Uniform(bs.uModel, model_mtx);
        gl::Uniform(bs.uView, s.camera.view_mtx());
        gl::Uniform(bs.uProjection, s.camera.persp_mtx());
        gl::Uniform(bs.uNormalMatrix, gl::normal_matrix(model_mtx));

        gl::Texture_2d& diff = use_wood ? wood.diffuse : brick.diffuse;
        gl::Texture_2d& norms = use_wood ? wood.normals : brick.normals;
        gl::Texture_2d& depth = use_wood ? wood.depth : brick.depth;

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(diff);
        gl::Uniform(bs.uDiffuseTex, gl::texture_index<GL_TEXTURE0>());

        gl::ActiveTexture(GL_TEXTURE1);
        gl::BindTexture(norms);
        gl::Uniform(bs.uNormalTex, gl::texture_index<GL_TEXTURE1>());

        gl::ActiveTexture(GL_TEXTURE2);
        gl::BindTexture(depth);
        gl::Uniform(bs.uDepthTex, gl::texture_index<GL_TEXTURE2>());

        gl::Uniform(bs.uHeightScale, height_scale);
        gl::Uniform(bs.uLightPos, light_pos);
        gl::Uniform(bs.uViewPos, s.camera.pos);

        gl::BindVertexArray(bs_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
        gl::BindVertexArray();
    }
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    glEnable(GL_FRAMEBUFFER_SRGB);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
                renderer.use_wood = not renderer.use_wood;
            }
        }

        game.tick(dt);
        renderer.tick(cur_time);

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.draw(sdl, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
