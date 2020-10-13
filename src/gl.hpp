#pragma once

#include <GL/glew.h>
#include <string>
#include <sstream>
#include <vector>

using std::literals::operator""s;

namespace gl {
    // gl: extremely thin wrappers around OpenGL.
    //
    // Code in here should:
    //
    //   - Roughly map 1:1 with OpenGL
    //   - Add RAII to types that have destruction methods (e.g. `glDeleteShader`)
    //   - Use exceptions to enforce basic invariants (e.g. compiling a shader
    //     should work, or throw)
    //
    // Emphasis is on simplicity, not "abstraction correctness". It is preferred
    // to have an API that is simple, rather than robustly encapsulated etc.

    // RAII wrapper for glDeleteShader
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml
    struct Shader_handle final {
        GLuint handle;

        Shader_handle(GLuint _handle) : handle{_handle} {
        }
        Shader_handle(Shader_handle const&) = delete;
        Shader_handle(Shader_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Shader_handle& operator=(Shader_handle const&) = delete;
        Shader_handle& operator=(Shader_handle&&) = delete;
        ~Shader_handle() noexcept {
            if (handle != 0) {
                glDeleteShader(handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateShader.xhtml
    Shader_handle CreateShader(GLenum shaderType) {
        GLuint handle = glCreateShader(shaderType);
        if (handle == 0) {
            throw std::runtime_error{"glCreateShader() failed"};
        }
        return Shader_handle{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glShaderSource.xhtml
    void ShaderSource(Shader_handle& sh, char const* src) {
        glShaderSource(sh.handle, 1, &src, nullptr);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCompileShader.xhtml
    void CompileShader(Shader_handle& sh) {
        glCompileShader(sh.handle);

        // check for compile errors
        GLint params = GL_FALSE;
        glGetShaderiv(sh.handle, GL_COMPILE_STATUS, &params);

        if (params == GL_TRUE) {
            return;
        }

        // else: there were compile errors

        GLint log_len = 0;
        glGetShaderiv(sh.handle, GL_INFO_LOG_LENGTH, &log_len);

        std::vector<GLchar> errmsg(log_len);
        glGetShaderInfoLog(sh.handle, log_len, &log_len, errmsg.data());

        std::stringstream ss;
        ss << errmsg.data();
        throw std::runtime_error{"gl::CompileShader failed: "s + ss.str()};
    }

    // RAII for glDeleteProgram
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteProgram.xhtml
    struct Program final {
        GLuint handle;

        Program(GLuint _handle) : handle{_handle} {
        }
        Program(Program const&) = delete;
        Program(Program&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&&) = delete;
        ~Program() noexcept {
            if (handle != 0) {
                glDeleteProgram(handle);
            }
        }
    };

    // RAIIed version of glCreateProgram
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateProgram.xhtml
    Program CreateProgram() {
        GLuint handle = glCreateProgram();
        if (handle == 0) {
            throw std::runtime_error{"gl::CreateProgram(): failed"};
        }
        return Program{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    void UseProgram(Program& p) {
        glUseProgram(p.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glAttachShader.xhtml
    void AttachShader(Program& p, Shader_handle const& sh) {
        glAttachShader(p.handle, sh.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void LinkProgram(gl::Program& prog) {
        glLinkProgram(prog.handle);

        // check for link errors
        GLint link_status = GL_FALSE;
        glGetProgramiv(prog.handle, GL_LINK_STATUS, &link_status);

        if (link_status == GL_TRUE) {
            return;
        }

        // else: there were link errors
        GLint log_len = 0;
        glGetProgramiv(prog.handle, GL_INFO_LOG_LENGTH, &log_len);

        std::vector<GLchar> errmsg(log_len);
        glGetProgramInfoLog(prog.handle, static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

        std::stringstream ss;
        ss << "OpenGL: glLinkProgram() failed: ";
        ss << errmsg.data();
        throw std::runtime_error{ss.str()};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    GLint GetUniformLocation(Program& p, GLchar const* name) {
        GLint handle = glGetUniformLocation(p.handle, name);
        if (handle == -1) {
            throw std::runtime_error{"glGetUniformLocation() failed: cannot get "s + name};
        }
        return handle;
    }

    // type-safe wrapper for glUniform1f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform1f final {
        GLint handle;
        Uniform1f(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    void Uniform(Uniform1f& u, GLfloat value) {
        glUniform1f(u.handle, value);
    }

    // type-safe wrapper for glUniform1i
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct Uniform1i final {
        GLint handle;
        Uniform1i(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    void Uniform(Uniform1i& u, GLint value) {
        glUniform1i(u.handle, value);
    }

    // type-safe wrapper for glUniformMatrix4fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformMatrix4fv final {
        GLint handle;
        UniformMatrix4fv(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    void Uniform(UniformMatrix4fv& u, GLfloat const* value) {
        glUniformMatrix4fv(u.handle, 1, false, value);
    }

    // type-safe wrapper for glUniformMatrix3fv
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformMatrix3fv final {
        GLint handle;
        UniformMatrix3fv(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec4f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformVec4f final {
        GLint handle;
        UniformVec4f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for UniformVec3f
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    struct UniformVec3f final {
        GLint handle;
        UniformVec3f(GLint _handle) : handle{_handle} {}
    };

    // type-safe wrapper for an GLSL attribute index
    //     (just prevents accidently handing a GLint to the wrong API)
    struct Attribute final {
        GLint handle;
        constexpr Attribute(GLint _handle) : handle{_handle} {}
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetAttribLocation.xhtml
    Attribute GetAttribLocation(Program& p, char const* name) {
        GLint handle = glGetAttribLocation(p.handle, name);
        if (handle == -1) {
            throw std::runtime_error{"glGetAttribLocation() failed: cannot get "s + name};
        }
        return Attribute{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
    void VertexAttribPointer(Attribute const& a,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                const void * pointer) {
        glVertexAttribPointer(a.handle,
                              size,
                              type,
                              normalized,
                              stride,
                              pointer);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnableVertexAttribArray.xhtml
    void EnableVertexAttribArray(Attribute const& a) {
        glEnableVertexAttribArray(a.handle);
    }

    // RAII wrapper for glDeleteBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteBuffers.xhtml
    struct Buffer_handle final {
        GLuint handle = static_cast<GLuint>(-1);
        Buffer_handle(GLuint _handle) : handle{_handle} {
        }
        Buffer_handle(Buffer_handle const&) = delete;
        Buffer_handle(Buffer_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Buffer_handle& operator=(Buffer_handle const&) = delete;
        Buffer_handle& operator=(Buffer_handle&&) = delete;
        ~Buffer_handle() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteBuffers(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenBuffers.xhtml
    Buffer_handle GenBuffers() {
        GLuint handle;
        glGenBuffers(1, &handle);
        return Buffer_handle{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    void BindBuffer(GLenum target, Buffer_handle& buffer) {
        glBindBuffer(target, buffer.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
    void BufferData(GLenum target,
                    size_t num_bytes,
                    void const* data,
                    GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
    }

    // type-safe wrapper for GL_ARRAY_BUFFER
    struct Array_buffer final {
        Buffer_handle handle = GenBuffers();
    };

    // type-safe sugar over glBindBuffer
    void BindBuffer(Array_buffer& buffer) {
        BindBuffer(GL_ARRAY_BUFFER, buffer.handle);
    }

    // type-safe sugar over glBufferData
    void BufferData(Array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(GL_ARRAY_BUFFER, num_bytes, data, usage);
    }

    // type-safe wrapper for GL_ELEMENT_ARRAY_BUFFER
    struct Element_array_buffer final {
        Buffer_handle handle = GenBuffers();
    };

    // type-safe sugar over glBindBuffer
    void BindBuffer(Element_array_buffer& buffer) {
        BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.handle);
    }

    // type-safe sugar over glBufferData
    void BufferData(Element_array_buffer&,
                    size_t num_bytes,
                    void const* data,
                    GLenum usage) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, data, usage);
    }

    // RAII wrapper for glDeleteVertexArrays
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteVertexArrays.xhtml
    class Vertex_array final {
        friend Vertex_array GenVertexArrays();
        Vertex_array(GLuint _handle) : handle{_handle} {
        }
    public:
        GLuint handle = static_cast<GLuint>(-1);

        Vertex_array(Vertex_array const&) = delete;
        Vertex_array(Vertex_array&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Vertex_array& operator=(Vertex_array const&) = delete;
        Vertex_array& operator=(Vertex_array&&) = delete;
        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenVertexArrays.xhtml
    Vertex_array GenVertexArrays() {
        GLuint handle;
        glGenVertexArrays(1, &handle);
        return Vertex_array{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    class Texture {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Texture() {
            glGenTextures(1, &handle);
        }
        Texture(Texture const&) = delete;
        Texture(Texture&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteTextures(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    struct Texture_2d final : public Texture {
        Texture_2d() : Texture{} {
        }
    };

    void BindTexture(Texture_2d& texture) {
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GenerateMipMap(Texture_2d&) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    struct Frame_buffer final {
        GLuint handle;

        Frame_buffer(GLuint _handle) : handle{_handle} {
        }
        Frame_buffer(Frame_buffer const&) = delete;
        Frame_buffer(Frame_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Frame_buffer& operator=(Frame_buffer const&) = delete;
        Frame_buffer& operator=(Frame_buffer&&) = delete;
        ~Frame_buffer() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteFramebuffers(1, &handle);
            }
        }
    };

    Frame_buffer GenFrameBuffer() {
        GLuint handle;
        glGenFramebuffers(1, &handle);
        return Frame_buffer{handle};
    }

    void BindFrameBuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb.handle);
    }

    void BindFrameBuffer() {
        // reset to default (monitor) FB
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    struct Render_buffer final {
        GLuint handle;

        Render_buffer(GLuint _handle) : handle{_handle} {
        }
        Render_buffer(Render_buffer const&) = delete;
        Render_buffer(Render_buffer&&) = delete;
        Render_buffer& operator=(Render_buffer const&) = delete;
        Render_buffer& operator=(Render_buffer&&) = delete;
        ~Render_buffer() noexcept {
            glDeleteRenderbuffers(1, &handle);
        }
    };

    Render_buffer GenRenderBuffer() {
        GLuint handle;
        glGenRenderbuffers(1, &handle);
        return Render_buffer{handle};
    }

    void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.handle);
    }

    void BindRenderBuffer() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}
