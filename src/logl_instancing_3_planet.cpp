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
        gl::CompileVertexShaderFile(RESOURCES_DIR "instanced_model_loading.vert"),
        gl::CompileFragmentShaderFile(RESOURCES_DIR "instanced_model_loading.frag"));

    static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
    static constexpr gl::Attribute aNormals = gl::AttributeAtLocation(1);
    static constexpr gl::Attribute aTexCoords = gl::AttributeAtLocation(2);
    static constexpr gl::Attribute aInstanceMatrix = gl::AttributeAtLocation(3);
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

// a mesh that is "compiled" against an Instanced_model_program
struct Compiled_mesh final {
    gl::Array_buffer vbo;

    gl::Element_array_buffer ebo;
    size_t num_indices;

    gl::Vertex_array vao;
    std::vector<std::shared_ptr<Mesh_tex>> textures;

    Sized_array_buffer<glm::mat4> ims;
};

// compile a mesh against the program, which sets up the VAOs etc.
static Compiled_mesh compile(
        Instanced_model_program& p,
        Mesh m,
        Sized_array_buffer<glm::mat4> ims) {

    Compiled_mesh rv = {
        std::move(m.vbo),

        std::move(m.ebo),
        m.num_indices,

        gl::GenVertexArrays(),
        std::move(m.textures),

        std::move(ims),
    };

    // now set up the VAO (the main purpose of "compilation")
    gl::BindVertexArray(rv.vao);

    gl::BindBuffer(rv.ebo);

    // set up vertex attribs
    gl::BindBuffer(rv.vbo);
    gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, pos)));
    gl::EnableVertexAttribArray(p.aPos);
    gl::VertexAttribPointer(p.aNormals, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, normal)));
    gl::EnableVertexAttribArray(p.aNormals);
    gl::VertexAttribPointer(p.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), reinterpret_cast<void*>(offsetof(Mesh_vert, tex_coords)));
    gl::EnableVertexAttribArray(p.aTexCoords);

    // set up instance attribs
    gl::BindBuffer(rv.ims);
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

    return rv;
}

// a model that is "compiled" against an Instanced_model_program
struct Compiled_model final {
    std::vector<Compiled_mesh> meshes;
};

// compile all meshes in the model against an Instanced_model_program
static Compiled_model compile(
    Instanced_model_program& p,
    Model m,
    Sized_array_buffer<glm::mat4> ims) {

    Compiled_model rv;
    rv.meshes.reserve(m.meshes.size());

    for (Mesh& mesh : m.meshes) {
        rv.meshes.push_back(compile(p, std::move(mesh), std::move(ims)));
    }

    return rv;
}

// draw a mesh
static void draw(Instanced_model_program& p,
                 Compiled_mesh& m,
                 ui::Game_state& gs) {
    gl::UseProgram(p.p);

    // assign textures
    {
        size_t active_diff_textures = 0;
        std::array<gl::Texture_2d const*, Instanced_model_program::maxDiffuseTextures> diff_handles;

        size_t active_spec__textures = 0;
        std::array<gl::Texture_2d const*, Instanced_model_program::maxSpecularTextures> spec_handles;

        // pass through texture list, gathering textures by type
        for (std::shared_ptr<Mesh_tex> const& tp : m.textures) {
            Mesh_tex const& t = *tp;

            if (t.type == Tex_type::diffuse) {
                if (active_diff_textures >= diff_handles.size()) {
                    static bool once = []() {
                        std::cerr << "WARNING: skipping assignment of diffuse texture: too many textures" << std::endl;
                        return true;
                    }();
                    continue;
                }

                diff_handles[active_diff_textures++] = &t.handle;
            } else if (t.type == Tex_type::specular) {
                if (active_spec__textures >= spec_handles.size()) {
                    static bool once = []() {
                        std::cerr << "WARNING: skipping assignment of specular texture" << std::endl;
                        return true;
                    }();

                    continue;  // skip assigning it (for now)
                }

                spec_handles[active_spec__textures++] = &t.handle;
            } else {
                throw std::runtime_error{"unhandled texture type encounted when drawing: this is probably because a new texture type has been added, but the drawing method has not been updated"};
            }
        }

        // bind the diffuse textures
        {
            // the uniform array contains pointers to the GL texture index
            // (e.g. GL_TEXTURE0). Must be contiguous for the assignment, so
            // swizzle them into an array and assign the uniform.
            std::array<GLint, Instanced_model_program::maxDiffuseTextures> tex_indices;
            for (size_t i = 0; i < active_diff_textures; ++i) {
                tex_indices[i] = i;
            }
            gl::Uniform(p.uDiffuseTextures, active_diff_textures, tex_indices.data());

            // and then bind the correct user-side texture handles to the correct
            // shader-side index
            for (size_t i = 0; i < active_diff_textures; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                gl::BindTexture(*diff_handles[i]);
            }

            gl::Uniform(p.uActiveDiffuseTextures, active_diff_textures);
        }

        // bind the specular textures:
        //     same story as diffuse textures (above), but they start
        //     *after* them so they must be offset by `active_diff_textures`
        {
            std::array<GLint, Instanced_model_program::maxSpecularTextures> tex_indices;
            for (size_t i = 0; i < active_spec__textures; ++i) {
                // this is offset by `active_diff_textures` because the
                // diff textures are already activated
                tex_indices[i] = active_diff_textures + i;
            }
            gl::Uniform(p.uSpecularTextures, active_spec__textures, tex_indices.data());

            for (size_t i = 0; i < active_spec__textures; ++i) {
                glActiveTexture(GL_TEXTURE0 + active_diff_textures + i);
                gl::BindTexture(*spec_handles[i]);
            }

            gl::Uniform(p.uActiveSpecularTextures, active_spec__textures);
        }
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

    gl::BindVertexArray(m.vao);
    glDrawElementsInstanced(GL_TRIANGLES, m.num_indices, GL_UNSIGNED_INT, nullptr, m.ims.size());
    gl::BindVertexArray();
}

// draw a compiled instance model
static void draw(Instanced_model_program& p,
                 Compiled_model& m,
                 ui::Game_state& gs) {
    for (Compiled_mesh& cm : m.meshes) {
        draw(p, cm, gs);
    }
}

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

    return compile(
        p,
        model::load_model(RESOURCES_DIR "rock/rock.obj"),
        Sized_array_buffer<glm::mat4>(roids.begin(), roids.end()));
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

    Compiled_model planet = compile(
        prog,
        model::load_model(RESOURCES_DIR "planet/planet.obj"),
        Sized_array_buffer<glm::mat4>{model});
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
