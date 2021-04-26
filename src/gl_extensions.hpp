#pragma once

#include "gl.hpp"

// glm
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <filesystem>
#include <array>
#include <vector>
#include <type_traits>


// gl extensions: useful extension/helper methods over base OpenGL API
//
// these are helpful sugar methods over the base OpenGL API. Anything that is
// OpenGL-ey, but not "pure" OpenGL, goes here.

// MACROS: usually for sanity-checking during debugging

#define AKGL_STRINGIZE(x) AKGL_STRINGIZE2(x)
#define AKGL_STRINGIZE2(x) #x

#ifndef NDEBUG
#define AKGL_ENABLE(capability) { \
    glEnable(capability); \
    gl::assert_no_errors(__FILE__ ":" AKGL_STRINGIZE(__LINE__) ": glEnable: " #capability); \
}
#else
#define AKGL_ENABLE(capability) { \
    glEnable(capability); \
}
#endif

#define AKGL_ASSERT_NO_ERRORS() { \
    gl::assert_no_errors(__FILE__ ":" AKGL_STRINGIZE(__LINE__)); \
}

namespace gl {
    inline void Uniform(Uniform_int const& u, GLsizei n, GLint const* data) {
        glUniform1iv(u.geti(), n, data);
    }

    inline void Uniform(Uniform_mat3& u, glm::mat3 const& mat) noexcept {
        glUniformMatrix3fv(u.geti(), 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_vec4& u, glm::vec4 const& v) noexcept {
        glUniform4fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, glm::vec3 const& v) noexcept {
        glUniform3fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec3& u, float x, float y, float z) noexcept {
        glUniform3f(u.geti(), x, y, z);
    }

    inline void Uniform(Uniform_vec3& u, GLsizei n, glm::vec3 const* vs) noexcept {
        static_assert(sizeof(glm::vec3) == 3*sizeof(GLfloat));
        glUniform3fv(u.geti(), n, glm::value_ptr(*vs));
    }

    // set a uniform array of vec3s from a userspace container type (e.g. vector<glm::vec3>)
    template<typename Container, size_t N>
    inline std::enable_if_t<std::is_same_v<glm::vec3, typename Container::value_type>, void> Uniform(Uniform_array<glsl::vec3, N>& u, Container& container) {
        assert(container.size() == N);
        glUniform3fv(u.geti(), static_cast<GLsizei>(container.size()), glm::value_ptr(*container.data()));
    }

    inline void Uniform(Uniform_mat4& u, glm::mat4 const& mat) noexcept {
        glUniformMatrix4fv(u.geti(), 1, false, glm::value_ptr(mat));
    }

    inline void Uniform(Uniform_mat4& u, GLsizei n, glm::mat4 const* first) noexcept {
        static_assert(sizeof(glm::mat4) == 16*sizeof(GLfloat));
        glUniformMatrix4fv(u.geti(), n, false, glm::value_ptr(*first));
    }


    struct Uniform_identity_val_tag {};
    inline Uniform_identity_val_tag identity_val;

    inline void Uniform(Uniform_mat4& u, Uniform_identity_val_tag) noexcept {
        Uniform(u, glm::identity<glm::mat4>());
    }

    inline void Uniform(Uniform_vec2& u, glm::vec2 const& v) noexcept {
        glUniform2fv(u.geti(), 1, glm::value_ptr(v));
    }

    inline void Uniform(Uniform_vec2& u, GLsizei n, glm::vec2 const* vs) noexcept {
        static_assert(sizeof(glm::vec2) == 2*sizeof(GLfloat));
        glUniform2fv(u.geti(), n, glm::value_ptr(*vs));
    }

    template<typename Container, size_t N>
    inline std::enable_if_t<std::is_same_v<glm::vec2, Container::value_type>, void> Uniform(Uniform_array<glsl::vec2, N>& u, Container const& container) {
        glUniform2fv(u.geti(), static_cast<GLsizei>(container.size()), glm::value_ptr(container.data()));
    }

    // COMPILE + LINK PROGRAMS:

    Vertex_shader CompileVertexShaderFile(std::filesystem::path const&);
    Vertex_shader CompileVertexShaderResource(char const* resource_id);
    Fragment_shader CompileFragmentShaderFile(std::filesystem::path const&);
    Fragment_shader CompileFragmentShaderResource(char const* resource_id);
    Geometry_shader CompileGeometryShaderFile(std::filesystem::path const&);
    Geometry_shader CompileGeometryShaderResource(char const* resource_id);

    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs);
    Program CreateProgramFrom(Vertex_shader const& vs,
                              Fragment_shader const& fs,
                              Geometry_shader const& gs);


    // OTHER:

    // asserts there are no current OpenGL errors (globally)
    void assert_no_errors(char const* label);

    enum Tex_flags {
        TexFlag_None = 0,
        TexFlag_SRGB = 1,

        // beware: this flips pixels vertically (in Y) but leaves the pixel's
        // contents untouched. This is fine if the pixels represent colors,
        // but causes surprising behavior if the pixels represent vectors (e.g.
        // normal maps)
        TexFlag_Flip_Pixels_Vertically = 2,
    };

    // read an image file into an OpenGL 2D texture
    gl::Texture_2d load_tex(std::filesystem::path const& path, Tex_flags = TexFlag_None);

    // read 6 image files into a single OpenGL cubemap (GL_TEXTURE_CUBE_MAP)
    gl::Texture_cubemap read_cubemap(
            std::filesystem::path const& path_pos_x,
            std::filesystem::path const& path_neg_x,
            std::filesystem::path const& path_pos_y,
            std::filesystem::path const& path_neg_y,
            std::filesystem::path const& path_pos_z,
            std::filesystem::path const& path_neg_z);

    inline glm::mat3 normal_matrix(glm::mat4 const& m) {
         return glm::transpose(glm::inverse(m));
    }

    template<GLenum E>
    inline constexpr unsigned texture_index() {
        static_assert(GL_TEXTURE0 <= E and E <= GL_TEXTURE30);
        return E - GL_TEXTURE0;
    }

    template<typename... T>
    inline void DrawBuffers(T... vs) {
        GLenum attachments[sizeof...(vs)] = { static_cast<GLenum>(vs)... };
        glDrawBuffers(sizeof...(vs), attachments);
    }

    inline void assert_current_fbo_complete() {
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
}
