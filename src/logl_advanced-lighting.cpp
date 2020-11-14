#include "logl_common.hpp"
#include "logl_model.hpp"

struct Blinn_phong_program final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "blinn_phong.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "blinn_phong.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormals = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);
    gl::Uniform_mat4f uModel = gl::GetUniformLocation(p, "model");
    gl::Uniform_mat4f uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4f uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat3f uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");

    gl::Uniform_vec3f uViewPos = gl::GetUniformLocation(p, "viewPos");

    gl::Uniform_vec3f uDirLightDirection = gl::GetUniformLocation(p, "light.direction");
    gl::Uniform_vec3f uDirLightAmbient = gl::GetUniformLocation(p, "light.ambient");
    gl::Uniform_vec3f uDirLightDiffuse = gl::GetUniformLocation(p, "light.diffuse");
    gl::Uniform_vec3f uDirLightSpecular = gl::GetUniformLocation(p, "light.specular");

    static constexpr size_t maxDiffuseTextures = 4;
    gl::Uniform_1i uDiffuseTextures = gl::GetUniformLocation(p, "diffuseTextures");
    gl::Uniform_1i uActiveDiffuseTextures = gl::GetUniformLocation(p, "activeDiffuseTextures");
    static constexpr size_t maxSpecularTextures = 4;
    gl::Uniform_1i uSpecularTextures = gl::GetUniformLocation(p, "specularTextures");
    gl::Uniform_1i uActiveSpecularTextures = gl::GetUniformLocation(p, "activeSpecularTextures");
};

using model::Mesh_tex;
using model::Mesh_vert;
using model::Mesh;
using model::Model;
using model::Tex_type;

struct Compiled_mesh final {
    gl::Array_buffer vbo;
    gl::Element_array_buffer ebo;
    size_t num_indices;
    gl::Vertex_array vao;
    std::vector<std::shared_ptr<Mesh_tex>> textures;
};

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Extra GL setup
    auto prog = Blinn_phong_program{};

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

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
