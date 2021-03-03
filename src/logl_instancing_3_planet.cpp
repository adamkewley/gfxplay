#include "logl_common.hpp"
#include "logl_model.hpp"

// A program that performs instanced rendering
//
// differences from normal rendering:
//
// - model matrices are passed in an attribute, rather than a uniform (so
//   multiple instances can be passed in one attribute
struct Instanced_model_program final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(gfxplay::resource_path("instanced_model_loading.vert")),
        gl::CompileFragmentShaderFile(gfxplay::resource_path("instanced_model_loading.frag")));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aNormals{1};
    static constexpr gl::Attribute_vec2 aTexCoords{2};
    static constexpr gl::Attribute_mat4 aInstanceMatrix{3};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};
    gl::Uniform_mat3 uNormalMatrix{p, "normalMatrix"};

    gl::Uniform_vec3 uViewPos{p, "viewPos"};

    gl::Uniform_vec3 uDirLightDirection{p, "light.direction"};
    gl::Uniform_vec3 uDirLightAmbient{p, "light.ambient"};
    gl::Uniform_vec3 uDirLightDiffuse{p, "light.diffuse"};
    gl::Uniform_vec3 uDirLightSpecular{p, "light.specular"};

    static constexpr size_t maxDiffuseTextures = 4;
    gl::Uniform_int uDiffuseTextures{p, "diffuseTextures"};
    gl::Uniform_int uActiveDiffuseTextures{p, "activeDiffuseTextures"};
    static constexpr size_t maxSpecularTextures = 4;
    gl::Uniform_int uSpecularTextures{p, "specularTextures"};
    gl::Uniform_int uActiveSpecularTextures{p, "activeSpecularTextures"};
};

using model::Mesh_tex;
using model::Mesh_vert;
using model::Mesh;
using model::Model;
using model::Tex_type;

static gl::Vertex_array create_vao(Instanced_model_program& p,
                                   Mesh& m,
                                   gl::Array_buffer<glm::mat4>& ims) {
    gl::Vertex_array vao;

    gl::BindVertexArray(vao);

    gl::BindBuffer(m.ebo);

    // set up vertex attribs
    gl::BindBuffer(m.vbo);
    gl::VertexAttribPointer(p.aPos, false, sizeof(Mesh_vert), offsetof(Mesh_vert, pos));
    gl::EnableVertexAttribArray(p.aPos);
    gl::VertexAttribPointer(p.aNormals, false, sizeof(Mesh_vert), offsetof(Mesh_vert, norm));
    gl::EnableVertexAttribArray(p.aNormals);
    gl::VertexAttribPointer(p.aTexCoords, false, sizeof(Mesh_vert), offsetof(Mesh_vert, uv));
    gl::EnableVertexAttribArray(p.aTexCoords);

    // set up instance attribs
    gl::BindBuffer(ims);
    gl::VertexAttribPointer(p.aInstanceMatrix, false, sizeof(glm::mat4), 0);
    gl::EnableVertexAttribArray(p.aInstanceMatrix);
    gl::VertexAttribDivisor(p.aInstanceMatrix, 1);
    gl::BindVertexArray();

    return vao;
}

struct Compiled_model final {
    std::shared_ptr<Model> model;
    gl::Array_buffer<glm::mat4> instance_matrices;
    std::vector<gl::Vertex_array> vaos;

    Compiled_model(Instanced_model_program& p,
                   std::shared_ptr<Model> m,
                   gl::Array_buffer<glm::mat4> ims) :
        model{std::move(m)},
        instance_matrices{std::move(ims)} {

        vaos.reserve(model->meshes.size());
        for (Mesh& mesh : model->meshes) {
            vaos.push_back(create_vao(p, mesh, instance_matrices));
        }
    }
};

static Compiled_model load_asteroids(Instanced_model_program& p) {
    constexpr size_t num_roids = 100000;

    std::array<glm::mat4, num_roids> roids;

    float radius = 150.0;
    float offset = 25.0f;
    for (unsigned int i = 0; i < num_roids; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = static_cast<float>(i)/static_cast<float>(num_roids) * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        roids[i] = model;
    }

    return Compiled_model{
        p,
        model::load_model_cached(gfxplay::resource_path("rock/rock.obj").c_str()),
        gl::Array_buffer<glm::mat4>(roids)
    };
}

// draw a mesh
static void draw(Instanced_model_program& p,
                 Mesh& m,
                 gl::Vertex_array& vao,
                 gl::Array_buffer<glm::mat4>& ims,
                 ui::Game_state& gs) {
    gl::UseProgram(p.p);

    // assign textures
    {
        size_t active_diff_textures = 0;
        std::array<GLint, Instanced_model_program::maxDiffuseTextures> diff_indices;
        size_t active_spec_textures = 0;
        std::array<GLint, Instanced_model_program::maxDiffuseTextures> spec_indices;

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

    gl::Uniform(p.uView, gs.camera.view_mtx());
    gl::Uniform(p.uProjection, gs.camera.persp_mtx());

    // TODO: now invalid
    {
        auto model_mat = glm::identity<glm::mat4>();
        gl::Uniform(p.uNormalMatrix, glm::transpose(glm::inverse(model_mat)));
    }

    // light
    gl::Uniform(p.uDirLightDirection, glm::vec3{1.0f, 0.0f, 0.0f});
    gl::Uniform(p.uDirLightAmbient, glm::vec3{1.0f});
    gl::Uniform(p.uDirLightDiffuse, glm::vec3{1.0f});
    gl::Uniform(p.uDirLightSpecular, glm::vec3{1.0f});
    gl::Uniform(p.uViewPos, gs.camera.pos);

    gl::BindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLES, m.num_indices, GL_UNSIGNED_INT, nullptr, ims.size());
    gl::BindVertexArray();
}

// draw a compiled instance model
static void draw(Instanced_model_program& p,
                 Compiled_model& m,
                 ui::Game_state& gs) {

    std::vector<Mesh>& meshes = m.model->meshes;

    for (size_t i = 0; i < m.vaos.size(); ++i) {
        draw(p, meshes[i], m.vaos[i], m.instance_matrices, gs);
    }
}

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Extra GL setup
    auto prog = Instanced_model_program{};

    glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));

    Compiled_model planet{
        prog,
        model::load_model_cached(gfxplay::resource_path("planet/planet.obj").c_str()),
        gl::Array_buffer<glm::mat4>{model}
    };

    Compiled_model asteroids = load_asteroids(prog);

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
        draw(prog, planet, game);
        draw(prog, asteroids, game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
