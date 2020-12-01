#include "logl_common.hpp"
#include "logl_model.hpp"

// thin abstraction over a write-only OpenGL array-buffer with known (by the
// CPU) size
template<typename T>
class Sized_array_buffer {
    gl::Array_buffer vbo = gl::GenArrayBuffer();
    size_t _size;

public:
    Sized_array_buffer(std::initializer_list<T> const& els) :
        Sized_array_buffer{els.begin(), els.end()} {
    }

    Sized_array_buffer(T const* begin, T const* end) :
        _size{static_cast<size_t>(std::distance(begin, end))} {

        gl::BindBuffer(vbo);
        gl::BufferData(vbo.type, static_cast<long>(_size * sizeof(T)), begin, GL_STATIC_DRAW);
    }

    operator gl::Array_buffer const& () const noexcept {
        return vbo;
    }

    operator gl::Array_buffer& () noexcept {
        return vbo;
    }

    size_t size() const noexcept {
        return _size;
    }
};

// A program that performs instanced rendering
//
// differences from normal rendering:
//
// - model matrices are passed in an attribute, rather than a uniform (so
//   multiple instances can be passed in one attribute
struct Instanced_model_program final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderFile(RESOURCES_DIR "instanced_model_loading.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "instanced_model_loading.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormals = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);
    static constexpr gl::Attribute aInstanceMatrix = gl::AttributeAtLocation(3);
    gl::Uniform_mat4 uView = gl::GetUniformLocation(p, "view");
    gl::Uniform_mat4 uProjection = gl::GetUniformLocation(p, "projection");
    gl::Uniform_mat3 uNormalMatrix = gl::GetUniformLocation(p, "normalMatrix");

    gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(p, "viewPos");

    gl::Uniform_vec3 uDirLightDirection = gl::GetUniformLocation(p, "light.direction");
    gl::Uniform_vec3 uDirLightAmbient = gl::GetUniformLocation(p, "light.ambient");
    gl::Uniform_vec3 uDirLightDiffuse = gl::GetUniformLocation(p, "light.diffuse");
    gl::Uniform_vec3 uDirLightSpecular = gl::GetUniformLocation(p, "light.specular");

    static constexpr size_t maxDiffuseTextures = 4;
    gl::Uniform_int uDiffuseTextures = gl::GetUniformLocation(p, "diffuseTextures");
    gl::Uniform_int uActiveDiffuseTextures = gl::GetUniformLocation(p, "activeDiffuseTextures");
    static constexpr size_t maxSpecularTextures = 4;
    gl::Uniform_int uSpecularTextures = gl::GetUniformLocation(p, "specularTextures");
    gl::Uniform_int uActiveSpecularTextures = gl::GetUniformLocation(p, "activeSpecularTextures");
};

using model::Mesh_tex;
using model::Mesh_vert;
using model::Mesh;
using model::Model;
using model::Tex_type;

gl::Vertex_array create_vao(Instanced_model_program& p,
                            Mesh& m,
                            Sized_array_buffer<glm::mat4>& ims) {
    auto vao = gl::GenVertexArrays();

    gl::BindVertexArray(vao);

    gl::BindBuffer(m.ebo);

    // set up vertex attribs
    gl::BindBuffer(m.vbo);
    gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, pos)));
    gl::EnableVertexAttribArray(p.aPos);
    gl::VertexAttribPointer(p.aNormals, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, normal)));
    gl::EnableVertexAttribArray(p.aNormals);
    gl::VertexAttribPointer(p.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, tex_coords)));
    gl::EnableVertexAttribArray(p.aTexCoords);

    // set up instance attribs
    gl::BindBuffer(ims);
    for (unsigned i = 0; i < 4; ++i) {
        // HACK: from LearnOpenGL: mat4's must be set in this way because
        //       of OpenGL not allowing more than 4 or so floats to be set
        //       in a single call
        //
        // see:  https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/10.3.asteroids_instanced/asteroids_instanced.cpp
        auto handle = p.aInstanceMatrix.handle + i;
        glVertexAttribPointer(
            handle,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(glm::mat4),
            reinterpret_cast<void*>(i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(handle);
        glVertexAttribDivisor(p.aInstanceMatrix.handle + i, 1);
    }

    gl::BindVertexArray();

    return vao;
}

struct Compiled_model final {
    std::shared_ptr<Model> model;
    Sized_array_buffer<glm::mat4> instance_matrices;
    std::vector<gl::Vertex_array> vaos;

    Compiled_model(Instanced_model_program& p,
                   std::shared_ptr<Model> m,
                   Sized_array_buffer<glm::mat4> ims) :
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
        model::load_model_cached(RESOURCES_DIR "rock/rock.obj"),
        Sized_array_buffer<glm::mat4>(roids.begin(), roids.end())
    };
}

// draw a mesh
static void draw(Instanced_model_program& p,
                 Mesh& m,
                 gl::Vertex_array& vao,
                 Sized_array_buffer<glm::mat4>& ims,
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
        model::load_model_cached(RESOURCES_DIR "planet/planet.obj"),
        Sized_array_buffer<glm::mat4>{model}
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
