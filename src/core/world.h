#pragma once

#include "window/input_controller.h"


class GUIScene;


class World : public InputController
{
 public:
    World();
    virtual ~World() {}
    virtual void Init() {}
    virtual void FrameStart() {}
    virtual void Update(float deltaTimeSeconds) {}
    virtual void FrameEnd() {}

    void Run();
    void Pause();
    void Exit();

    double GetLastFrameTime();

 private:
    void ComputeFrameDeltaTime();
    void LoopUpdate();

 private:
    double previousTime;
    double elapsedTime;
    double deltaTime;
    bool paused;
    bool shouldClose;

    // Set while running if this world is also a `GUIScene`
    GUIScene *guiScene;
};
