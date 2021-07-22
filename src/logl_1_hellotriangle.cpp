#include "app.hpp"

#include "gl.hpp"
#include "gl_extensions.hpp"

static constexpr char vertShader[] = R"(
    #version 330 core

    in vec3 aPos;

    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

static constexpr char fragShader[] = R"(
    #version 330 core

    out vec4 FragColor;

    void main() {
        FragColor = vec4(1.0, 0.5f, 0.2f, 1.0f);
    }
)";

struct HelloTriangleScreen : public gp::Layer {
    gl::Program prog = gl::CreateProgramFrom(
        gl::Vertex_shader::from_source(vertShader),
        gl::Fragment_shader::from_source(fragShader));

    gl::Attribute_vec3 aPos{prog, "aPos"};

    gl::Array_buffer<glm::vec3> quadVbo = {
        { 0.5f,  0.5f, 0.0f},  // top right
        { 0.5f, -0.5f, 0.0f},  // bottom right
        {-0.5f, -0.5f, 0.0f},  // bottom left
        {-0.5f,  0.5f, 0.0f},  // top left
    };

    gl::Element_array_buffer<GLuint> quadTrianglesEbo = {
        0, 1, 3,
        1, 2, 3
    };

    gl::Vertex_array vao = [this]() {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quadVbo);
        gl::BindBuffer(quadTrianglesEbo);
        gl::VertexAttribPointer(aPos, false, sizeof(decltype(quadVbo)::value_type), 0);
        gl::EnableVertexAttribArray(aPos);
        gl::BindVertexArray();
        return rv;
    }();

    void onDraw() override {
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::UseProgram(prog);
        gl::BindVertexArray(vao);
        gl::DrawElements(GL_TRIANGLES, quadTrianglesEbo.sizei(), gl::index_type(quadTrianglesEbo), nullptr);
        gl::BindVertexArray();
    }
};

int main(int, char**) {
    gp::App app;
    app.enableOpenGLDebugMode();
    app.show(std::make_unique<HelloTriangleScreen>());
    return 0;
}
