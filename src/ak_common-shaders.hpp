#pragma once

#include "logl_common.hpp"

struct Shaded_textured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};
static_assert(sizeof(Shaded_textured_vert) == 8*sizeof(float));

struct Plain_vert final {
    glm::vec3 pos;
};
static_assert(sizeof(Plain_vert) == 3*sizeof(float));

struct Colored_vert final {
    glm::vec3 pos;
    glm::vec3 color;
};
static_assert(sizeof(Colored_vert) == 6*sizeof(float));

// shader that renders geometry with Blinn-Phong shading. Requires the geometry
// to have surface normals and textures
//
// only supports one light and one diffuse texture
struct Blinn_phong_textured_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("selectable.vert"),
        gl::CompileFragmentShaderResource("selectable.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aNormal{1};
    static constexpr gl::Attribute_vec2 aTexCoords{2};

    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};
    gl::Uniform_mat3 uNormalMatrix{p, "normalMatrix"};

    gl::Uniform_sampler2d uTexture1{p, "texture1"};
    gl::Uniform_vec3 uLightPos{p, "lightPos"};
    gl::Uniform_vec3 uViewPos{p, "viewPos"};
};

template<typename Vbo>
static gl::Vertex_array create_vao(Blinn_phong_textured_shader& s, Vbo& vbo) {
    using T = typename Vbo::value_type;

    gl::Vertex_array vao;

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, false, sizeof(T), offsetof(T, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aNormal, false, sizeof(T), offsetof(T, norm));
    gl::EnableVertexAttribArray(s.aNormal);
    gl::VertexAttribPointer(s.aTexCoords, false, sizeof(T), offsetof(T, uv));
    gl::EnableVertexAttribArray(s.aTexCoords);
    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with basic texture mapping (no lighting etc.)
struct Plain_texture_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("plain_texture_shader.vert"),
        gl::CompileFragmentShaderResource("plain_texture_shader.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTextureCoord{1};

    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};

    gl::Uniform_sampler2d uTexture1{p, "texture1"};
    gl::Uniform_mat4 uSamplerMultiplier{p, "uSamplerMultiplier"};
};

static gl::Vertex_array create_vao(
        Plain_texture_shader& s,
        gl::Array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao;

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, false, sizeof(Shaded_textured_vert), offsetof(Shaded_textured_vert, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aTextureCoord, false, sizeof(Shaded_textured_vert), offsetof(Shaded_textured_vert, uv));
    gl::EnableVertexAttribArray(s.aTextureCoord);
    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with a solid, uniform-defined, color
struct Uniform_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("uniform_color_shader.vert"),
        gl::CompileFragmentShaderResource("uniform_color_shader.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};

    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};
    gl::Uniform_vec3 uColor{p, "color"};
};

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Array_buffer<Shaded_textured_vert>& vbo) {

    gl::Vertex_array vao;

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, false, sizeof(Shaded_textured_vert), offsetof(Shaded_textured_vert, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::BindVertexArray();

    return vao;
}

static gl::Vertex_array create_vao(
        Uniform_color_shader& s,
        gl::Array_buffer<Plain_vert>& vbo) {

    gl::Vertex_array vao;

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, false, sizeof(Plain_vert), offsetof(Plain_vert, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::BindVertexArray();

    return vao;
}

// shader that renders geometry with an attribute-defined color
struct Attribute_color_shader final {
    gl::Program p = gl::CreateProgramFrom(
        gl::CompileVertexShaderResource("attribute_color_shader.vert"),
        gl::CompileFragmentShaderResource("attribute_color_shader.frag"));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec3 aColor{1};

    gl::Uniform_mat4 uModel{p, "model"};
    gl::Uniform_mat4 uView{p, "view"};
    gl::Uniform_mat4 uProjection{p, "projection"};
};

static gl::Vertex_array create_vao(
        Attribute_color_shader& s,
        gl::Array_buffer<Colored_vert>& vbo) {

    gl::Vertex_array vao;

    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(s.aPos, false, sizeof(Colored_vert), offsetof(Colored_vert, pos));
    gl::EnableVertexAttribArray(s.aPos);
    gl::VertexAttribPointer(s.aColor, false, sizeof(Colored_vert), offsetof(Colored_vert, color));
    gl::EnableVertexAttribArray(s.aColor);
    gl::BindVertexArray();

    return vao;
}

// standard textured cube with dimensions [-1, +1] in xyz and uv coords of
// (0, 0) bottom-left, (1, 1) top-right for each (quad) face
static constexpr std::array<Shaded_textured_vert, 36> shaded_textured_cube_verts = {{
    // back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}}, // top-left
    // front face
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    // left face
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    {{-1.0f,  1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-right
    // right face
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-left
    // bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, // top-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-left
    {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}}, // bottom-right
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // top-right
    // top face
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{ 1.0f,  1.0f , 1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // top-left
    {{-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}  // bottom-left
}};

// standard textured quad
// - dimensions [-1, +1] in xy and [0, 0] in z
// - uv coords are (0, 0) bottom-left, (1, 1) top-right
// - normal is +1 in Z, meaning that it faces toward the camera
static constexpr std::array<Shaded_textured_vert, 6> shaded_textured_quad_verts = {{
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // top-right
    {{-1.0f, -1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
    {{-1.0f,  1.0f,  0.0f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}  // top-left
}};

static constexpr std::array<Plain_vert, 6> plain_axes_verts = {{
    {{0.0f, 0.0f, 0.0f}}, // x origin
    {{1.0f, 0.0f, 0.0f}}, // x

    {{0.0f, 0.0f, 0.0f}}, // y origin
    {{0.0f, 1.0f, 0.0f}}, // y

    {{0.0f, 0.0f, 0.0f}}, // z origin
    {{0.0f, 0.0f, 1.0f}}  // z
}};

static constexpr std::array<Colored_vert, 6> colored_axes_verts = {{
    // x axis (red)
    {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

    // y axis (green)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

    // z axis (blue)
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
}};
