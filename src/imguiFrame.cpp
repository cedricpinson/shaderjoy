#include "Application.h"
#include "UniformList.h"
#include <imgui/imgui.h>

void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplGlfw_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData*);

void frameIMGUI(Application* app, const UniformList& uniformList)
{
    (void)app;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool showDemo = false;
    if (showDemo) {
        ImGui::ShowDemoWindow(&showDemo);
    }

    ImGuiWindowFlags windowFlags = 0;
    // windowFlags |= ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("stats", nullptr, windowFlags);
    ImGui::Text("FPS %.1f", uniformList.iFrameRate);
    ImGui::SameLine();
    ImGui::Text("Frame %d", uniformList.iFrame);
    ImGui::SameLine();
    ImGui::Text("Time %.2f", uniformList.iTime);

    ImGui::SameLine(240);
    const int width = static_cast<int>(double(app->width) * app->pixelRatio);
    const int height = static_cast<int>(double(app->height) * app->pixelRatio);
    ImGui::Text("%d x %d", width, height);

    ImGui::End();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
