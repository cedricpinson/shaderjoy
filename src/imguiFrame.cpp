#include "Application.h"
#include "UniformList.h"
#include <imgui/imgui.h>
#include <stdio.h>

void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplGlfw_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData*);

size_t printImGuiShaderLine(const Line& line, LineType lineType, int indentationError, char* buffer)
{
    (void)buffer;
    switch (lineType) {
    case LINE_ERROR:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::Text("%*s%.*s", indentationError + 5, "", int(line.size), line.text);
        ImGui::PopStyleColor();
        return 0;
    case LINE_SHADER_ERROR:
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::Text("%3d :%.*s\n", int(line.lineNumber), int(line.size), line.text);
        ImGui::PopStyleColor();
        return 0;
    case LINE_SHADER:
        ImGui::Text("%3d :%.*s", int(line.lineNumber), int(line.size), line.text);
        return 0;
    }
}

void frameIMGUI(Application* app, const UniformList& uniformList)
{
    (void)app;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#if 0
    bool showDemo = false;
    if (showDemo) {
        ImGui::ShowDemoWindow(&showDemo);
    }
#endif

    char menuTitle[1024];
    int index = 0;
    index += sprintf(&menuTitle[index], "FPS %.1f ", uniformList.iFrameRate);
    index += sprintf(&menuTitle[index], "Frame %d ", uniformList.iFrame);
    index += sprintf(&menuTitle[index], "Time %.2f ", uniformList.iTime);
    index += sprintf(&menuTitle[index], "Time %.2f ", uniformList.iTime);
    const int width = static_cast<int>(double(app->width) * app->pixelRatio);
    const int height = static_cast<int>(double(app->height) * app->pixelRatio);
    index += sprintf(&menuTitle[index], "     %d x %d ", width, height);
    index += sprintf(&menuTitle[index], "     compile %s", app->shaderReport.compileSuccess ? "success" : "failed");
    index += sprintf(&menuTitle[index], "###AnimatedTitle");
    menuTitle[index] = 0;

    ImGui::SetNextWindowSize(ImVec2(550, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(menuTitle)) {

        if (!app->shaderReport.compileSuccess) {
            ImGui::Text("Shader Errors %d", int(app->shaderReport.errorLines.size()));
#if 0
            if (ImGui::TreeNode((void*)(intptr_t)0, ))) {
#endif
            for (auto&& line : app->shaderReport.errorLines) {
                ImGui::Text("%d :%.*s", int(line.lineNumber), int(line.size), line.text);
            }
#if 0
                ImGui::TreePop();
            }
#endif
            ImGui::Separator();
        }

#if 0
        if (ImGui::TreeNode("Shader")) {
#endif
        generateShaderTextWithErrorsInlined(app->shaderReport, nullptr, printImGuiShaderLine);
#if 0
        ImGui::TreePop();
    }
#endif
    }
    ImGui::End();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
