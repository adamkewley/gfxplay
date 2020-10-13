#pragma once

#include "gl.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stbi.hpp"
#include <fstream>

namespace gl {
    // These structs act as type-safe wrappers around the various GL shader
    // types, so that it's harder for downstream applications to mix them up.

    struct Vertex_shader final {
        Shader_handle handle = CreateShader(GL_VERTEX_SHADER);
    };

    struct Fragment_shader final {
        Shader_handle handle = CreateShader(GL_FRAGMENT_SHADER);
    };

    struct Geometry_shader final {
        Shader_handle handle = CreateShader(GL_GEOMETRY_SHADER);
    };

    Vertex_shader CompileVertexShader(char const* src) {
        auto s = Vertex_shader{};
        ShaderSource(s.handle, src);
        CompileShader(s.handle);
        return s;
    }

    Fragment_shader CompileFragmentShader(char const* src) {
        auto s = Fragment_shader{};
        ShaderSource(s.handle, src);
        CompileShader(s.handle);
        return s;
    }

    Geometry_shader CompileGeometryShader(char const* src) {
        auto s = Geometry_shader{};
        ShaderSource(s.handle, src);
        CompileShader(s.handle);
        return s;
    }

    // convenience helper
    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs) {
        auto p = CreateProgram();
        AttachShader(p, vs.handle);
        AttachShader(p, fs.handle);
        LinkProgram(p);
        return p;
    }

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs,
                              Geometry_shader const& gs) {
        auto p = CreateProgram();
        AttachShader(p, vs.handle);
        AttachShader(p, gs.handle);
        AttachShader(p, fs.handle);
        LinkProgram(p);
        return p;
    }

    std::string slurp_file(const char* path) {
        std::ifstream f;
        f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        f.open(path, std::ios::binary | std::ios::in);

        std::stringstream ss;
        ss << f.rdbuf();

        return ss.str();
    }

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* func) {
        static auto to_string = [](GLubyte const* err_string) {
            return std::string{reinterpret_cast<char const*>(err_string)};
        };

        GLenum err = glGetError();
        if (err == GL_NO_ERROR) {
            return;
        }

        std::vector<GLenum> errors;

        do {
            errors.push_back(err);
        } while ((err = glGetError()) != GL_NO_ERROR);

        std::stringstream msg;
        msg << func << " failed";
        if (errors.size() == 1) {
            msg << ": ";
        } else {
            msg << " with " << errors.size() << " errors: ";
        }
        for (auto it = errors.begin(); it != errors.end()-1; ++it) {
            msg << to_string(gluErrorString(*it)) << ", ";
        }
        msg << to_string(gluErrorString(errors.back()));

        throw std::runtime_error{msg.str()};
    }

    Vertex_shader CompileVertexShaderFile(char const* path) {
        return CompileVertexShader(slurp_file(path).c_str());
    }

    Fragment_shader CompileFragmentShaderFile(char const* path) {
        return CompileFragmentShader(slurp_file(path).c_str());
    }

    gl::Geometry_shader CompileGeometryShaderFile(char const* path) {
        return CompileGeometryShader(slurp_file(path).c_str());
    }

    void TexImage2D(gl::Texture_2d& t, GLint mipmap_lvl, stbi::Image const& image) {
        stbi_set_flip_vertically_on_load(true);
        gl::BindTexture(t);
        glTexImage2D(GL_TEXTURE_2D,
                     mipmap_lvl,
                     image.nrChannels == 3 ? GL_RGB : GL_RGBA,
                     image.width,
                     image.height,
                     0,
                     image.nrChannels == 3 ? GL_RGB : GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     image.data);
    }

    gl::Texture_2d mipmapped_texture(char const* path) {
        auto t = gl::Texture_2d{};
        auto img = stbi::Image{path};
        TexImage2D(t, 0, img);
        gl::GenerateMipMap(t);
        return t;
    }

    void Uniform(gl::UniformMatrix3fv& u, glm::mat3 const& mat) {
        glUniformMatrix3fv(u.handle, 1, false, glm::value_ptr(mat));
    }

    void Uniform(gl::UniformVec4f& u, glm::vec4 const& v) {
        glUniform4f(u.handle, v.x, v.y, v.z, v.w);
    }

    void Uniform(gl::UniformVec3f& u, glm::vec3 const& v) {
        glUniform3f(u.handle, v.x, v.y, v.z);
    }

    void Uniform(gl::UniformMatrix4fv& u, glm::mat4 const& mat) {
        gl::Uniform(u, glm::value_ptr(mat));
    }
}
