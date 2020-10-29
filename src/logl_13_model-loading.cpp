#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "logl_common.hpp"

#include <filesystem>
#include <unordered_map>
#include <mutex>

namespace {
    using std::filesystem::path;

    struct Mesh_vert final {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex_coords;
    };

    enum class Tex_type {
        diffuse,
        specular,
    };

    struct Mesh_tex final {
        Tex_type type;
        gl::Texture_2d handle;

        Mesh_tex(Tex_type _type, gl::Texture_2d _handle) :
            type{_type}, handle{std::move(_handle)} {
        }
    };

    struct Mesh final {
        gl::Array_buffer vbo;
        gl::Element_array_buffer ebo;
        size_t num_indices;
        std::vector<std::shared_ptr<Mesh_tex>> textures;
    };

    struct Model final {
        std::vector<Mesh> meshes;
    };

    static std::unique_ptr<Mesh_tex> load_tex(path p, Tex_type type) {
        // TODO: these should be set on a per-texture basis
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // TODO: the nonflipped API sucks
        return std::make_unique<Mesh_tex>(
            type,
            gl::nonflipped_and_mipmapped_texture(p.c_str()));
    }

    struct Caching_model_loader final {
        std::unordered_map<std::string, std::shared_ptr<Mesh_tex>> cache;

        std::shared_ptr<Mesh_tex> load(path p, Tex_type type) {
            auto it = cache.find(p);

            if (it != cache.end()) {
                return it->second;
            }

            std::shared_ptr<Mesh_tex> t = load_tex(p, type);

            cache.emplace(std::move(p).string(), t);

            return t;
        }
    };

    static std::shared_ptr<Mesh_tex> load_texture_cached(path p, Tex_type type) {
        // TODO: the texture loader probably shouldn't be a global like this,
        //       but cba'd refactoring right now
        static Caching_model_loader cml;
        static std::mutex m;

        auto l = std::lock_guard(m);
        return cml.load(p, type);
    }

    static Mesh load_mesh(path const& dir,
                          aiScene const& scene,
                          aiMesh const& mesh) {

        // load verts into an OpenGL VBO
        gl::Array_buffer vbo = [&mesh]() {
            bool has_tex_coords = mesh.mTextureCoords[0] != nullptr;

            std::vector<Mesh_vert> dest_verts;
            dest_verts.reserve(mesh.mNumVertices);

            for (size_t i = 0; i < mesh.mNumVertices; ++i) {
                Mesh_vert& v = dest_verts.emplace_back();

                aiVector3D const& p = mesh.mVertices[i];
                v.pos.x = p.x;
                v.pos.y = p.y;
                v.pos.z = p.z;

                aiVector3D const& n = mesh.mNormals[i];
                v.normal.x = n.x;
                v.normal.y = n.y;
                v.normal.z = n.z;

                if (has_tex_coords) {
                    v.tex_coords.x = mesh.mTextureCoords[0][i].x;
                    v.tex_coords.y = mesh.mTextureCoords[0][i].y;
                } else {
                    v.tex_coords.x = 0.0f;
                    v.tex_coords.y = 0.0f;
                }
            }

            gl::Array_buffer vbo = gl::GenArrayBuffer();
            gl::BindBuffer(vbo);
            gl::BufferData(vbo.type, dest_verts.size() * sizeof(Mesh_vert), dest_verts.data(), GL_STATIC_DRAW);

            return vbo;
        }();

        // load indices into an OpenGL EBO
        size_t num_indices = 1337;
        gl::Element_array_buffer ebo = [&mesh, &num_indices]() {
            std::vector<unsigned> indices;

            for (size_t i = 0; i < mesh.mNumFaces; ++i) {
                aiFace const& f = mesh.mFaces[i];
                for (size_t j = 0; j < f.mNumIndices; ++j) {
                    indices.push_back(f.mIndices[j]);
                }
            }
            num_indices = indices.size();

            gl::Element_array_buffer ebo = gl::GenElementArrayBuffer();
            gl::BindBuffer(ebo);
            gl::BufferData(ebo.type, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
            return ebo;
        }();

        // load textures into a std::vector for later binding
        std::vector<std::shared_ptr<Mesh_tex>> textures = [&dir, &scene, &mesh]() {
            std::vector<std::shared_ptr<Mesh_tex>> textures;
            aiMaterial const& m = *scene.mMaterials[mesh.mMaterialIndex];

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_DIFFUSE); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_DIFFUSE, i, &s);

                path path_to_texture = dir / s.C_Str();
                textures.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::diffuse));
            }

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_SPECULAR); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_SPECULAR, i, &s);

                path path_to_texture = dir / s.C_Str();
                textures.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::specular));
            }

            return textures;
        }();

        return Mesh{std::move(vbo), std::move(ebo), num_indices , std::move(textures)};
    }

    static void process_node(path const& dir,
                             aiScene const& scene,
                             aiNode const& node,
                             Model& out) {
        // process all meshes in `node`
        for (size_t i = 0; i < node.mNumMeshes; ++i) {
            out.meshes.push_back(load_mesh(dir, scene, *scene.mMeshes[node.mMeshes[i]]));
        }

        // recurse into all sub-nodes in `node`
        for (size_t i = 0; i < node.mNumChildren; ++i) {
            process_node(dir, scene, *node.mChildren[i], out);
        }
    }

    static Model load_model(char const* path) {
        Assimp::Importer imp;
        aiScene const* scene =
            imp.ReadFile(path, aiProcess_Triangulate);

        if (scene == nullptr
            or scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
            or scene->mRootNode == nullptr) {

            std::stringstream msg;
            msg << path << ": error: model load failed: " << imp.GetErrorString();
            throw std::runtime_error{std::move(msg).str()};
        }

        std::filesystem::path model_dir =
                std::filesystem::path{path}.parent_path();

        Model rv;
        process_node(model_dir, *scene, *scene->mRootNode, rv);
        return rv;
    }

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

    // A mesh that has been "compiled" against its target program (effectively,
    // its VAO has been initialized against the program)
    struct Compiled_mesh final {
        gl::Array_buffer vbo;
        gl::Element_array_buffer ebo;
        size_t num_indices;
        gl::Vertex_array vao;
        std::vector<std::shared_ptr<Mesh_tex>> textures;
    };

    static Compiled_mesh compile(Model_program& p, Mesh m) {
        Compiled_mesh rv = {
            std::move(m.vbo),
            std::move(m.ebo),
            m.num_indices,
            gl::GenVertexArrays(),
            std::move(m.textures),
        };

        gl::BindVertexArray(rv.vao);

        gl::BindBuffer(rv.ebo);

        // bind the vertices vbo
        gl::BindBuffer(rv.vbo);
        gl::VertexAttribPointer(p.aPos, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)0);
        gl::EnableVertexAttribArray(p.aPos);
        gl::VertexAttribPointer(p.aNormals, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)offsetof(Mesh_vert, normal));
        gl::EnableVertexAttribArray(p.aNormals);
        gl::VertexAttribPointer(p.aTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh_vert), (void*)offsetof(Mesh_vert, tex_coords));
        gl::EnableVertexAttribArray(p.aTexCoords);
        gl::BindVertexArray();

        return rv;
    }

    struct Compiled_model final {
        std::vector<Compiled_mesh> meshes;
    };

    static Compiled_model compile(Model_program &p, Model m) {
        Compiled_model rv;
        rv.meshes.reserve(m.meshes.size());

        for (Mesh& mesh : m.meshes) {
            rv.meshes.push_back(compile(p, std::move(mesh)));
        }

        return rv;
    }

    static void draw(Model_program& p,
                     Compiled_mesh& m,
                     ui::Game_state& gs) {
        gl::UseProgram(p.p);

        // assign textures
        {
            size_t active_diff_textures = 0;
            std::array<gl::Texture_2d const*, Model_program::maxDiffuseTextures> diff_handles;

            size_t active_spec__textures = 0;
            std::array<gl::Texture_2d const*, Model_program::maxSpecularTextures> spec_handles;

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
                std::array<GLint, Model_program::maxDiffuseTextures> tex_indices;
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
                std::array<GLint, Model_program::maxSpecularTextures> tex_indices;
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

        gl::BindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.num_indices, GL_UNSIGNED_INT, nullptr);
        gl::BindVertexArray();
    }

    static void draw(Model_program& p,
                     Compiled_model& m,
                     ui::Game_state& gs) {
        for (Compiled_mesh& cm : m.meshes) {
            draw(p, cm, gs);
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
    Model model = load_model(RESOURCES_DIR "backpack/backpack.obj");
    Compiled_model cmodel = compile(prog, std::move(model));

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
