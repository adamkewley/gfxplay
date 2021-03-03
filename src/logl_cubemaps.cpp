#include "logl_common.hpp"

namespace {
    static constexpr float cube_verts[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    struct Skybox_prog final {
        gl::Texture_cubemap cubemap = gl::read_cubemap(
            gfxplay::resource_path("textures", "skybox", "right.jpg"),
            gfxplay::resource_path("textures", "skybox", "left.jpg"),
            gfxplay::resource_path("textures", "skybox", "top.jpg"),
            gfxplay::resource_path("textures", "skybox", "bottom.jpg"),
            gfxplay::resource_path("textures", "skybox", "front.jpg"),
            gfxplay::resource_path("textures", "skybox", "back.jpg")
        );

        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
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
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}
)"
        ));

        static constexpr gl::Attribute_vec3 aPos{0};
        gl::Uniform_mat4 projection{prog, "projection"};
        gl::Uniform_mat4 view{prog, "view"};

        gl::Array_buffer<float> cube_ab{cube_verts};

        gl::Vertex_array vao = [&]() {
            gl::BindBuffer(cube_ab);
            gl::VertexAttribPointer(aPos, false, 3*sizeof(float), 0);
            gl::EnableVertexAttribArray(aPos);
        };

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
            gl::BindTexture(cubemap);
            gl::DrawArrays(GL_TRIANGLES, 0, 36);
            gl::BindVertexArray();

            glDepthFunc(GL_LESS);  // reset to default
        }
    };

    struct Instanced_quad_prog final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::Vertex_shader::from_source(R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0f);
}
)"),
            gl::Fragment_shader::from_source(R"(
#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
)"
        ));

        static constexpr gl::Attribute_vec3 aPos{0};
        gl::Uniform_mat4 projection{prog, "projection"};
        gl::Uniform_mat4 view{prog, "view"};

        gl::Array_buffer<float> cube_ab{cube_verts};

        gl::Vertex_array vao = [this]() {
            gl::BindBuffer(cube_ab);
            gl::VertexAttribPointer(aPos, false, 3*sizeof(float), 0);
            gl::EnableVertexAttribArray(aPos);
        };

        void draw(ui::Game_state const& g) {
            gl::UseProgram(prog);

            gl::Uniform(projection, g.camera.persp_mtx());
            gl::Uniform(view, g.camera.view_mtx());

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
