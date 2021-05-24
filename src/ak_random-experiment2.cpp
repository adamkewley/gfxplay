#include "app.hpp"

#include "gl.hpp"
#include "gl_extensions.hpp"
#include "runtime_config.hpp"

#include <imgui.h>

#include <array>

static constexpr std::array<glm::vec3, 10> cubePositions = {{
    { 0.0f,  0.0f,  0.0f },
    { 2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f, -2.5f },
    {-3.8f, -2.0f, -12.3f},
    { 2.4f, -0.4f, -3.5f },
    {-1.7f,  3.0f, -7.5f },
    { 1.3f, -2.0f, -2.5f },
    { 1.5f,  2.0f, -2.5f },
    { 1.5f,  0.2f, -1.5f },
    {-1.3f,  1.0f, -1.5f },
}};

static char const vs[] = R"(
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

static char const fs[] = R"(
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

struct Vert final {
    glm::vec3 pos;
    glm::vec2 uv;
};

static constexpr Vert cubeVerts[] = {
    // pos                  // uv
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},

    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},

    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},

    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},

    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
};

struct Shader final {
    gl::Program prog = gl::CreateProgramFrom(
        gl::Vertex_shader::from_source(vs),
        gl::Fragment_shader::from_source(fs));
    gl::Attribute_vec3 aPos{0};
    gl::Attribute_vec2 aTexCoord{1};
    gl::Uniform_mat4 uModel{prog, "uModel"};
    gl::Uniform_mat4 uView{prog, "uView"};
    gl::Uniform_mat4 uProjection{prog, "uProjection"};
    gl::Uniform_sampler2d uSampler0{prog, "uSampler0"};
    gl::Uniform_sampler2d uSampler1{prog, "uSampler1"};
};

struct MainScreen : public gp::Screen {
    Shader shader;

    // cube data
    gl::Array_buffer<Vert> cubeVbo{cubeVerts};
    gl::Vertex_array cubeVao = [this]() {
        gl::BindBuffer(cubeVbo);
        gl::VertexAttribPointer(shader.aPos, false, sizeof(Vert), offsetof(Vert, pos));
        gl::EnableVertexAttribArray(shader.aPos);
        gl::VertexAttribPointer(shader.aTexCoord, false, sizeof(Vert), offsetof(Vert, uv));
        gl::EnableVertexAttribArray(shader.aTexCoord);
    };

    // textures
    gl::Texture_2d wall = gl::load_tex(gfxplay::resource_path("wall.jpg"));
    gl::Texture_2d face = gl::load_tex(gfxplay::resource_path("awesomeface.png"));

    gp::Euler_perspective_camera camera;

    bool onEvent(SDL_Event const& e) override {
        switch (e.type) {
        case SDL_MOUSEMOTION:
            return camera.onEvent(e.motion);
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            return camera.onEvent(e.key);
        default:
            return false;
        }
    }

    void onUpdate() override {
        camera.onUpdate();
    }

    void onDraw(double) override {
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl::UseProgram(shader.prog);

        // uModel: handled for each cube below
        gl::Uniform(shader.uView, camera.viewMatrix());
        gl::Uniform(shader.uProjection, camera.projectionMatrix(gp::App::aspectRatio()));

        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(wall);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());

        gl::ActiveTexture(GL_TEXTURE1);
        gl::BindTexture(face);
        gl::Uniform(shader.uSampler1, gl::texture_index<GL_TEXTURE1>());

        gl::BindVertexArray(cubeVao);
        float angle = 0.0f;
        const glm::vec3 rotationAxis{1.0f, 0.3f, 0.5f};
        for (glm::vec3 const& cubePos : cubePositions) {
            glm::mat4 modelMtx = glm::identity<glm::mat4>();
            modelMtx = glm::translate(modelMtx, cubePos);
            modelMtx = glm::rotate(modelMtx, glm::radians(angle), rotationAxis);

            gl::Uniform(shader.uModel, modelMtx);
            gl::DrawArrays(GL_TRIANGLES, 0, cubeVbo.sizei());
            angle += 20.0f;
        }
        gl::BindVertexArray();

    }
};

int main(int, char*[]) {
    gp::App app;
    app.hideCursor();
    app.show(std::make_unique<MainScreen>());
    return 0;
}
