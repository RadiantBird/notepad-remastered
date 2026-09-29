#include <iostream>
#include <GLFW/glfw3.h>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aero UI", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 垂直同期 (VSync) 有効化

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("imguiSources/imgui/misc/fonts/Cousine-Regular.ttf", 20.0f);

    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 5.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);

    style.Colors[ImGuiCol_WindowBg] = ImColor(255, 255, 255, 127);
    style.Colors[ImGuiCol_TitleBgActive]    = ImColor(0, 189, 229, 255);
    style.Colors[ImGuiCol_TitleBg]          = ImColor(0, 98, 175, 255);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(0, 98, 122, 255);

    // バックエンドの初期化 (GLFW + OpenGL3)
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 3. メインループ
    static std::vector<float> values(100);
    static size_t offset = 0;
    static float t = 0.0f;
    {
        float _t = 0;
        for (float& v : values) {
            v = std::sin(_t);
            _t += 0.05f;
        }
        t = _t;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui の新しいフレームを開始
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- 内容 ---
        ImGui::Begin("Test Window");
        ImGui::Text("Hello, ImGui!");
        ImGui::End();
        // ----------------------------------

        t += 0.05f;
        values[offset] = std::sin(t);
        offset = (offset + 1) % values.size();

        ImGui::Begin("Realtime Wave");

        ImGui::PlotLines(
            "Waveform",
            values.data(),
            static_cast<int>(values.size()),
            static_cast<int>(offset),
            "sin(t)",
            -1.0f,
            1.0f,
            ImVec2(0, 100)
        );

        ImGui::End();

        // レンダリング処理
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0, 0.447, 0.753, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 4. クリーンアップ
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}