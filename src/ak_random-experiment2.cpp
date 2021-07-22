#include "app.hpp"

#include "gl.hpp"
#include "gl_extensions.hpp"
#include "runtime_config.hpp"

#include <imgui.h>

#include <array>

using namespace gp;

static constexpr std::array<glm::vec3, 10> g_CubePositions = {{
    { 0.0f,  0.0f,  0.0f },
    { 4.0f,  10.0f, -30.0f},
    {-3.0f, -4.4f, -5.0f },
    {-7.6f, -4.0f, -24.6f},
    { 4.8f, -0.8f, -7.0f },
    {-3.4f,  6.0f, -15.0f },
    { 2.6f, -4.0f, -5.0f },
    { 3.0f,  4.0f, -5.0f },
    { 3.0f,  0.4f, -3.0f },
    {-2.6f,  2.0f, -3.0f },
}};

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec3 ourColor;
    out vec2 TexCoord;

    void main()
    {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform sampler2D uSampler0;
    uniform sampler2D uSampler1;

    in vec2 TexCoord;
    out vec4 FragColor;

    void main()
    {
        FragColor = mix(texture(uSampler0, TexCoord), texture(uSampler1, TexCoord), 0.2);
    }
)";

struct Shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::Vertex_shader::from_source(g_VertexShader),
        gl::Fragment_shader::from_source(g_FragmentShader));

    static constexpr gl::Attribute_vec3 aPos{0};
    static constexpr gl::Attribute_vec2 aTexCoord{1};
    gl::Uniform_mat4 uModel{prog, "uModel"};
    gl::Uniform_mat4 uView{prog, "uView"};
    gl::Uniform_mat4 uProjection{prog, "uProjection"};
    gl::Uniform_sampler2d uSampler0{prog, "uSampler0"};
    gl::Uniform_sampler2d uSampler1{prog, "uSampler1"};
};

struct MainScreen final : public Layer {
    Shader shader;

    // cube data
    gl::Array_buffer<TexturedVert> cubeVbo = generateTexturedCubeVerts();
    gl::Vertex_array cubeVao = [this]() {
        gl::Vertex_array vao;
        gl::BindVertexArray(vao);
        gl::BindBuffer(cubeVbo);
        gl::VertexAttribPointer(shader.aPos, false, sizeof(TexturedVert), offsetof(TexturedVert, pos));
        gl::EnableVertexAttribArray(shader.aPos);
        gl::VertexAttribPointer(shader.aTexCoord, false, sizeof(TexturedVert), offsetof(TexturedVert, uv));
        gl::EnableVertexAttribArray(shader.aTexCoord);
        gl::BindVertexArray();
        return vao;
    }();

    // textures
    gl::Texture_2d wall = gl::load_tex(gfxplay::resource_path("wall.jpg"));
    gl::Texture_2d face = gl::load_tex(gfxplay::resource_path("awesomeface.png"));

    // main FPS camera
    Euler_perspective_camera camera;

    void onUpdate() override {
        camera.onUpdate(10.0f, 0.001f);
    }

    void onDraw() override {
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl::UseProgram(shader.prog);

        // `uModel` is handled for each cube below
        gl::Uniform(shader.uView, camera.viewMatrix());
        gl::Uniform(shader.uProjection, camera.projectionMatrix(App::cur().aspectRatio()));

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(wall);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());

        gl::ActiveTexture(GL_TEXTURE1);
        gl::BindTexture(face);
        gl::Uniform(shader.uSampler1, gl::texture_index<GL_TEXTURE1>());

        gl::BindVertexArray(cubeVao);
        float angle = 0.0f;
        for (glm::vec3 const& cubePos : g_CubePositions) {
            glm::mat4 modelMtx = glm::identity<glm::mat4>();
            modelMtx = glm::translate(modelMtx, cubePos);
            modelMtx = glm::rotate(modelMtx, glm::radians(angle), {1.0f, 0.3f, 0.5f});

            gl::Uniform(shader.uModel, modelMtx);
            gl::DrawArrays(GL_TRIANGLES, 0, cubeVbo.sizei());
            angle += 20.0f;
        }
        gl::BindVertexArray();
    }
};

int main(int, char*[]) {
    App app;
    app.enableRelativeMouseMode();
    app.show(std::make_unique<MainScreen>());
    return 0;
}
