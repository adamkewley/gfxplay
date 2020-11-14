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

    Mesh_tex load_texture(path p, Tex_type type) {
        // TODO: these should be set on a per-texture basis
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // TODO: the nonflipped API sucks
        return Mesh_tex{
            type,
            gl::nonflipped_and_mipmapped_texture(p.c_str())
        };
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

    std::shared_ptr<Mesh_tex> load_texture_cached(path p, Tex_type type) {
        static Caching_texture_loader ctl;
        return ctl.load(p, type);
    }

    Mesh load_mesh(path const& dir, aiScene const& scene, aiMesh const& mesh) {
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

            for (size_t i = 0, len = m.GetTextureCount(aiTextureType_AMBIENT); i < len; ++i) {
                aiString s;
                m.GetTexture(aiTextureType_AMBIENT, i, &s);

                path path_to_texture = dir / s.C_Str();
                textures.push_back(
                    load_texture_cached(std::move(path_to_texture), Tex_type::diffuse));
            }

            return textures;
        }();

        textures.shrink_to_fit();

        return Mesh{std::move(vbo), std::move(ebo), num_indices , std::move(textures)};
    }

    void process_node(path const& dir,
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

    Model load_model_(char const* path) {
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

    std::shared_ptr<Model> load_model_cached(char const* path) {
        static Caching_model_loader cml;
        return cml.load(path);
    }
}

