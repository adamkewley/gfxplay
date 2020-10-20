#include "gl.hpp"

#include <sstream>
#include <vector>

using std::literals::operator""s;

gl::Shader_handle gl::CreateShader(GLenum shaderType) {
    GLuint handle = glCreateShader(shaderType);
    if (handle == 0) {
        throw std::runtime_error{"glCreateShader() failed"};
    }
    return Shader_handle{handle};
}

void gl::ShaderSource(Shader_handle& sh, char const* src) {
    glShaderSource(sh.handle, 1, &src, nullptr);
}

void gl::CompileShader(Shader_handle& sh) {
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

gl::Program gl::CreateProgram() {
    GLuint handle = glCreateProgram();
    if (handle == 0) {
        throw std::runtime_error{"gl::CreateProgram(): failed"};
    }
    return Program{handle};
}

void gl::UseProgram(Program& p) {
    glUseProgram(p.handle);
}

void gl::UseProgram() {
    glUseProgram(static_cast<GLuint>(0));
}

void gl::AttachShader(Program& p, Shader_handle const& sh) {
    glAttachShader(p.handle, sh.handle);
}

void gl::LinkProgram(gl::Program& prog) {
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

GLint gl::GetUniformLocation(Program& p, GLchar const* name) {
    GLint handle = glGetUniformLocation(p.handle, name);
    if (handle == -1) {
        throw std::runtime_error{"glGetUniformLocation() failed: cannot get "s + name};
    }
    return handle;
}

void gl::Uniform(Uniform1i& u, GLint value) {
    glUniform1i(u.handle, value);
}

void gl::Uniform(Uniform1f& u, GLfloat value) {
    glUniform1f(u.handle, value);
}

void gl::Uniform(UniformMatrix4fv& u, GLfloat const* value) {
    glUniformMatrix4fv(u.handle, 1, false, value);
}

gl::Attribute gl::GetAttribLocation(Program& p, char const* name) {
    GLint handle = glGetAttribLocation(p.handle, name);
    if (handle == -1) {
        throw std::runtime_error{"glGetAttribLocation() failed: cannot get "s + name};
    }
    return Attribute{handle};
}

void gl::VertexAttribPointer(Attribute const& a,
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

void gl::EnableVertexAttribArray(Attribute const& a) {
    glEnableVertexAttribArray(a.handle);
}

gl::Buffer_handle gl::GenBuffers() {
    GLuint handle;
    glGenBuffers(1, &handle);
    return Buffer_handle{handle};
}

void gl::BindBuffer(GLenum target, Buffer_handle& buffer) {
    glBindBuffer(target, buffer.handle);
}

void gl::BufferData(GLenum target,
                    size_t num_bytes,
                    void const* data,
                    GLenum usage) {
    glBufferData(target, num_bytes, data, usage);
}

void gl::BindBuffer(Array_buffer& buffer) {
    BindBuffer(GL_ARRAY_BUFFER, buffer.handle);
}

void gl::BufferData(Array_buffer&, size_t num_bytes, void const* data, GLenum usage) {
    glBufferData(GL_ARRAY_BUFFER, num_bytes, data, usage);
}

void gl::BindBuffer(Element_array_buffer& buffer) {
    BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.handle);
}

void gl::BufferData(Element_array_buffer&,
                    size_t num_bytes,
                    void const* data,
                    GLenum usage) {
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_bytes, data, usage);
}

gl::Vertex_array gl::GenVertexArrays() {
    GLuint handle;
    glGenVertexArrays(1, &handle);
    return Vertex_array{handle};
}

void gl::BindVertexArray(Vertex_array& vao) {
    glBindVertexArray(vao.handle);
}

void gl::BindVertexArray() {
    glBindVertexArray(static_cast<GLuint>(0));
}

gl::Texture_handle gl::GenTextures() {
    GLuint handle;
    glGenTextures(1, &handle);
    return Texture_handle{handle};
}

void gl::BindTexture(GLenum target, Texture_handle& texture) {
    glBindTexture(target, texture.handle);
}

void gl::BindTexture() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gl::BindTexture(Texture_2d& texture) {
    BindTexture(GL_TEXTURE_2D, texture.handle);
}

gl::Frame_buffer gl::GenFrameBuffer() {
    GLuint handle;
    glGenFramebuffers(1, &handle);
    return Frame_buffer{handle};
}

void gl::BindFrameBuffer(GLenum target, Frame_buffer const& fb) {
    glBindFramebuffer(target, fb.handle);
}

void gl::BindFrameBuffer() {
    // reset to default (monitor) FB
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

gl::Render_buffer gl::GenRenderBuffer() {
    GLuint handle;
    glGenRenderbuffers(1, &handle);
    return Render_buffer{handle};
}

void gl::BindRenderBuffer(Render_buffer& rb) {
    glBindRenderbuffer(GL_RENDERBUFFER, rb.handle);
}

void gl::BindRenderBuffer() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
