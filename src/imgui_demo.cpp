#include "logl_common.hpp"
#include "imgui_extensions.hpp"
#include "examples/imgui_impl_sdl.h"
#include "examples/imgui_impl_opengl3.h"

int main(int, char**) {
    auto app = ui::Window_state{};
    auto imgui = igx::Context{};
    auto imguisdl = igx::SDL2_Context{app.window, app.gl};
    auto gl = igx::OpenGL3_Context{"#version 330 core"};
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    bool show_demo = true;

    while (not done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(app.window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(app.window);
        ImGui::NewFrame();
        ImGui::ShowDemoWindow(&show_demo);
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(app.window);
    }
    ImGui::ShowDemoWindow();
}
