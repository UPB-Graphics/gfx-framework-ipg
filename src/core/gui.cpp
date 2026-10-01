#include "core/gui.h"

#include <iostream>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "core/managers/resource_path.h"
#include "core/window/window_object.h"
#include "utils/gl_utils.h"


static bool initialized = false;


static void AddFonts(const std::string &selfDir)
{
    ImGuiIO &io = ImGui::GetIO();

    const std::string textFont = PATH_JOIN(selfDir, RESOURCE_PATH::FONTS, "Inter_28pt-Regular.ttf");

    // The font is rasterized large and scaled down, which keeps it crisp
    ImFontConfig config;
    config.PixelSnapH = true;
    if (io.Fonts->AddFontFromFileTTF(textFont.c_str(), 28.0f, &config) == nullptr) {
        std::cout << "[GUI] Could not load " << textFont << ", using the default font" << std::endl;
        io.Fonts->AddFontDefault();
        return;
    }

    io.FontGlobalScale = 0.55f;
}


void gui::Init(WindowObject *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // No layout file: scenes build their UI in code every frame
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;

    AddFonts(window->props.selfDir);

    // The framework has already installed its GLFW callbacks, so ImGui chains
    // to them instead of replacing them
    ImGui_ImplGlfw_InitForOpenGL(window->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    initialized = true;
}


void gui::Shutdown()
{
    if (!initialized)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}


void gui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


void gui::EndFrame()
{
    ImGui::Render();

    // The UI goes over whatever the scene left in the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGuiIO &io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)(io.DisplaySize.x * io.DisplayFramebufferScale.x),
                     (GLsizei)(io.DisplaySize.y * io.DisplayFramebufferScale.y));

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


bool gui::WantsMouse()
{
    return initialized && ImGui::GetIO().WantCaptureMouse;
}


bool gui::WantsKeyboard()
{
    return initialized && ImGui::GetIO().WantCaptureKeyboard;
}
