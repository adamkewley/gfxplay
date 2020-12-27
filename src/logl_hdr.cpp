#include "logl_common.hpp"
#include "ak_common-shaders.hpp"

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

struct Renderer final {
    Multilight_textured_shader bs;

    gl::Sized_array_buffer<Shaded_textured_vert> cube_vbo = []() {
        auto copy = shaded_textured_cube_verts;
        // invert normals: we're inside the cube
        for (Shaded_textured_vert& v : copy) {
            v.norm = -v.norm;
        }
        return gl::Sized_array_buffer<Shaded_textured_vert>{copy};
    }();

    gl::Vertex_array cube_vao = create_vao(bs, cube_vbo);

    gl::Texture_2d wood{
        gl::load_tex(RESOURCES_DIR "textures/wood.png", gl::TexFlag_SRGB),
    };

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
        {50.0f, 50.0f, 50.0f},
        {0.1f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.2f},
        {0.0f, 0.1f, 0.0f},
    }};

    void draw(ui::Window_state& w, ui::Game_state& s) {
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
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};

    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    glEnable(GL_FRAMEBUFFER_SRGB);

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

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.draw(sdl, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
