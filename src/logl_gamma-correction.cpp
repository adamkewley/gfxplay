#include "logl_common.hpp"
#include "logl_model.hpp"

struct Blinn_phong_program final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "blinn_phong.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "blinn_phong.frag"));

    // vertex shader attrs/uniforms
    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormals = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);
    gl::Uniform_mat4f uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4f uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4f uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat3f uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");

    // frag shader attrs/uniforms
    gl::Uniform_1i uTexture1 = gl::GetUniformLocation(p, "texture1");
    gl::Uniform_vec3f uLightPos = gl::GetUniformLocation(p, "lightPos");
    gl::Uniform_vec3f uViewPos = gl::GetUniformLocation(p, "viewPos");
    gl::Uniform_1i uBlinn = gl::GetUniformLocation(p, "blinn");
};

struct Whole_app final {
    Blinn_phong_program prog;

    gl::Array_buffer vbo = []() {
        static float planeVertices[] = {
            // positions            // normals         // texcoords
             10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

             10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
             10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
        };

        gl::Array_buffer vbo = gl::GenArrayBuffer();
        gl::BindBuffer(vbo);
        gl::BufferData(vbo.type, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        return vbo;
    }();

    gl::Vertex_array vao = [this]() {
        gl::Vertex_array vao = gl::GenVertexArrays();

        gl::BindVertexArray(vao);

        gl::BindBuffer(vbo);
        gl::VertexAttribPointer(prog.aPos, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), reinterpret_cast<void*>(0));
        gl::EnableVertexAttribArray(prog.aPos);
        gl::VertexAttribPointer(prog.aNormals, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), reinterpret_cast<void*>(3*sizeof(float)));
        gl::EnableVertexAttribArray(prog.aNormals);
        gl::VertexAttribPointer(prog.aTexCoords, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), reinterpret_cast<void*>(6*sizeof(float)));
        gl::EnableVertexAttribArray(prog.aTexCoords);

        gl::BindVertexArray();

        return vao;
    }();

    gl::Texture_2d floor =
        gl::flipped_and_mipmapped_texture(RESOURCES_DIR "textures/wood.png", true);

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

    glEnable(GL_FRAMEBUFFER_SRGB);

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
