#pragma once

#include "gl_extensions.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <vector>

namespace model {
    using std::filesystem::path;

    struct Mesh_vert final {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
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
        gl::Array_buffer<Mesh_vert> vbo;
        gl::Element_array_buffer<unsigned> ebo;
        size_t num_indices;
        std::vector<std::shared_ptr<Mesh_tex>> textures;
    };

    struct Model final {
        std::vector<Mesh> meshes;
    };

    static Mesh_tex load_texture(path p, Tex_type type) {
        gl::Tex_flags flgs = type == Tex_type::diffuse ?
            gl::Tex_flags::TexFlag_SRGB :
            gl::Tex_flags::TexFlag_None;

        Mesh_tex rv = Mesh_tex{type, gl::load_tex(p.c_str(), flgs)};

        glTextureParameteri(rv.handle.handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(rv.handle.handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(rv.handle.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(rv.handle.handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return rv;
    }

    struct Caching_texture_loader final {
        std::unordered_map<std::string, std::shared_ptr<Mesh_tex>> cache;
        std::mutex m;

        std::shared_ptr<Mesh_tex> load(path p, Tex_type type) {
            auto l = std::lock_guard(m);
            auto it = cache.find(p);

            if (it != cache.end()) {
                return it->second;
            }

            std::shared_ptr<Mesh_tex> t =
                std::make_shared<Mesh_tex>(load_texture(p, type));

            cache.emplace(std::move(p).string(), t);

            return t;
        }
    };

    static std::shared_ptr<Mesh_tex> load_texture_cached(path p, Tex_type type) {
        static Caching_texture_loader ctl;
        return ctl.load(p, type);
    }

    static Mesh load_mesh(path const& dir, aiScene const& scene, aiMesh const& mesh) {
        // load verts into an OpenGL VBO
        gl::Array_buffer<Mesh_vert> vbo = [&mesh]() {
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
                v.norm.x = n.x;
                v.norm.y = n.y;
                v.norm.z = n.z;

                if (has_tex_coords) {
                    v.uv.x = mesh.mTextureCoords[0][i].x;
                    v.uv.y = mesh.mTextureCoords[0][i].y;
                } else {
                    v.uv.x = 0.0f;
                    v.uv.y = 0.0f;
                }
            }

            return gl::Array_buffer<Mesh_vert>{dest_verts};
        }();

        // load indices into an OpenGL EBO
        size_t num_indices = 1337;
        gl::Element_array_buffer<unsigned> ebo = [&mesh, &num_indices]() {
            std::vector<unsigned> indices;

            for (size_t i = 0; i < mesh.mNumFaces; ++i) {
                aiFace const& f = mesh.mFaces[i];
                for (size_t j = 0; j < f.mNumIndices; ++j) {
                    indices.push_back(f.mIndices[j]);
                }
            }
            num_indices = indices.size();

            return gl::Element_array_buffer<unsigned>{indices};
        }();

        // load textures into a std::vector for later binding
        std::vector<std::shared_ptr<Mesh_tex>> textures = [&dir, &scene, &mesh]() {
            std::vector<std::shared_ptr<Mesh_tex>> rv;
            aiMaterial const& m = *scene.mMaterials[mesh.mMaterialIndex];

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_DIFFUSE); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_DIFFUSE, i, &s);

                path path_to_texture = dir / s.C_Str();
                rv.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::diffuse));
            }

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_SPECULAR); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_SPECULAR, i, &s);

                path path_to_texture = dir / s.C_Str();
                rv.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::specular));
            }

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_AMBIENT); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_AMBIENT, i, &s);

                path path_to_texture = dir / s.C_Str();
                rv.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::diffuse));
            }

            return rv;
        }();

        textures.shrink_to_fit();

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

    static Model load_model_(char const* path) {
        Assimp::Importer imp;
        aiScene const* scene =
            imp.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

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

    struct Caching_model_loader final {
        std::unordered_map<std::string, std::shared_ptr<Model>> cache;
        std::mutex m;

        std::shared_ptr<Model> load(path p) {
            auto l = std::lock_guard(m);
            auto it = cache.find(p);

            if (it != cache.end()) {
                return it->second;
            }

            std::shared_ptr<Model> t =
                std::make_shared<Model>(load_model_(p.c_str()));

            cache.emplace(std::move(p).string(), t);

            return t;
        }
    };

    static std::shared_ptr<Model> load_model_cached(char const* path) {
        static Caching_model_loader cml;
        return cml.load(path);
    }
}

