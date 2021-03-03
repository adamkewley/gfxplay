#include "logl_common.hpp"

struct Blinn_phong_program final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(gfxplay::resource_path("blinn_phong.vert")),
        gl::CompileFragmentShaderFile(gfxplay::resource_path("blinn_phong.frag")));

    // vertex shader attrs/uniforms
    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aNormals{1};
    static constexpr gl::Attribute_vec2 aTexCoords{2};
    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};
    gl::Uniform_mat3 uNormalMatrix{p, "normalMatrix"};

    // frag shader attrs/uniforms
    gl::Uniform_int uTexture1{p, "texture1"};
    gl::Uniform_vec3 uLightPos{p, "lightPos"};
    gl::Uniform_vec3 uViewPos{p, "viewPos"};
    gl::Uniform_int uBlinn{p, "blinn"};
};

struct Whole_app final {
    Blinn_phong_program prog;

    gl::Array_buffer<float> vbo = {
        // positions            // normals         // texcoords
         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };

    gl::Vertex_array vao = [this]() {
        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(prog.aPos, false, 8*sizeof(float), 0);
        gl::EnableVertexAttribArray(prog.aPos);
        gl::VertexAttribPointer(prog.aNormals, false, 8*sizeof(float), 3*sizeof(float));
        gl::EnableVertexAttribArray(prog.aNormals);
        gl::VertexAttribPointer(prog.aTexCoords, false, 8*sizeof(float), 6*sizeof(float));
        gl::EnableVertexAttribArray(prog.aTexCoords);
    };

    gl::Texture_2d floor =
        gl::load_tex(gfxplay::resource_path("textures/wood.png"));

    void draw(ui::Game_state const& s, bool blinn) {
        gl::UseProgram(prog.p);

        gl::Uniform(prog.uTexture1, 0);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(floor);

        auto model_mat = glm::identity<glm::mat4>();

        gl::Uniform(prog.uModel, model_mat);
        gl::Uniform(prog.uView, s.camera.view_mtx());
        gl::Uniform(prog.uProjection, s.camera.persp_mtx());
        gl::Uniform(prog.uNormalMatrix, glm::transpose(glm::inverse(model_mat)));

        glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
        gl::Uniform(prog.uLightPos, lightPos);
        gl::Uniform(prog.uViewPos, s.camera.pos);

        gl::Uniform(prog.uBlinn, blinn ? 1 : 0);

        gl::BindVertexArray(vao);
        gl::DrawArrays(GL_TRIANGLES, 0, 6);
        gl::BindVertexArray();
    }
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    auto app = Whole_app{};

    // Game state setup
    auto game = ui::Game_state{};

    // game loop
    auto throttle = util::Software_throttle{8ms};
    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    bool blinn = false;
    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_t) {
                blinn = not blinn;
            }

            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
        }

        game.tick(dt);

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        app.draw(game, blinn);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
