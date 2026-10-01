#include "core/window/window_callbacks.h"

#include <iostream>

#include "core/engine.h"
#include "core/gui.h"


// ImGui chains to these callbacks, so they see every event. Presses and
// scrolls that land on the UI are not forwarded to the scene; releases always
// are, so the scene never believes a key or button is stuck down.


void WindowCallbacks::KeyCallback(GLFWwindow *W, int key, int scanCode, int action, int mods)
{
    if (action != GLFW_RELEASE && gui::WantsKeyboard())
        return;

    Engine::GetWindow()->KeyCallback(key, scanCode, action, mods);
}


void WindowCallbacks::CursorMove(GLFWwindow *W, double posX, double posY)
{
    Engine::GetWindow()->MouseMove((int)posX, (int)posY);
}


void WindowCallbacks::MouseClick(GLFWwindow *W, int button, int action, int mods)
{
    if (action == GLFW_PRESS && gui::WantsMouse())
        return;

    Engine::GetWindow()->MouseButtonCallback(button, action, mods);
}


void WindowCallbacks::MouseScroll(GLFWwindow * W, double offsetX, double offsetY)
{
    if (gui::WantsMouse())
        return;

    Engine::GetWindow()->MouseScroll(offsetX, offsetY);
}


void WindowCallbacks::OnClose(GLFWwindow * W)
{
    Engine::GetWindow()->Close();
}


void WindowCallbacks::OnResize(GLFWwindow *W, int width, int height)
{
    Engine::GetWindow()->SetSize(width, height);
}


void WindowCallbacks::OnError(int error, const char * description)
{
    std::cout << "[GLFW ERROR]\t" << error << "\t" << description << std::endl;
}
