#include "logl_common.hpp"
#include "logl_model.hpp"

namespace {
    struct Model_program final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(RESOURCES_DIR "model_loading.vert"),
            gl::CompileFragmentShaderFile(RESOURCES_DIR "model_loading.frag"));

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

    gl::Vertex_array create_vao(Model_program& p, Mesh& m) {
        gl::Vertex_array vao = gl::GenVertexArrays();

        gl::BindVertexArray(vao);

        gl::BindBuffer(m.ebo);

        gl::BindBuffer(m.vbo);
        gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)0);
        gl::EnableVertexAttribArray(p.aPos);
        gl::VertexAttribPointer(p.aNormals, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)offsetof(Mesh_vert, normal));
        gl::EnableVertexAttribArray(p.aNormals);
        gl::VertexAttribPointer(p.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)offsetof(Mesh_vert, tex_coords));
        gl::EnableVertexAttribArray(p.aTexCoords);

        gl::BindVertexArray();

        return vao;
    }

    struct Compiled_model final {
        std::shared_ptr<Model> m;
        std::vector<gl::Vertex_array> vaos;

        Compiled_model(Model_program& p, std::shared_ptr<Model> _m) :
            m{std::move(_m)} {

            vaos.reserve(m->meshes.size());
            for (Mesh& m : m->meshes) {
                vaos.push_back(create_vao(p, m));
            }
        }
    };

    static void draw(Model_program& p,
                     Mesh& m,
                     gl::Vertex_array& vao,
                     ui::Game_state& gs) {
        gl::UseProgram(p.p);

        // assign textures
        {
            size_t active_diff_textures = 0;
            std::array<GLint, Model_program::maxDiffuseTextures> diff_indices;
            size_t active_spec_textures = 0;
            std::array<GLint, Model_program::maxDiffuseTextures> spec_indices;

            for (size_t i = 0; i < m.textures.size(); ++i) {
                Mesh_tex const& t = *m.textures[i];

                if (t.type == Tex_type::diffuse) {
                    if (active_diff_textures >= diff_indices.size()) {
                        throw std::runtime_error{"cannot assign diffuse texture: too many textures to assign"};
                    }
                    diff_indices[active_diff_textures++] = static_cast<GLint>(i);
                } else if (t.type == Tex_type::specular) {
                    if (active_spec_textures >= spec_indices.size()) {
                        throw std::runtime_error{"cannot assign spec texture: too many textures to assign"};
                    }
                    spec_indices[active_spec_textures++] = static_cast<GLint>(i);
                } else {
                    throw std::runtime_error{"unhandled texture type encounted when drawing: this is probably because a new texture type has been added, but the drawing method has not been updated"};
                }

                glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
                gl::BindTexture(t.handle);
            }

            gl::Uniform(p.uDiffuseTextures,
                        static_cast<GLsizei>(active_diff_textures),
                        diff_indices.data());
            gl::Uniform(p.uActiveDiffuseTextures, active_diff_textures);
            gl::Uniform(p.uSpecularTextures,
                        static_cast<GLsizei>(active_spec_textures),
                        spec_indices.data());
            gl::Uniform(p.uActiveSpecularTextures, active_spec_textures);
        }

        auto model_mat = glm::identity<glm::mat4>();
        gl::Uniform(p.uModel, model_mat);
        gl::Uniform(p.uView, gs.camera.view_mtx());
        gl::Uniform(p.uProjection, gs.camera.persp_mtx());
        gl::Uniform(p.uNormalMatrix, glm::transpose(glm::inverse(model_mat)));

        // light
        gl::Uniform(p.uDirLightDirection, glm::vec3{1.0f, 0.0f, 0.0f});
        gl::Uniform(p.uDirLightAmbient, glm::vec3{1.0f});
        gl::Uniform(p.uDirLightDiffuse, glm::vec3{1.0f});
        gl::Uniform(p.uDirLightSpecular, glm::vec3{1.0f});
        gl::Uniform(p.uViewPos, gs.camera.pos);

        gl::BindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, m.num_indices, GL_UNSIGNED_INT, nullptr);
        gl::BindVertexArray();
    }

    static void draw(Model_program& p,
                     Compiled_model& m,
                     ui::Game_state& gs) {        
        for (size_t i = 0; i < m.vaos.size(); ++i) {
            draw(p, m.m->meshes[i], m.vaos[i], gs);
        }
    }
}

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Extra GL setup
    auto prog = Model_program{};
    std::shared_ptr<Model> model = model::load_model_cached(RESOURCES_DIR "backpack/backpack.obj");
    Compiled_model cmodel{prog, std::move(model)};

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
        draw(prog, cmodel, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
