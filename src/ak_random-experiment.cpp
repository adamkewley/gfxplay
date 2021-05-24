#include "app.hpp"

#include "gl.hpp"
#include "gl_extensions.hpp"

#include <imgui.h>

static constexpr char const vs[] = R"(
    #version 330 core

    in vec3 aPos;

    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

static constexpr char fs[] = R"(
    #version 330 core

    out vec4 FragColor;
    uniform vec4 uColor;

    void main() {
        FragColor = uColor;
    }
)";

struct MainScreen : public gp::Screen {
    gl::Program prog = gl::CreateProgramFrom(
        gl::Vertex_shader::from_source(vs),
        gl::Fragment_shader::from_source(fs));

    gl::Attribute_vec3 aPos{prog, "aPos"};
    gl::Uniform_vec4 uColor{prog, "uColor"};

    gl::Array_buffer<glm::vec3> vbo = {
        { 0.5f,  0.5f, 0.0f},  // top right
        { 0.5f, -0.5f, 0.0f},  // bottom right
        {-0.5f, -0.5f, 0.0f},  // bottom left
        {-0.5f,  0.5f, 0.0f},  // top left
    };

    gl::Element_array_buffer<GLuint> ebo = {
        0, 1, 3,
        1, 2, 3,
    };

    gl::Vertex_array vao = [this]() {
        gl::BindBuffer(vbo);
        gl::BindBuffer(ebo);
        gl::VertexAttribPointer(aPos, false, sizeof(decltype(vbo)::value_type), 0);
        gl::EnableVertexAttribArray(aPos);
    };

    float color[4] = {1.0f, 0.5f, 0.2f, 1.0f};

    void onMount() override {
        gp::ImGuiInit();
    }

    void onUnmount() override {
        gp::ImGuiShutdown();
    }

    void onDraw(double) override {
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gp::ImGuiNewFrame();

        if (ImGui::Begin("editor")) {
            ImGui::ColorEdit4("color", color);
        }
        ImGui::End();

        gl::UseProgram(prog);
        gl::Uniform(uColor, color);
        gl::BindVertexArray(vao);
        gl::DrawElements(GL_TRIANGLES, ebo.sizei(), GL_UNSIGNED_INT, nullptr);
        gl::BindVertexArray();

        gp::ImGuiRender();
    }
};

int main(int, char*[]) {
    gp::App app;
    app.show(std::make_unique<MainScreen>());
    return 0;
}
