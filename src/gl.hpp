#pragma once

#include <GL/glew.h>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <initializer_list>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define GFXP_AT __FILE__ ":" TOSTRING(__LINE__)

// gl: thin C++ wrappers around OpenGL
//
// Code in here should:
//
//   - Roughly map 1:1 with OpenGL
//   - Add RAII to types that have destruction methods
//     (e.g. `glDeleteShader`)
//   - Use exceptions to enforce basic invariants (e.g. compiling a shader
//     should work, or throw)
//
// Emphasis is on simplicity, not "abstraction correctness". It is preferred
// to have an API that is simple, rather than robustly encapsulated etc.
//
// `inline`ing here is alright, provided it just forwards the gl calls and
//  doesn't do much else

namespace gl {

    class Shader_handle {
        static constexpr GLuint senteniel = 0;
        GLuint handle;

    public:
        explicit Shader_handle(GLenum type) : handle{glCreateShader(type)} {
            if (handle == senteniel) {
                throw std::runtime_error{GFXP_AT ": glCreateShader() failed"};
            }
        }

        Shader_handle(Shader_handle const&) = delete;

        constexpr Shader_handle(Shader_handle&& tmp) noexcept : handle{tmp.handle} {
            tmp.handle = senteniel;
        }

        Shader_handle& operator=(Shader_handle const&) = delete;

        constexpr Shader_handle& operator=(Shader_handle&& tmp) noexcept {
            auto v = tmp.handle;
            tmp.handle = handle;
            handle = v;
            return *this;
        }

        ~Shader_handle() noexcept {
            if (handle != senteniel) {
                glDeleteShader(handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    void CompileFromSource(Shader_handle const&, const char* src);

    template<GLuint ShaderType>
    class Shader {
        Shader_handle underlying_handle;
    public:
        [[nodiscard]] static Shader<ShaderType> from_source(char const* src) {
            Shader<ShaderType> rv;
            CompileFromSource(rv.handle(), src);
            return rv;
        }

        static constexpr GLuint type = ShaderType;

        Shader() : underlying_handle{type} {
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return underlying_handle.get();
        }

        [[nodiscard]] constexpr Shader_handle const& handle() const noexcept {
            return underlying_handle;
        }
    };

    using Vertex_shader = Shader<GL_VERTEX_SHADER>;
    using Fragment_shader = Shader<GL_FRAGMENT_SHADER>;
    using Geometry_shader = Shader<GL_GEOMETRY_SHADER>;

    class Program final {
        GLuint handle;
    public:
        static constexpr GLuint senteniel = 0;

        Program() : handle{glCreateProgram()} {
            if (handle == senteniel) {
                throw std::runtime_error{GFXP_AT "glCreateProgram() failed"};
            }
        }
        Program(Program const&) = delete;
        Program(Program&& tmp) : handle{tmp.handle} {
            tmp.handle = senteniel;
        }
        Program& operator=(Program const&) = delete;
        Program& operator=(Program&& tmp) {
            auto v = tmp.handle;
            tmp.handle = handle;
            handle = v;
            return *this;
        }
        ~Program() noexcept {
            if (handle != senteniel) {
                glDeleteProgram(handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram(Program const& p) noexcept {
        glUseProgram(p.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUseProgram.xhtml
    inline void UseProgram() {
        glUseProgram(static_cast<GLuint>(0));
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLinkProgram.xhtml
    void LinkProgram(Program& prog);

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //     *throws on error
    [[nodiscard]] inline GLint GetUniformLocation(Program const& p, GLchar const* name) {
        GLint handle = glGetUniformLocation(p.get(), name);
        if (handle == -1) {
            throw std::runtime_error{std::string{"glGetUniformLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    [[nodiscard]] inline GLint GetAttribLocation(Program const& p, GLchar const* name) {
        GLint handle = glGetAttribLocation(p.get(), name);
        if (handle == -1) {
            throw std::runtime_error{std::string{"glGetAttribLocation() failed: cannot get "} + name};
        }
        return handle;
    }

    // metadata for GLSL data types that are typically bound from the CPU via (e.g.)
    // glVertexAttribPointer
    namespace glsl {
        struct float_ final {
            static constexpr GLint size = 1;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct int_ final {
            static constexpr GLint size = 1;
            static constexpr GLenum type = GL_INT;
        };
        using sampler2d = int_;
        using samplerCube = int_;
        using bool_ = int_;
        struct vec2 final {
            static constexpr GLint size = 2;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct vec3 final {
            static constexpr GLint size = 3;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct vec4 final {
            static constexpr GLint size = 4;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct mat4 final {
            static constexpr GLint size = 16;
            static constexpr GLenum type = GL_FLOAT;
        };
        struct mat3 final {
            static constexpr GLint size = 9;
            static constexpr GLenum type = GL_FLOAT;
        };
    }

    class Shader_location {
        GLint value;
    public:
        static constexpr GLint senteniel = -1;

        constexpr Shader_location(GLint _value) noexcept : value{_value} {
        }

        operator bool () const noexcept {
            return value != senteniel;
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return static_cast<GLuint>(value);
        }
    };

    class Attribute_location : public Shader_location {};
    class Uniform_location : public Shader_location {};

    template<typename TGlsl>
    class Attribute final {
        Attribute_location location;
    public:
        [[nodiscard]] static constexpr Attribute at_location(GLint loc) noexcept {
            return Attribute{Attribute_location{loc}};
        }

        [[nodiscard]] static Attribute with_name(Program const& p, GLchar const* name) {
            return Attribute{Attribute_location{GetAttribLocation(p, name)}};
        }

        constexpr Attribute(Shader_location _location) noexcept : location{_location} {
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return location.get();
        }

        operator Attribute_location () const noexcept {
            return location;
        }
    };

    using Attribute_float = Attribute<glsl::float_>;
    using Attribute_int = Attribute<glsl::int_>;
    using Attribute_vec2 = Attribute<glsl::vec2>;
    using Attribute_vec3 = Attribute<glsl::vec3>;
    using Attribute_vec4 = Attribute<glsl::vec4>;
    using Attribute_mat4 = Attribute<glsl::mat4>;
    using Attribute_mat3 = Attribute<glsl::mat3>;

    template<typename TGlsl>
    inline void VertexAttribPointer(Attribute<TGlsl> const& attr, bool normalized, size_t stride, size_t offset) noexcept {
        GLboolean normgl = normalized ? GL_TRUE : GL_FALSE;
        GLsizei stridegl = static_cast<GLsizei>(stride);
        void* offsetgl = reinterpret_cast<void*>(offset);

        if constexpr (TGlsl::size <= 4) {
            glVertexAttribPointer(attr.get(), TGlsl::size, TGlsl::type, normgl, stridegl, offsetgl);
        } else if constexpr (TGlsl::type == GL_FLOAT) {
            static constexpr size_t els_per_step = 4;
            for (unsigned i = 0; i < TGlsl::size/els_per_step; ++i) {
                auto off = reinterpret_cast<void*>(offset + i*els_per_step*sizeof(float));
                glVertexAttribPointer(attr.get() + i, els_per_step, TGlsl::type, normgl, stridegl, off);
            }
        } else {
            throw std::runtime_error{"not supported"};
        }
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glEnableVertexAttribArray.xhtml
    template<typename TGlsl>
    inline void EnableVertexAttribArray(Attribute<TGlsl> const& loc) {
        if constexpr (TGlsl::size <= 4) {
            glEnableVertexAttribArray(loc.get());
        } else if constexpr (TGlsl::type == GL_FLOAT) {
            for (unsigned i = 0; i < TGlsl::size/4; ++i) {
                glEnableVertexAttribArray(loc.get() + i);
            }
        } else {
            throw std::runtime_error{"not supported"};
        }
    }

    template<typename TGlsl>
    inline void VertexAttribDivisor(Attribute<TGlsl> const& loc, GLuint divisor) {
        if constexpr (TGlsl::size <= 4) {
            glVertexAttribDivisor(loc.get(), divisor);
        } else if constexpr (TGlsl::type == GL_FLOAT) {
            for (unsigned i = 0; i < TGlsl::size/4; ++i) {
                glVertexAttribDivisor(loc.get() + i, divisor);
            }
        } else {
            throw std::runtime_error{"not supported"};
        }
    }

    class Buffer_handle final {
        GLuint handle;

    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Buffer_handle() noexcept {
            glGenBuffers(1, &handle);
        }
        Buffer_handle(Buffer_handle const&) = delete;
        Buffer_handle(Buffer_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = senteniel;
        }
        Buffer_handle& operator=(Buffer_handle const&) = delete;
        Buffer_handle& operator=(Buffer_handle&&) = delete;
        ~Buffer_handle() noexcept {
            if (handle != senteniel) {
                glDeleteBuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    inline void BindBuffer(GLenum target, Buffer_handle const& handle) noexcept {
        glBindBuffer(target, handle.get());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    //     *overload that unbinds current buffer
    inline void BindBuffer() noexcept {
        // from docs:
        // > Instead, buffer set to zero effectively unbinds any buffer object
        // > previously bound, and restores client memory usage for that buffer
        // > object target (if supported for that target)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    class Sized_raw_buffer final {
        Buffer_handle underlying_handle;
        size_t nbytes;
    public:
        Sized_raw_buffer() noexcept : underlying_handle{}, nbytes{0} {
        }

        Sized_raw_buffer(GLenum type, void const* begin, size_t n, GLenum usage) noexcept :
            underlying_handle{},
            nbytes{n} {

            glBindBuffer(type, underlying_handle.get());
            glBufferData(type, static_cast<GLsizeiptr>(n), begin, usage);
        }

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return underlying_handle.get();
        }

        [[nodiscard]] constexpr size_t size() const noexcept {
            return nbytes;
        }

        [[nodiscard]] constexpr GLsizei sizei() const noexcept {
            return static_cast<GLsizei>(nbytes);
        }

        void assign(GLenum type, void const* begin, size_t n, GLenum usage) {
            glBindBuffer(type, underlying_handle.get());
            glBufferData(type, static_cast<GLsizeiptr>(n), begin, usage);
            nbytes = n;
        }
    };

    template<typename T, GLenum BufferType, GLenum Usage>
    class Buffer {
        Sized_raw_buffer storage;
    public:
        static_assert(std::is_trivially_copyable<T>::value);
        static_assert(std::is_standard_layout<T>::value);

        using value_type = T;
        static constexpr GLenum buffer_type = BufferType;

        Buffer() : storage{} {
        }

        Buffer(T const* begin, size_t n) : storage{BufferType, begin, n * sizeof(T), Usage} {
        }

        template<typename Collection>
        Buffer(Collection const& c) : storage{BufferType, c.data(), c.size() * sizeof(T), Usage} {
        }

        Buffer(std::initializer_list<T> lst) : Buffer{lst.begin(), lst.size()} {
        }

        template<size_t N>
        Buffer(T const (&arr)[N]) : Buffer{arr, N} {
        }

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return storage.raw_handle();
        }

        [[nodiscard]] constexpr size_t size() const noexcept {
            return storage.size() / sizeof(T);
        }

        [[nodiscard]] constexpr GLsizei sizei() const noexcept {
            return static_cast<GLsizei>(storage.size() / sizeof(T));
        }

        void assign(T const* begin, size_t n) noexcept {
            storage.assign(BufferType, begin, n * sizeof(T), Usage);
        }

        template<typename Container>
        void assign(Container const& c) noexcept {
            storage.assign(BufferType, c.data(), c.size(), Usage);
        }

        template<size_t N>
        void assign(T const (&arr)[N]) {
            storage.assign(BufferType, arr, N * sizeof(T), Usage);
        }
    };

    template<typename T, GLenum Usage = GL_STATIC_DRAW>
    using Array_buffer = Buffer<T, GL_ARRAY_BUFFER, Usage>;

    template<typename T, GLenum Usage = GL_STATIC_DRAW>
    using Element_array_buffer = Buffer<T, GL_ELEMENT_ARRAY_BUFFER, Usage>;

    template<typename Buffer>
    inline void BindBuffer(Buffer const& buf) noexcept {
        glBindBuffer(Buffer::buffer_type, buf.raw_handle());
    }

    // RAII wrapper for glDeleteVertexArrays
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteVertexArrays.xhtml
    class Vertex_array final {
        GLuint handle;
    public:        
        Vertex_array() {
            glGenVertexArrays(1, &handle);
        }

        template<typename SetupFunction>
        Vertex_array(SetupFunction f) {
            glGenVertexArrays(1, &handle);
            glBindVertexArray(handle);
            f();
            glBindVertexArray(static_cast<GLuint>(0));
        }

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

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray(Vertex_array& vao) {
        glBindVertexArray(vao.raw_handle());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml
    inline void BindVertexArray() {
        glBindVertexArray(static_cast<GLuint>(0));
    }

    // RAII wrapper for glGenTextures/glDeleteTextures
    //     https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glDeleteTextures.xml
    struct Texture_handle {
        friend Texture_handle GenTextures();

        GLuint handle;

    protected:
        Texture_handle() {
            glGenTextures(1, &handle);
        }
    public:
        Texture_handle(Texture_handle const&) = delete;
        Texture_handle(Texture_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = static_cast<GLuint>(-1);
        }
        Texture_handle& operator=(Texture_handle const&) = delete;
        Texture_handle& operator=(Texture_handle&&) = delete;
        ~Texture_handle() noexcept {
            if (handle != static_cast<GLuint>(-1)) {
                glDeleteTextures(1, &handle);
            }
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenTextures.xhtml
    inline Texture_handle GenTextures() {
        return Texture_handle{};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture) {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture_handle const& texture) {
        glBindTexture(target, texture.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    inline void TexImage2D(
            GLenum target,
            GLint level,
            GLint internalformat,
            GLsizei width,
            GLsizei height,
            GLint border,
            GLenum format,
            GLenum type,
            const void * data) {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
    }

    // RAII wrapper for glDeleteFrameBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteFramebuffers.xhtml
    struct Frame_buffer final {
        GLuint handle;

    private:
        friend Frame_buffer GenFrameBuffer();
        Frame_buffer(GLuint _handle) : handle{_handle} {
        }
    public:
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

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenFramebuffers.xhtml
    inline Frame_buffer GenFrameBuffer() {
        GLuint handle;
        glGenFramebuffers(1, &handle);
        return Frame_buffer{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, GLuint handle) {
        glBindFramebuffer(target, handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFrameBuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb.handle);
    }

    static constexpr GLuint window_fbo = 0;

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glFramebufferTexture2D.xml
    inline void FramebufferTexture2D(
            GLenum target,
            GLenum attachment,
            GLenum textarget,
            GLuint texture,
            GLint level) {
        glFramebufferTexture2D(target, attachment, textarget, texture, level);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlitFramebuffer.xhtml
    inline void BlitFramebuffer(
            GLint srcX0,
            GLint srcY0,
            GLint srcX1,
            GLint srcY1,
            GLint dstX0,
            GLint dstY0,
            GLint dstX1,
            GLint dstY1,
            GLbitfield mask,
            GLenum filter) {
        glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }

    // RAII wrapper for glDeleteRenderBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteRenderbuffers.xhtml
    struct Render_buffer final {
        GLuint handle;

    private:
        friend Render_buffer GenRenderBuffer();
        Render_buffer(GLuint _handle) : handle{_handle} {
        }
    public:
        Render_buffer(Render_buffer const&) = delete;
        Render_buffer(Render_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = 0;
        }
        Render_buffer& operator=(Render_buffer const&) = delete;
        Render_buffer& operator=(Render_buffer&& tmp) {
            GLuint v = handle;
            handle = tmp.handle;
            tmp.handle = v;
            return *this;
        }

        ~Render_buffer() noexcept {
            if (handle != 0) {
                glDeleteRenderbuffers(1, &handle);
            }
        }

        operator GLuint() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glGenRenderbuffers.xml
    inline Render_buffer GenRenderBuffer() {
        GLuint handle;
        glGenRenderbuffers(1, &handle);
        assert(handle != 0 && "OpenGL spec: The value zero is reserved, but there is no default renderbuffer object. Instead, renderbuffer set to zero effectively unbinds any renderbuffer object previously bound");
        return Render_buffer{handle};
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.handle);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glClear.xhtml
    inline void Clear(GLbitfield mask) {
        glClear(mask);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArrays.xhtml
    inline void DrawArrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawArraysInstanced.xhtml
    inline void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
        glDrawArraysInstanced(mode, first, count, instancecount);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribDivisor.xhtml
    inline void VertexAttribDivisor(Attribute_location loc, GLuint divisor) noexcept {
        glVertexAttribDivisor(loc.get(), divisor);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
    inline void DrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices) {
        glDrawElements(mode, count, type, indices);
    }

    inline void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
        glClearColor(red, green, blue, alpha);
    }

    inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
        glViewport(x, y, w, h);
    }

    inline void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
        glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TexParameteri(GLenum target, GLenum pname, GLint param) {
        glTexParameteri(target, pname, param);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
    inline void TextureParameteri(GLuint texture, GLenum pname, GLint param) {
        glTextureParameteri(texture, pname, param);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
        glRenderbufferStorage(target, internalformat, width, height);
    }
}
