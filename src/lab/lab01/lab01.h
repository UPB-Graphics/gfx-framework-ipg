#pragma once

#include "components/primitives_rendering_scene.h"

namespace lab
{
    class Lab01 : public gfxc::PrimitivesRenderingScene
    {
     public:
        Lab01();
        ~Lab01();

        void Initialize() override;

     protected:
         void Exercise1();
         void Exercise2();
         void Exercise3();
         void Exercise4();
         void Exercise5();
         void Exercise6();
         void Bonus2();
    };
}   // namespace lab
