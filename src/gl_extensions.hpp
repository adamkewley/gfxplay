#pragma once

#include "gl.hpp"

// glm
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// forward-declare std::ostream
#include <iosfwd>


// gl extensions: useful extension/helper methods over base OpenGL API

namespace gl {
    // type-safe wrapper over GL_VERTEX_SHADER
    struct Vertex_shader final {
        Shader_handle handle = CreateShader(GL_VERTEX_SHADER);
    };

    void AttachShader(Program& p, Vertex_shader& vs);

    // type-safe wrapper over GL_FRAGMENT_SHADER
    struct Fragment_shader final {
        Shader_handle handle = CreateShader(GL_FRAGMENT_SHADER);
    };

    void AttachShader(Program& p, Fragment_shader& fs);

    // type-safe wrapper over GL_GEOMETRY_SHADER
    struct Geometry_shader final {
        Shader_handle handle = CreateShader(GL_GEOMETRY_SHADER);
    };

    void AttachShader(Program& p, Geometry_shader& gs);

    Vertex_shader CompileVertexShader(char const* src);
    Vertex_shader CompileVertexShaderFile(char const* path);
    Fragment_shader CompileFragmentShader(char const* src);
    Fragment_shader CompileFragmentShaderFile(char const* path);
    Geometry_shader CompileGeometryShader(char const* src);
    Geometry_shader CompileGeometryShaderFile(char const* path);

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs);

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs,
                              Geometry_shader const& gs);

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* func);

    gl::Texture_2d mipmapped_texture(char const* path);

    // set uniforms directly from GLM types
    void Uniform(gl::UniformMatrix3fv& u, glm::mat3 const& mat);
    void Uniform(gl::UniformMatrix4fv& u, glm::mat4 const& mat);
    void Uniform(gl::UniformVec3f& u, glm::vec3 const& v);
    void Uniform(gl::UniformVec4f& u, glm::vec4 const& v);
}

namespace glm {
    std::ostream& operator<<(std::ostream&, glm::vec3 const&);
    std::ostream& operator<<(std::ostream&, glm::vec4 const&);
    std::ostream& operator<<(std::ostream&, glm::mat4 const&);
}
