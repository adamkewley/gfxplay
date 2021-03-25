#include "logl_common.hpp"
#include "ak_common-shaders.hpp"
#include "logl_model.hpp"

#include <array>
#include <random>

struct Ssao_geometry_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("ssao_geometry.vert"),
        gl::CompileFragmentShaderResource("ssao_geometry.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aNormal{1};
    static constexpr gl::Attribute_vec2 aTexCoords{2};

    gl::Uniform_bool uInvertedNormals{p, "invertedNormals"};
    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};

    template<typename Vbo, typename T = typename Vbo::value_type>
    [[nodiscard]] static gl::Vertex_array create_vao(Vbo& vbo, gl::Element_array_buffer<unsigned>* ebo = nullptr) noexcept {
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

struct Ssao_lighting_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("ssao_quad.vert"),
        gl::CompileFragmentShaderResource("ssao_lighting.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoords{1};

    gl::Uniform_sampler2d gPosition{p, "gPosition"};
    gl::Uniform_sampler2d gNormal{p, "gNormal"};
    gl::Uniform_sampler2d gAlbedo{p, "gAlbedo"};
    gl::Uniform_sampler2d ssao{p, "ssao"};

    gl::Uniform_vec3 light_Position{p, "light.Position"};
    gl::Uniform_vec3 light_Color{p, "light.Color"};
    gl::Uniform_float light_Linear{p, "light.Linear"};
    gl::Uniform_float light_Quadratic{p, "light.Quadratic"};
};

struct Ssao_ssao_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("ssao_quad.vert"),
        gl::CompileFragmentShaderResource("ssao_ssao.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoords{1};

    gl::Uniform_sampler2d gPosition{p, "gPosition"};
    gl::Uniform_sampler2d gNormal{p, "gNormal"};
    gl::Uniform_sampler2d gTexNoise{p, "texNoise"};
    gl::Uniform_array<gl::glsl::vec3, 64> samples{p, "samples"};
    gl::Uniform_mat4 projection{p, "projection"};

    template<typename Vbo, typename T = typename Vbo::value_type>
    [[nodiscard]] static gl::Vertex_array create_vao(Vbo& vbo) noexcept {
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

struct Ssao_blur_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("ssao_quad.vert"),
        gl::CompileFragmentShaderResource("ssao_blur.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoords{1};

    gl::Uniform_sampler2d ssaoInput{p, "ssaoInput"};
};

[[nodiscard]] static constexpr float lerp(float a, float b, float f) noexcept {
    return a + f * (b - a);
}

// generate sequence of vectors in tangent space that are between -1.0f and +1.0f in X and Y (or
// T and B) and between 0.0f and +1.0f in Z (or N)
static constexpr size_t kernel_size = 64;
static std::array<glm::vec3, kernel_size> generate_sample_kernel() {
    std::default_random_engine prng{std::random_device{}()};
    std::uniform_real_distribution<float> zero_to_one{0.0f, 1.0f};
    std::uniform_real_distribution<float> minus_one_to_plus_one{-1.0f, 1.0f};

    std::array<glm::vec3, kernel_size> rv;
    int i = 0;
    for (glm::vec3& vec : rv) {
        vec.x = minus_one_to_plus_one(prng);
        vec.y = minus_one_to_plus_one(prng);
        vec.z = zero_to_one(prng);

        vec = glm::normalize(vec);
        vec *= zero_to_one(prng);

        // scale samples so that they're more aligned to center of kernel
        float scale = static_cast<float>(i++) / static_cast<float>(rv.size());
        vec *= lerp(0.1f, 1.0f, scale * scale);
    }

    return rv;
}

// generate random direction vectors in tangent space that are between -1.0f and 1.0f in X and Y
// (T and B in tangent space) and 0 in Z (or N). These are used to rotate samples around the Z (or
// N) axis
[[nodiscard]] static gl::Texture_2d generate_noise_texture() {
    std::default_random_engine prng{std::random_device{}()};
    std::uniform_real_distribution<float> minus_one_to_plus_one{-1.0f, 1.0f};

    std::array<glm::vec3, 16> noise;
    for (glm::vec3& v : noise) {
        v.x = minus_one_to_plus_one(prng);
        v.y = minus_one_to_plus_one(prng);
        v.z = 0.0f;  // rotate around Z axis (in tangent space)
    }

    gl::Texture_2d rv;
    gl::BindTexture(rv);
    gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return rv;
}

struct State final {
    struct {
        Ssao_geometry_shader geom;
        Ssao_lighting_shader lighting;
        Ssao_ssao_shader ssao;
        Ssao_blur_shader blur;
        Plain_texture_shader pts;
    } shaders;

    struct {
        gl::Array_buffer<Shaded_textured_vert> vbo{shaded_textured_cube_verts};
        gl::Vertex_array geom_vao = Ssao_geometry_shader::create_vao(vbo);

        glm::mat4 room_model_mtx = []() {
            glm::mat4 rv = glm::mat4{1.0f};
            rv = glm::translate(rv, glm::vec3{0.0f, 7.0f, 0.0f});
            rv = glm::scale(rv, glm::vec3{7.5f, 7.5f, 7.5f});
            return rv;
        }();
    } cube;

    struct {
        std::shared_ptr<model::Model> model = model::load_model_cached(gfxplay::resource_path("backpack/backpack.obj").c_str());
        std::vector<gl::Vertex_array> geom_vaos = [&model = *this->model]() {
            std::vector<gl::Vertex_array> rv;
            rv.reserve(model.meshes.size());

            for (model::Mesh& mesh : model.meshes) {
                rv.push_back(Ssao_geometry_shader::create_vao(mesh.vbo, &mesh.ebo));
            }

            return rv;
        }();

        glm::mat4 model_mtx = []() {
            glm::mat4 rv = glm::mat4{1.0f};
            rv = glm::translate(rv, glm::vec3(0.0f, 0.5f, 0.0));
            rv = glm::rotate(rv, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
            rv = glm::scale(rv, glm::vec3(1.0f));
            return rv;
        }();
    } backpack;

    gl::Array_buffer<Shaded_textured_vert> quad_vbo{shaded_textured_quad_verts};
    gl::Vertex_array quad_pts_vao = create_vao(shaders.pts, quad_vbo);
    gl::Vertex_array quad_ssao_vao = Ssao_ssao_shader::create_vao(quad_vbo);

    gl::Texture_2d gPosition_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return t;
    }();

    gl::Texture_2d gNormal_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ui::window_width, ui::window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return t;
    }();

    gl::Texture_2d gAlbedo_tex = []() {
        gl::Texture_2d t;
        gl::BindTexture(t);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ui::window_width, ui::window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return t;
    }();

    gl::Render_buffer gDepth_rbo = []() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ui::window_width, ui::window_height);
        return rv;
    }();

    gl::Frame_buffer gbuffer_fbo = [this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gPosition_tex, 0);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gNormal_tex, 0);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gAlbedo_tex, 0);
        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gDepth_rbo);
        gl::assert_current_fbo_complete();
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }();

    gl::Texture_2d ssao_colorbuffer_tex = []() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RED, ui::window_width, ui::window_height, 0, GL_RED, GL_FLOAT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return rv;
    }();

    gl::Frame_buffer ssao_colorbuffer_fbo = [this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ssao_colorbuffer_tex, 0);
        gl::assert_current_fbo_complete();
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }();

    gl::Texture_2d ssao_blur_tex = []() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RED, ui::window_width, ui::window_height, 0, GL_RED, GL_FLOAT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return rv;
    }();

    gl::Frame_buffer ssao_blur_fbo = [this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ssao_blur_tex, 0);
        gl::assert_current_fbo_complete();
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }();

    std::array<glm::vec3, kernel_size> ssao_kernel = generate_sample_kernel();

    gl::Texture_2d noise_texture = generate_noise_texture();

    glm::vec3 light_pos = {2.0f, 4.0f, -2.0f};
    glm::vec3 light_color = {0.4f, 0.4f, 0.8f};
};

namespace tu {
    static constexpr GLenum gPosition = GL_TEXTURE0;
    static constexpr GLenum gNormal = GL_TEXTURE1;
    static constexpr GLenum gAlbedo = GL_TEXTURE2;
    static constexpr GLenum ssao = GL_TEXTURE3;
    static constexpr GLenum texNoise = GL_TEXTURE2;
    static constexpr GLenum ssaoInput = GL_TEXTURE0;
}

static void draw(State& st, ui::Window_state&, ui::Game_state& game) {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 persp_mtx = game.camera.persp_mtx();

    // 1. geometry pass: render cube + backpack into the gbuffers (pos, normals, depth, albedo)
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, st.gbuffer_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Ssao_geometry_shader& shader = st.shaders.geom;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uProjection, persp_mtx);
        gl::Uniform(shader.uView, game.camera.view_mtx());

        // render room cube
        {
            gl::Uniform(shader.uModel, st.cube.room_model_mtx);
            gl::Uniform(shader.uInvertedNormals, true);
            gl::BindVertexArray(st.cube.geom_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, st.cube.vbo.sizei());
            gl::BindVertexArray();
        }

        // render backpack
        {
            gl::Uniform(shader.uModel, st.backpack.model_mtx);
            gl::Uniform(shader.uInvertedNormals, false);

            assert(st.backpack.model->meshes.size() == st.backpack.geom_vaos.size());
            for (size_t i = 0; i < st.backpack.geom_vaos.size(); ++i) {
                auto& vao = st.backpack.geom_vaos[i];
                auto const& mesh = st.backpack.model->meshes[i];

                gl::BindVertexArray(vao);
                gl::DrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
                gl::BindVertexArray();
            }
        }

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    // 1. (debug): render output gbuffers to quads for inspection
    {
        Plain_texture_shader& shader = st.shaders.pts;

        gl::UseProgram(shader.p);
        gl::Uniform(shader.uView, gl::identity_val);
        gl::Uniform(shader.uProjection, gl::identity_val);
        gl::Uniform(shader.uSamplerMultiplier, gl::identity_val);

        // pos
        {
            glm::mat4 tl = glm::identity<glm::mat4>();
            tl = glm::translate(tl, glm::vec3{-0.75f, 0.75f, 0.0f});
            tl = glm::scale(tl, glm::vec3{0.25f});
            gl::Uniform(shader.uModel, tl);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(st.gPosition_tex);
            gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());
            gl::BindVertexArray(st.quad_pts_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
            gl::BindVertexArray();
        }

        // normals
        {
            glm::mat4 tl = glm::identity<glm::mat4>();
            tl = glm::translate(tl, glm::vec3{-0.75f, 0.25f, 0.0f});
            tl = glm::scale(tl, glm::vec3{0.25f});
            gl::Uniform(shader.uModel, tl);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(st.gNormal_tex);
            gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());
            gl::BindVertexArray(st.quad_pts_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
            gl::BindVertexArray();
        }

        // albedo
        {
            glm::mat4 tl = glm::identity<glm::mat4>();
            tl = glm::translate(tl, glm::vec3{-0.75f, -0.25f, 0.0f});
            tl = glm::scale(tl, glm::vec3{0.25f});
            gl::Uniform(shader.uModel, tl);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(st.gAlbedo_tex);
            gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());
            gl::BindVertexArray(st.quad_pts_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
            gl::BindVertexArray();
        }
    }

    // 2. SSAO: use the gbuffers compute ambient occlusion in screen space
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, st.ssao_colorbuffer_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT);

        Ssao_ssao_shader& shader = st.shaders.ssao;
        gl::UseProgram(shader.p);

        // bind gbuffer textures
        gl::ActiveTexture(tu::gPosition);
        gl::BindTexture(st.gPosition_tex);
        gl::ActiveTexture(tu::gNormal);
        gl::BindTexture(st.gNormal_tex);
        gl::ActiveTexture(tu::texNoise);
        gl::BindTexture(st.noise_texture);

        gl::Uniform(shader.samples, st.ssao_kernel);
        gl::Uniform(shader.projection, persp_mtx);

        gl::BindVertexArray(st.quad_ssao_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    // 2. (debug): show SSAO output in a debug quad
    {
        Plain_texture_shader& shader = st.shaders.pts;

        gl::UseProgram(shader.p);
        gl::Uniform(shader.uView, gl::identity_val);
        gl::Uniform(shader.uProjection, gl::identity_val);

        glm::mat4 tl = glm::identity<glm::mat4>();
        tl = glm::translate(tl, glm::vec3{-0.75f, -0.75f, 0.0f});
        tl = glm::scale(tl, glm::vec3{0.25f});
        gl::Uniform(shader.uModel, tl);

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(st.ssao_colorbuffer_tex);
        gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());

        glm::mat4 red2white = {
            1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        //  r     g     b     a  (column-major)
        };
        gl::Uniform(shader.uSamplerMultiplier, red2white);

        gl::BindVertexArray(st.quad_pts_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();
    }

    // 3. blur the SSAO texture
    {
        gl::BindFramebuffer(GL_FRAMEBUFFER, st.ssao_blur_fbo);
        gl::Clear(GL_COLOR_BUFFER_BIT);

        Ssao_blur_shader& shader = st.shaders.blur;
        gl::UseProgram(shader.p);
        gl::ActiveTexture(tu::ssaoInput);
        gl::BindTexture(st.ssao_colorbuffer_tex);

        // TODO: this VAO is technically a violation...
        gl::BindVertexArray(st.quad_ssao_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    // 3. (debug): show blur output in a debug quad
    {
        Plain_texture_shader& shader = st.shaders.pts;

        gl::UseProgram(shader.p);
        gl::Uniform(shader.uView, gl::identity_val);
        gl::Uniform(shader.uProjection, gl::identity_val);

        glm::mat4 tl = glm::identity<glm::mat4>();
        tl = glm::translate(tl, glm::vec3{-0.25f, +0.75f, 0.0f});
        tl = glm::scale(tl, glm::vec3{0.25f});
        gl::Uniform(shader.uModel, tl);

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(st.ssao_blur_tex);
        gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());

        glm::mat4 red2white = {
            1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        //  r     g     b     a  (column-major)
        };
        gl::Uniform(shader.uSamplerMultiplier, red2white);

        gl::BindVertexArray(st.quad_pts_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();
    }

    // 4. shading pass (combine the gbuffers, ssao, etc. into a Blinn-Phong shader)
    {
        Ssao_lighting_shader& shader = st.shaders.lighting;

        gl::UseProgram(shader.p);

        // set input uniforms
        gl::ActiveTexture(tu::gPosition);
        gl::BindTexture(st.gPosition_tex);
        gl::ActiveTexture(tu::gNormal);
        gl::BindTexture(st.gNormal_tex);
        gl::ActiveTexture(tu::gAlbedo);
        gl::BindTexture(st.gAlbedo_tex);
        gl::ActiveTexture(tu::ssao);
        gl::BindTexture(st.ssao_blur_tex);

        glm::vec3 lightPosView = glm::vec3(game.camera.view_mtx() * glm::vec4(st.light_pos, 1.0));
        gl::Uniform(shader.light_Position, lightPosView);
        gl::Uniform(shader.light_Color, st.light_color);
        gl::Uniform(shader.light_Linear, 0.09f);
        gl::Uniform(shader.light_Quadratic, 0.032f);

        gl::BindVertexArray(st.quad_ssao_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();
    }

    // DEV: full blit the ssao tex
    if (false) {
        Plain_texture_shader& shader = st.shaders.pts;

        gl::UseProgram(shader.p);
        gl::Uniform(shader.uView, gl::identity_val);
        gl::Uniform(shader.uProjection, gl::identity_val);
        gl::Uniform(shader.uModel, gl::identity_val);

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(st.ssao_blur_tex);
        gl::Uniform(shader.uTexture1, gl::texture_index<GL_TEXTURE0>());

        glm::mat4 red2white = {
            1.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        //  r     g     b     a  (column-major)
        };
        gl::Uniform(shader.uSamplerMultiplier, red2white);

        gl::BindVertexArray(st.quad_pts_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, st.quad_vbo.sizei());
        gl::BindVertexArray();
    }
}

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);

    // game loop
    State s;
    ui::Game_state game;
    util::Software_throttle throttle{8ms};
    std::chrono::milliseconds last_time = util::now();

    // shader config
    {
        Ssao_lighting_shader& shader = s.shaders.lighting;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.gPosition, gl::texture_index<tu::gPosition>());
        gl::Uniform(shader.gNormal, gl::texture_index<tu::gNormal>());
        gl::Uniform(shader.gAlbedo, gl::texture_index<tu::gAlbedo>());
        gl::Uniform(shader.ssao, gl::texture_index<tu::ssao>());
    }
    {
        Ssao_ssao_shader& shader = s.shaders.ssao;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.gPosition, gl::texture_index<tu::gPosition>());
        gl::Uniform(shader.gNormal, gl::texture_index<tu::gNormal>());
        gl::Uniform(shader.gTexNoise, gl::texture_index<tu::texNoise>());
    }
    {
        Ssao_blur_shader& shader = s.shaders.blur;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.ssaoInput, gl::texture_index<tu::ssaoInput>());
    }

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
        draw(s, sdl, game);
        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
