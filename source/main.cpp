#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <shobjidl.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <functional>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void SetTransparentTitleBar()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_TitleBg].w = 0.0f;
    style.Colors[ImGuiCol_TitleBgActive].w = 0.0f;
    style.Colors[ImGuiCol_TitleBgCollapsed].w = 0.0f;
}

// ImGui スタイル設定
static void SetupImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 5.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);

    style.Colors[ImGuiCol_Text]             = ImColor(0, 0, 0, 255);
    style.Colors[ImGuiCol_InputTextCursor]  = ImColor(0, 0, 0, 255);
    style.Colors[ImGuiStyleVar_FramePadding]= ImColor(0, 0, 0, 0);
    style.Colors[ImGuiCol_PopupBg]          = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_MenuBarBg]        = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_FrameBg]          = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_WindowBg]         = ImColor(255, 255, 255, 127);
    style.Colors[ImGuiCol_TitleBgActive]    = ImColor(0, 189, 229, 255);
    style.Colors[ImGuiCol_TitleBg]          = ImColor(0, 98, 175, 127);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(0, 98, 122, 127);

    SetTransparentTitleBar();
}

// 初期化処理（GLFW / Window / DWM / ImGui）
static GLFWwindow* InitApp(int width, int height, const char* title) {
    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 親ウィンドウは非表示

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.FontNo = 2; // 0: ＭＳ ゴシック, 1: ＭＳ Ｐゴシック, 2: ＭＳ ＵUI Gothic

    // MS UI Gothic を指定して読み込み
    ImFont* _font = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/msgothic.ttc", 
        20.0f, 
        &config, 
        io.Fonts->GetGlyphRangesJapanese()
    );

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    SetupImGuiStyle();

    // バックエンドの初期化（ここで標準の Platform_CreateWindow フックがセットされる）
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- サブウィンドウ（ビューポート）生成フックのラップ ---
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

        // バックエンドがセットした元の作成関数を保持
        static void (*prev_create_window)(ImGuiViewport* vp) = platform_io.Platform_CreateWindow;

        platform_io.Platform_CreateWindow = [](ImGuiViewport* vp) {
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
            glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

            if (prev_create_window) {
                prev_create_window(vp);
                GLFWwindow* w = (GLFWwindow*)vp->PlatformHandle;
                // printf("transparent = %d\n", glfwGetWindowAttrib(w, GLFW_TRANSPARENT_FRAMEBUFFER));
            }
        };

        static void (*prev_render_window)(ImGuiViewport*, void*) = platform_io.Renderer_RenderWindow;

        platform_io.Renderer_RenderWindow = [](ImGuiViewport* vp, void* render_arg) {
            vp->Flags |= ImGuiViewportFlags_NoRendererClear; // バックエンドの黒クリアを抑止
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);            // 自前で透明クリア
            glClear(GL_COLOR_BUFFER_BIT);
            if (prev_render_window) prev_render_window(vp, render_arg);
        };
    }

    return window;
}

// 終了・リソース解放
static void CleanupApp(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

std::wstring OpenFileDialog(HWND hwndOwner = NULL) {
    std::wstring filePath = L"";

    IFileOpenDialog* pFileOpen = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                                  IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Text Documents (*.txt)", L"*.txt" },
            { L"All Files (*.*)", L"*.*" }
        };
        pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);

        // 親ウィンドウハンドルを指定して表示
        hr = pFileOpen->Show(hwndOwner);

        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr)) {
                    filePath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    return filePath;
}

std::wstring SaveFileDialog(HWND hwnd, const std::wstring& defaultFileName = L"No name.txt") {
    std::wstring filePath = L"";
    IFileSaveDialog* pFileSave = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_FileSaveDialog, 
        NULL, 
        CLSCTX_ALL, 
        IID_IFileSaveDialog, 
        reinterpret_cast<void**>(&pFileSave)
    );

    if (SUCCEEDED(hr)) {
        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Text Documents (*.txt)", L"*.txt" },
            { L"All Files (*.*)", L"*.*" }
        };
        pFileSave->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
        pFileSave->SetDefaultExtension(L"txt");

        if (!defaultFileName.empty()) {
            pFileSave->SetFileName(defaultFileName.c_str());
        }

        hr = pFileSave->Show(hwnd);

        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    filePath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }

    return filePath;
}

std::string ReadFileText(const std::wstring& filePath) {
    if (filePath.empty()) return "";

    // filesystem::path を使うことで日本語パスのファイル開けない問題を回避
    std::ifstream file(std::filesystem::path(filePath), std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool WriteFileText(const std::wstring& filePath, const std::string& content) {
    if (filePath.empty()) return false;

    std::ofstream file(std::filesystem::path(filePath), std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    file.write(content.data(), content.size());
    file.close();
    return !file.fail();
}

class UndoHistory {
private:
    std::vector<std::string> history;
    int index = -1;
    size_t maxHistorySize = 100; // 保持する履歴の最大数

public:
    UndoHistory() = default;

    // 初期状態の登録（ファイル読み込み時など）
    void Clear(const std::string& currentText) {
        history.clear();
        history.push_back(currentText);
        index = 0;
    }

    // テキスト変更時に履歴を追加
    void Push(const std::string& newText) {
        if (index >= 0 && history[index] == newText) return; // 変化がなければ無視

        // 現在地より先のRedo履歴を削除
        if (index + 1 < (int)history.size()) {
            history.erase(history.begin() + index + 1, history.end());
        }

        history.push_back(newText);

        // 最大件数を超えたら古いものを削除
        if (history.size() > maxHistorySize) {
            history.erase(history.begin());
        }

        index = (int)history.size() - 1;
    }

    // Undo実行
    bool CanUndo() const { return index > 0; }
    std::string Undo() {
        if (CanUndo()) {
            index--;
        }
        return history[index];
    }

    // Redo実行
    bool CanRedo() const { return index + 1 < (int)history.size(); }
    std::string Redo() {
        if (CanRedo()) {
            index++;
        }
        return history[index];
    }
};

std::string  currentTitle = "No name.txt - Notepad Remastered###window";
std::wstring currentFile  = L"";
std::string  textfield    = "";

bool isFileDirty = false;
UndoHistory undoHistory;

std::vector<std::string> split(const std::string& txt, const std::string& dem) {
    std::vector<std::string> result;
    std::string buff;

    if (dem.empty()) {
        return {txt};
    }

    size_t idx = 0;

    while (idx < txt.size()) {
        if (
            idx + dem.size() <= txt.size() &&
            txt.compare(idx, dem.size(), dem) == 0
        ) {
            result.push_back(buff);
            buff.clear();
            idx += dem.size();
        }
        else {
            buff += txt[idx];
            ++idx;
        }
    }

    result.push_back(buff);
    return result;
}

void SetTitle(const std::wstring& path, bool dirty = false) {
    std::string filename = std::filesystem::path(path).filename().string();
    std::string unsavedSymbol = "";
    if (filename.empty()) {
        filename = "No name.txt";
    }
    if (dirty) {
        unsavedSymbol = "* ";
    }
    currentTitle = unsavedSymbol + filename + " - Notepad Remastered###window";
}

namespace menucmd {
    void UpdateFileState(bool isDirty) {
        isFileDirty = isDirty;
        SetTitle(currentFile, isDirty);
    }

    // --- 確認モーダルの状態（フレームをまたいで保持）---
    static bool                  requestConfirm = false;
    static std::string           confirmMessage;
    static std::function<void()> confirmAction;

    static bool                  requestCheck = false;
    static std::string           checkMessage;
    static std::function<void()> checkAction;

    void RequestConfirm(const std::string& message, std::function<void()> onYes) {
        confirmMessage = message;
        confirmAction  = std::move(onYes);
        requestConfirm = true;
    }

    void DrawConfirmModal() {
        if (requestConfirm) {
            ImGui::OpenPopup("UserControl");
            requestConfirm = false;
        }

        bool yes = false;
        if (ImGui::BeginPopupModal("UserControl", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(confirmMessage.c_str());  // Text() はフォーマット文字列扱いになるので避ける
            ImGui::Separator();

            if (ImGui::Button("Yes", ImVec2(120, 0))) {
                yes = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // モーダルを閉じた後に実行（ファイルダイアログが出る処理もあるため）
        if (yes && confirmAction) {
            auto action = std::move(confirmAction);
            confirmAction = nullptr;
            action();
        }
    }

    void RequestCheck(const std::string& message) {
        checkMessage = message;
        requestCheck = true;
    }

    void DrawCheckModal() {
        if (requestCheck) {
            ImGui::OpenPopup("Error :(");
            requestCheck = false;
        }

        if (ImGui::BeginPopupModal("Error :(", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(checkMessage.c_str());  // Text() はフォーマット文字列扱いになるので避ける
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
    }

    static void DoNew() {
        textfield   = "";
        currentFile = L"";                // これがないと New 後の Ctrl+S が前のファイルを上書きする
        undoHistory.Clear(textfield);
        UpdateFileState(false);
    }

    static void DoOpen(HWND hwnd) {
        std::wstring path = OpenFileDialog(hwnd);
        if (path.empty()) return;
        textfield   = ReadFileText(path);
        currentFile = path;
        undoHistory.Clear(textfield);
        UpdateFileState(false);
    }

    // static void DoSaveCleanUp() {

    // }

    // --- メニュー／ショートカットから呼ばれる入口 ---
    void New() {
        if (isFileDirty) {
            RequestConfirm("Are you sure to create new file without saving?", [] { DoNew(); });
        } else {
            DoNew();
        }
    }

    void Open(HWND hwnd) {
        if (isFileDirty) {
            RequestConfirm("Are you sure to open other file without saving?", [hwnd] { DoOpen(hwnd); });
        } else {
            DoOpen(hwnd);
        }
    }

    void Save(HWND hwnd) {
        std::wstring path = currentFile;
        if (currentFile == L"") {
            path = SaveFileDialog(hwnd);
        }

        // ユーザーがファイルを指定しなかった場合は空の可能性があるため
        if (path == L"") {
            return;
        }
        
        bool success = WriteFileText(path, textfield);
        if (!success) {
            RequestCheck("Could not save your file.");
            return;
        }
        currentFile = path;
        UpdateFileState(false);
    }

    void Undo(bool enabled) {
        if (!enabled) {
            return;
        }
        textfield = undoHistory.Undo();
        UpdateFileState(true);
    }

    void Redo(bool enabled) {
        if (!enabled) {
            return;
        }
        textfield = undoHistory.Redo();
        UpdateFileState(true);
    }
} // namespace menucmd

namespace userinterface {
void handleShortcuts(HWND hwnd, float& editorFontScale, bool& open) {
    const ImGuiInputFlags routeFlags = ImGuiInputFlags_RouteGlobal;
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsPopupOpen("UserControl") || ImGui::IsPopupOpen("Error :(") || ImGui::IsPopupOpen("ConfirmClose")) return;

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus, routeFlags)) {
        editorFontScale -= 0.1f;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Equal, routeFlags) ||
        ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Semicolon, routeFlags) ||
        ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd, routeFlags)) {
        editorFontScale += 0.1f;
    }

    if (io.KeyCtrl && io.MouseWheel != 0.0f) {
        editorFontScale += io.MouseWheel * 0.1f;
        io.MouseWheel = 0.0f;   // これ以降のスクロール処理に渡さない
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, routeFlags)) {
        menucmd::Undo(undoHistory.CanUndo());
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, routeFlags)) {
        menucmd::Redo(undoHistory.CanRedo());
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, routeFlags)) {
        menucmd::New();
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, routeFlags)) {
        menucmd::Open(hwnd);
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, routeFlags)) {
        menucmd::Save(hwnd);
    }

    if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_F4, routeFlags)) {
        open = false;
    }

    editorFontScale = std::clamp(editorFontScale, 0.3f, 5.0f);
}

void handleTopMenu(HWND hwnd, bool& open) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                menucmd::New();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                menucmd::Open(hwnd);
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                menucmd::Save(hwnd);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                open = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, undoHistory.CanUndo())) {
                menucmd::Undo(true);
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, undoHistory.CanRedo())) {
                menucmd::Redo(true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

} // namespace userinterface

bool drawContent(HWND hwnd) {
    bool shouldExit = false;
    bool open = true;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;

    if (!ImGui::Begin(currentTitle.c_str(), &open, flags)) {
        ImGui::End();
        if (!open) shouldExit = true;
        return shouldExit;
    }

    static float editorFontScale = 1.0f;

    userinterface::handleShortcuts(hwnd, editorFontScale, open);

    userinterface::handleTopMenu(hwnd, open);
    
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImFont* font = ImGui::GetFont();
    font->Scale = editorFontScale;
    ImGui::PushFont(font);

    if (ImGui::InputTextMultiline("##longpaper", &textfield, avail, ImGuiInputTextFlags_NoUndoRedo)) {
        undoHistory.Push(textfield);
        menucmd::UpdateFileState(true);
    }

    font->Scale = 1.0f;
    ImGui::PopFont();

    menucmd::DrawConfirmModal();
    menucmd::DrawCheckModal();

    if (!open) {
        if (!isFileDirty) {
            shouldExit = true;
            goto UI_END;
        }
        ImGui::OpenPopup("ConfirmClose");
        open = true;
    }

    if (ImGui::BeginPopupModal("ConfirmClose", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Do you want to close notepad without saving?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            shouldExit = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            shouldExit = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    UI_END:
    ImGui::End();
    
    return shouldExit;
}

int main() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    GLFWwindow* window = InitApp(1, 1, "Dummy");
    if (!window) return -1;
    glfwHideWindow(window);

    undoHistory.Clear(textfield);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        HWND hwnd = glfwGetWin32Window(window);
        bool shouldExit = drawContent(hwnd);

        ImGui::Render();

        if (shouldExit) {
            // ループを抜ける前にプラットフォーム処理を完結させる
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            break;
        }

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(1.0f, 0.0f, 0.0f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // マルチビューポート有効時のプラットフォームウィンドウ更新
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    CleanupApp(window);

    CoUninitialize();
    return 0;
}