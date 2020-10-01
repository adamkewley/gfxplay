#pragma once

#include <GL/glew.h>
#include <string>
#include <sstream>
#include <vector>
#include <optional>

namespace gl {
    std::string to_string(GLubyte const* err_string) {
        return std::string{reinterpret_cast<char const*>(err_string)};
    }

    void assert_no_errors(char const* func) {
        std::vector<GLenum> errors;
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
            errors.push_back(error);
        }

        if (errors.empty()) {
            return;
        }

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

    class Program final {
        GLuint handle = glCreateProgram();
    public:
        Program() {
            if (handle == 0) {
                throw std::runtime_error{"glCreateProgram() failed"};
            }
        }
        Program(Program const&) = delete;
        Program(Program&& tmp) :
            handle{tmp.handle} {
            tmp.handle = 0;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&&) = delete;
        ~Program() noexcept {
            if (handle != 0) {
                glDeleteProgram(handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void UseProgram(Program& p) {
        glUseProgram(p);
        assert_no_errors("glUseProgram");
    }

    void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    static std::optional<std::string> get_shader_compile_errors(GLuint shader) {
        GLint params = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
        if (params == GL_FALSE) {
            GLint log_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

            std::vector<GLchar> errmsg(log_len);
            glGetShaderInfoLog(shader, log_len, &log_len, errmsg.data());

            std::stringstream ss;
            ss << "glCompileShader() failed: ";
            ss << errmsg.data();
            return std::optional<std::string>{ss.str()};
        }
        return std::nullopt;
    }

    struct Shader {
        static Shader Compile(GLenum shaderType, char const* src) {
            Shader shader{shaderType};
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            if (auto compile_errors = get_shader_compile_errors(shader); compile_errors) {
                throw std::runtime_error{"error compiling vertex shader: "s + compile_errors.value()};
            }

            return shader;
        }
    private:
        GLuint handle;
    public:
        Shader(GLenum shaderType) :
            handle{glCreateShader(shaderType)} {

            if (handle == 0) {
                throw std::runtime_error{"glCreateShader() failed"};
            }
        }
        Shader(Shader const&) = delete;
        Shader(Shader&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Shader& operator=(Shader const&) = delete;
        Shader& operator=(Shader&&) = delete;
        ~Shader() noexcept {
            if (handle != 0) {
                glDeleteShader(handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void AttachShader(Program& p, Shader& s) {
        glAttachShader(p, s);
        assert_no_errors("glAttachShader");
    }

    struct Vertex_shader final : public Shader {
        static Vertex_shader Compile(char const* src) {
            return Vertex_shader{Shader::Compile(GL_VERTEX_SHADER, src)};
        }
    };

    struct Fragment_shader final : public Shader {
        static Fragment_shader Compile(char const* src) {
            return Fragment_shader{Shader::Compile(GL_FRAGMENT_SHADER, src)};
        }
    };

    void LinkProgram(gl::Program& prog) {
        glLinkProgram(prog);

        GLint link_status = GL_FALSE;
        glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
        if (link_status == GL_FALSE) {
            GLint log_len = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);

            std::vector<GLchar> errmsg(log_len);
            glGetProgramInfoLog(prog, static_cast<GLsizei>(errmsg.size()), nullptr, errmsg.data());

            std::stringstream ss;
            ss << "OpenGL: glLinkProgram() failed: ";
            ss << errmsg.data();
            throw std::runtime_error{ss.str()};
        }
    }

    class Uniform {
    protected:
        GLint handle;
    public:
        Uniform(Program& p, char const* name) :
            handle{glGetUniformLocation(p, name)} {

            if (handle == -1) {
                throw std::runtime_error{"glGetUniformLocation() failed: cannot get "s + name};
            }
        }

        operator GLint () noexcept {
            return handle;
        }
    };

    struct Uniform1f final : public Uniform {
        Uniform1f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(Uniform1f& u, GLfloat value) {
        glUniform1f(u, value);
    }

    struct Uniform1i final : public Uniform {
        Uniform1i(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(Uniform1i& u, GLint value) {
        glUniform1i(u, value);
    }

    struct UniformMatrix4fv final : public Uniform {
        UniformMatrix4fv(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    void Uniform(UniformMatrix4fv& u, GLfloat const* value) {
        glUniformMatrix4fv(u, 1, false, value);
    }

    struct UniformVec4f final : public Uniform {
        UniformVec4f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    struct UniformVec3f final : public Uniform {
        UniformVec3f(Program& p, char const* name) :
            Uniform{p, name} {
        }
    };

    class Attribute final {
        GLint handle;
    public:
        Attribute(Program& p, char const* name) :
            handle{glGetAttribLocation(p, name)} {
            if (handle == -1) {
                throw std::runtime_error{"glGetAttribLocation() failed: cannot get "s + name};
            }
        }

        operator GLint () noexcept {
            return handle;
        }
    };

    void VertexAttributePointer(Attribute& a,
                                GLint size,
                                GLenum type,
                                GLboolean normalized,
                                GLsizei stride,
                                const void * pointer) {
        glVertexAttribPointer(a, size, type, normalized, stride, pointer);
    }

    void EnableVertexAttribArray(Attribute& a) {
        glEnableVertexAttribArray(a);
    }

    class Buffer {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Buffer(GLenum target) {
            glGenBuffers(1, &handle);
        }
        Buffer(Buffer const&) = delete;
        Buffer(Buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Buffer& operator=(Buffer const&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        ~Buffer() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteBuffers(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void BindBuffer(GLenum target, Buffer& buffer) {
        glBindBuffer(target, buffer);
    }

    void BufferData(GLenum target, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(target, num_bytes, data, usage);
    }

    struct Array_buffer final : public Buffer {
        Array_buffer() : Buffer{GL_ARRAY_BUFFER} {
        }
    };

    void BindBuffer(Array_buffer& buffer) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
    }

    void BufferData(Array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(GL_ARRAY_BUFFER, num_bytes, data, usage);
    }

    struct Element_array_buffer final : public Buffer {
        Element_array_buffer() : Buffer{GL_ELEMENT_ARRAY_BUFFER} {
        }
    };

    void BindBuffer(Element_array_buffer& buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    }

    void BufferData(Element_array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, data, usage);
    }

    class Vertex_array final {
        GLuint handle = static_cast<GLuint>(-1);
    public:
        Vertex_array() {
            glGenVertexArrays(1, &handle);
        }
        Vertex_array(Vertex_array const&) = delete;
        Vertex_array(Vertex_array&& tmp) :
            handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Vertex_array& operator=(Vertex_array const&) = delete;
        Vertex_array& operator=(Vertex_array&&) = delete;
        ~Vertex_array() noexcept {
            if (handle == static_cast<GLuint>(-1)) {
                glDeleteVertexArrays(1, &handle);
            }
        }

        operator GLuint () noexcept {
            return handle;
        }
    };

    void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao);
    }

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
}
