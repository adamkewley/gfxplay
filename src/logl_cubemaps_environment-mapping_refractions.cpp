#include "logl_common.hpp"

#include <memory>

namespace {
    static constexpr float cube_verts[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
    };

    std::shared_ptr<gl::Texture_cubemap> load_cubemap() {
        static std::shared_ptr<gl::Texture_cubemap> cm = std::make_shared<gl::Texture_cubemap>(
            gl::read_cubemap(
                RESOURCES_DIR "textures/skybox/right.jpg",
                RESOURCES_DIR "textures/skybox/left.jpg",
                RESOURCES_DIR "textures/skybox/top.jpg",
                RESOURCES_DIR "textures/skybox/bottom.jpg",
                RESOURCES_DIR "textures/skybox/front.jpg",
                RESOURCES_DIR "textures/skybox/back.jpg"));
        return cm;
    }

    struct Skybox_prog final {
        std::shared_ptr<gl::Texture_cubemap> cubemap = load_cubemap();

        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(R"(
#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);

    // skybox optimization: the skybox is always infinitely (or max distance)
    // away, so everything else should draw over it.
    //
    // One (suboptimal) way to do this is to draw the skybox first and then
    // draw the rest of the scene over it. That works, but is suboptimal because
    // it makes the fragment shader draw a whole screen's worth of skybox.
    //
    // Another (faster) way to do this is to draw the skybox last, but at the
    // maximum NDC distance (z = 1.0). By the time the skybox is being drawn
    // (last) the rest of the scene, wherever it draws, has populated the
    // depth buffer with depths of z < 1.0. Wherever that's true (i.e. wherever
    // the scene was drawn), the skybox's fragment will fail the early depth
    // test and OpenGL will skip running the fragment shader on it.
    //
    // We set the Z component to 'w' here because OpenGL performs perspective
    // division on gl_Position after the vertex shader runs to yield the NDC
    // of the vertex. That

    gl_Position = pos.xyww;
}
)"),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}
)"
        ));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        gl::Uniform_mat4f projection = gl::GetUniformLocation(prog, "projection");
        gl::Uniform_mat4f view = gl::GetUniformLocation(prog, "view");

        gl::Array_buffer cube_ab = []() {
            auto buf = gl::GenArrayBuffer();
            gl::BindBuffer(buf);
            gl::BufferData(buf.type, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
            return buf;
        }();

        gl::Vertex_array vao = [&]() {
            auto vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(cube_ab);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), nullptr);
            gl::EnableVertexAttribArray(aPos);
            gl::BindVertexArray();
            return vao;
        }();

        void draw(ui::Game_state const& g) {
            glDepthFunc(GL_LEQUAL);  // for the optimization (see shader)

            gl::UseProgram(prog);
            gl::Uniform(projection, g.camera.persp_mtx());

            // remove translation component from camera view matrix, giving
            // the impression that the cubemap is infinitely far away (i.e.
            // no matter how far the player travels, they never get closer to
            // the cubemap)
            glm::mat4 v = glm::mat4(glm::mat3(g.camera.view_mtx()));
            gl::Uniform(view, v);

            gl::BindVertexArray(vao);
            gl::BindTexture(*cubemap);
            gl::DrawArrays(GL_TRIANGLES, 0, 36);
            gl::BindVertexArray();

            glDepthFunc(GL_LESS);  // reset to default
        }
    };

    struct Instanced_quad_prog final {
        std::shared_ptr<gl::Texture_cubemap> cubemap = load_cubemap();

        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileVertexShader(R"(
#version 330 core

out vec3 FragPos;
out vec3 Normal;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0f);
    FragPos = aPos;
    Normal = aNormal;
}
)"),
            gl::CompileFragmentShader(R"(
#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform samplerCube skybox;

void main() {
    vec3 view2frag = normalize(FragPos - viewPos);
    vec3 norm = normalize(Normal);
    const float ior = 1.00f/1.52f;
    vec3 frag2cube = refract(view2frag, norm, ior);

    FragColor = texture(skybox, frag2cube);
}
)"
        ));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
        gl::Uniform_mat4f projection = gl::GetUniformLocation(prog, "projection");
        gl::Uniform_mat4f view = gl::GetUniformLocation(prog, "view");
        gl::Uniform_1i uSkyboxSampler = gl::GetUniformLocation(prog, "skybox");
        gl::Uniform_vec3f uViewPos = gl::GetUniformLocation(prog, "viewPos");

        gl::Array_buffer cube_ab = []() {
            auto buf = gl::GenArrayBuffer();
            gl::BindBuffer(buf);
            gl::BufferData(buf.type, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
            return buf;
        }();

        gl::Vertex_array vao = [&]() {
            auto vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(cube_ab);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();
            return vao;
        }();

        void draw(ui::Game_state const& g) {
            gl::UseProgram(prog);

            gl::Uniform(uSkyboxSampler, 0);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(*cubemap);

            gl::Uniform(projection, g.camera.persp_mtx());
            gl::Uniform(view, g.camera.view_mtx());
            gl::Uniform(uViewPos, g.camera.pos);

            gl::BindVertexArray(vao);
            gl::DrawArrays(GL_TRIANGLES, 0, 36);
            gl::BindVertexArray();
        }
    };
}

int main(int, char**) {
    // SDL setup
    auto sdl = ui::Window_state{};
    SDL_SetWindowGrab(sdl.window, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Extra GL setup
    auto skybox = Skybox_prog{};
    auto cube = Instanced_quad_prog{};

    // Game state setup
    auto game = ui::Game_state{};

    // game loop
    auto throttle = util::Software_throttle{8ms};
    SDL_Event e;
    std::chrono::milliseconds last_time = util::now();
    while (true) {
        std::chrono::milliseconds cur_time = util::now();
        std::chrono::milliseconds dt = cur_time - last_time;
        last_time = cur_time;

        while (SDL_PollEvent(&e)) {
            if (game.handle(e) == ui::Handle_response::should_quit) {
                return 0;
            }
        }

        game.tick(dt);

        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cube.draw(game);
        skybox.draw(game);

        throttle.wait();

        SDL_GL_SwapWindow(sdl.window);
    }
}
