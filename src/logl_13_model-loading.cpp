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
        path source_path;
        Tex_type type;
        gl::Texture_2d handle;

        Mesh_tex(path _p, Tex_type _t, gl::Texture_2d _h) :
            source_path{std::move(_p)}, type{_t}, handle{std::move(_h)} {
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

    static std::shared_ptr<Mesh_tex> load_texture(path p, Tex_type type) {
        static std::unordered_map<std::string, std::shared_ptr<Mesh_tex>> textures;
        static std::mutex m;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        auto lock = std::lock_guard{m};

        auto it = textures.find(p);
        if (it != textures.end()) {
            // texture previously loaded, just return a referenced-counted
            // handle to it
            return it->second;
        }

        auto tex = std::make_shared<Mesh_tex>(
            p,
            type,
            gl::nonflipped_and_mipmapped_texture(p.c_str()));

        textures.emplace(p.string(), tex);

        return tex;
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
                    load_texture(std::move(path_to_texture), Tex_type::diffuse));
            }

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_SPECULAR); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_SPECULAR, i, &s);

                path path_to_texture = dir / s.C_Str();
                textures.push_back(
                    load_texture(std::move(path_to_texture), Tex_type::specular));
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

        std::filesystem::path model_path = path;
        std::filesystem::path model_dir = model_path.parent_path();

        Model rv;
        process_node(model_dir, *scene, *scene->mRootNode, rv);
        return rv;
    }

    struct Model_program final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileVertexShaderFile(RESOURCES_DIR "model_loading.vert"),
            gl::CompileFragmentShaderFile(RESOURCES_DIR "model_loading.frag"));

        static constexpr gl::Attribute aPos = 0;
        static constexpr gl::Attribute aNormals = 1;
        static constexpr gl::Attribute aTexCoords = 2;
        gl::Uniform_mat4f uModel = gl::GetUniformLocation(p, "model");
        gl::Uniform_mat4f uView = gl::GetUniformLocation(p, "view");
        gl::Uniform_mat4f uProjection = gl::GetUniformLocation(p, "projection");
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

        assert(m.textures.size() > 0);

        glActiveTexture(GL_TEXTURE0);
        gl::BindTexture(m.textures[0]->handle);

        gl::Uniform(p.uModel, glm::identity<glm::mat4>());
        gl::Uniform(p.uView, gs.camera.view_mtx());
        gl::Uniform(p.uProjection, gs.camera.persp_mtx());

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

    // TODO: need to do L12_multiple-lights first
    return 0;
}
