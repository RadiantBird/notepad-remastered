cmake_minimum_required(VERSION 3.14)
project(ImguiTest C CXX)

set(CMAKE_CXX_STANDARD 17)

set(GL_DIR "C:/CPHL/GL")
set(GLFW_DIR "C:/CPHL/GLFW")
set(LIBS_DIR "C:/CPHL/Libs")
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imguiSources/imgui")

file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/*.cpp"
    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

add_library(imgui_lib STATIC ${IMGUI_SOURCES})

# PUBLIC にしておくことで、このライブラリをリンクする側(exe)にもインクルードパスが自動で引き継がれる
target_include_directories(imgui_lib PUBLIC
    "C:/CPHL"
    "${GL_DIR}/src"
    "${GLFW_DIR}/src"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    "${IMGUI_DIR}/misc/cpp"
)

target_link_directories(imgui_lib PUBLIC "${LIBS_DIR}")

target_link_libraries(imgui_lib PUBLIC
    glfw3.lib
    opengl32
    gdi32
    winmm
    dwmapi.lib
    ole32
    shell32
)

add_executable(aeroUI "test1/main.cpp")
target_link_libraries(aeroUI PRIVATE imgui_lib)

add_executable(notepad_remastered "source/main.cpp")
target_link_libraries(notepad_remastered PRIVATE imgui_lib)