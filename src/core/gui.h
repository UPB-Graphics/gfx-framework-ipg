#pragma once

#include "imgui.h"


class WindowObject;


/*
 *  A scene that has a user interface. Inherit it alongside the scene class:
 *
 *      class MyLab : public gfxc::SimpleScene, public GUIScene
 *
 *  For such scenes the world initializes Dear ImGui when it starts running
 *  and calls `DrawUserInterface` every frame after `FrameEnd`; the UI is
 *  drawn over whatever the scene left in the default framebuffer.
 */
class GUIScene
{
 public:
    virtual ~GUIScene() = default;

    // Issue the ImGui calls of the scene (windows, draw lists, ...)
    virtual void DrawUserInterface() = 0;
};


/*
 *  Dear ImGui integration (GLFW + OpenGL 3 backends), driven by `World`.
 */
namespace gui
{
    void Init(WindowObject *window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // True when ImGui is using the mouse / keyboard (e.g. hovering one of its
    // windows), in which case the scene should not react to that input.
    // Always false while ImGui is not initialized.
    bool WantsMouse();
    bool WantsKeyboard();
}   // namespace gui
