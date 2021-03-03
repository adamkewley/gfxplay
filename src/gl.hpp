#pragma once

#include <GL/glew.h>
#include <cassert>

#include <stdexcept>
#include <type_traits>
#include <initializer_list>
#include <limits>

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

    template<typename TGlsl>
    class Shader_symbol {
        GLint location;
    public:
        constexpr Shader_symbol(GLint _location) noexcept : location{_location} {
        }

        [[nodiscard]] constexpr GLuint get() const noexcept {
            return static_cast<GLuint>(location);
        }

        [[nodiscard]] constexpr GLint geti() const noexcept {
            return static_cast<GLint>(location);
        }
    };

    template<typename TGlsl>
    class Uniform_ : public Shader_symbol<TGlsl> {
    public:
        constexpr Uniform_(GLint _location) noexcept :
            Shader_symbol<TGlsl>{_location} {
        }

        Uniform_(Program const& p, GLchar const* name) :
            Uniform_{GetUniformLocation(p, name)} {
        }
    };

    using Uniform_float = Uniform_<glsl::float_>;
    using Uniform_int = Uniform_<glsl::int_>;
    using Uniform_mat4 = Uniform_<glsl::mat4>;
    using Uniform_mat3 = Uniform_<glsl::mat3>;
    using Uniform_vec4 = Uniform_<glsl::vec4>;
    using Uniform_vec3 = Uniform_<glsl::vec3>;
    using Uniform_vec2 = Uniform_<glsl::vec2>;
    using Uniform_bool = Uniform_int;
    using Uniform_sampler2d = Uniform_int;
    using Uniform_samplerCube = Uniform_int;

    inline void Uniform(Uniform_float& u, GLfloat value) noexcept {
        glUniform1f(u.geti(), value);
    }

    inline void Uniform(Uniform_int& u, GLint value) noexcept {
        glUniform1i(u.geti(), value);
    }

    template<typename TGlsl>
    class Attribute : public Shader_symbol<TGlsl> {
    public:
        constexpr Attribute(GLint  _location) noexcept : Shader_symbol<TGlsl>{_location} {
        }

        Attribute(Program const& p, GLchar const* name) :
            Attribute{GetAttribLocation(p, name)} {
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
    class Texture_handle {
        GLuint handle;
    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Texture_handle() {
            glGenTextures(1, &handle);
        }
        Texture_handle(Texture_handle const&) = delete;
        Texture_handle(Texture_handle&& tmp) : handle{tmp.handle} {
            tmp.handle = senteniel;
        }
        Texture_handle& operator=(Texture_handle const&) = delete;
        Texture_handle& operator=(Texture_handle&&) = delete;
        ~Texture_handle() noexcept {
            if (handle != senteniel) {
                glDeleteTextures(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml
    inline void ActiveTexture(GLenum texture) {
        glActiveTexture(texture);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture(GLenum target, Texture_handle const& texture) {
        glBindTexture(target, texture.raw_handle());
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    inline void BindTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    template<GLenum TextureType>
    class Texture {
        Texture_handle handle;
    public:
        static constexpr GLenum type = TextureType;

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return handle.raw_handle();
        }

        constexpr operator Texture_handle const&() const noexcept {
            return handle;
        }
    };

    using Texture_2d = Texture<GL_TEXTURE_2D>;
    using Texture_cubemap = Texture<GL_TEXTURE_CUBE_MAP>;
    using Texture_2d_multisample = Texture<GL_TEXTURE_2D_MULTISAMPLE>;

    template<typename Texture>
    inline void BindTexture(Texture const& t) noexcept {
        glBindTexture(t.type, t.raw_handle());
    }

    class Frame_buffer final {
        GLuint handle;
    public:
        static constexpr GLuint senteniel = static_cast<GLuint>(-1);

        Frame_buffer() {
            glGenFramebuffers(1, &handle);
        }
        Frame_buffer(Frame_buffer const&) = delete;
        Frame_buffer(Frame_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = senteniel;
        }
        Frame_buffer& operator=(Frame_buffer const&) = delete;
        Frame_buffer& operator=(Frame_buffer&&) = delete;
        ~Frame_buffer() noexcept {
            if (handle != senteniel) {
                glDeleteFramebuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml
    inline void BindFramebuffer(GLenum target, Frame_buffer const& fb) {
        glBindFramebuffer(target, fb.raw_handle());
    }

    struct Window_fbo final {};
    static constexpr Window_fbo window_fbo{};
    inline void BindFramebuffer(GLenum target, Window_fbo) noexcept {
        glBindFramebuffer(target, 0);
    }

    // RAII wrapper for glDeleteRenderBuffers
    //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDeleteRenderbuffers.xhtml
    class Render_buffer final {
        GLuint handle;
    public:
        static constexpr GLuint senteniel = 0;

        Render_buffer() {
            glGenRenderbuffers(1, &handle);
            assert(handle != 0 && "OpenGL spec: The value zero is reserved, but there is no default renderbuffer object. Instead, renderbuffer set to zero effectively unbinds any renderbuffer object previously bound");
        }
        Render_buffer(Render_buffer const&) = delete;
        Render_buffer(Render_buffer&& tmp) : handle{tmp.handle} {
            tmp.handle = senteniel;
        }
        Render_buffer& operator=(Render_buffer const&) = delete;
        Render_buffer& operator=(Render_buffer&& tmp) {
            GLuint v = handle;
            handle = tmp.handle;
            tmp.handle = v;
            return *this;
        }

        ~Render_buffer() noexcept {
            if (handle != senteniel) {
                glDeleteRenderbuffers(1, &handle);
            }
        }

        [[nodiscard]] constexpr GLuint raw_handle() const noexcept {
            return handle;
        }
    };

    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindRenderbuffer.xml
    inline void BindRenderBuffer(Render_buffer& rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, rb.raw_handle());
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
    template<typename Attribute>
    inline void VertexAttribDivisor(Attribute loc, GLuint divisor) noexcept {
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
    template<typename Texture>
    inline void TextureParameteri(Texture const& texture, GLenum pname, GLint param) {
        glTextureParameteri(texture.raw_handle(), pname, param);
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glRenderbufferStorage.xhtml
    inline void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
        glRenderbufferStorage(target, internalformat, width, height);
    }
}
