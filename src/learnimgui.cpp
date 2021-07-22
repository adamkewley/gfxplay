#include "app.hpp"
#include "gl.hpp"
#include "gl_extensions.hpp"

#include <imgui.h>

using namespace gp;

struct LearnImGuiScreen : public Layer {

    void onMount() override {
        ImGuiInit();
    }

    void onUnmount() override {
        ImGuiShutdown();
    }

    bool onEvent(SDL_Event const& e) override {
        return ImGuiOnEvent(e);
    }

    void onDraw() override {
        ImGuiNewFrame();
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT);

        ImDrawList& dl = *ImGui::GetForegroundDrawList();
        dl.AddLine(ImVec2{0.0f, 0.0f}, ImVec2{100.0f, 100.0f}, ImU32{0xff0000ff});
        dl.AddLine(ImVec2{100.0f, 100.0f}, ImVec2{100.0f, 0.0f}, ImU32{0xff0000ff});
        log::info("%i", dl.CmdBuffer.Size);

        ImGuiRender();
    }
};

int main(int, char**) {
    App app;
    app.show(std::make_unique<LearnImGuiScreen>());
    return 0;
}
