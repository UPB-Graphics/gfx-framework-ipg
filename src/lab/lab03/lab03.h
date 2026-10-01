#pragma once

#include <vector>

#include "components/rasterizer_scene.h"
#include "core/gui.h"
#include "lab/lab03/transform2D.h"

namespace lab
{
    using gfxc::Vertex;
    using gfxc::Triangle;

    class Lab03 : public gfxc::RasterizerScene, public GUIScene
    {
     public:
        void DrawUserInterface() override;

        static constexpr int STAR_MIN_RAYS = 3;
        static constexpr int STAR_MAX_RAYS = 64;
        static constexpr int STAR_MAX_VERTICES = 16;
        static constexpr int STAR_MAX_TRIANGLES = 16;

        void BuildStarRay();
        void BuildStarTransforms(int rays);

     protected:
        std::string GetShaderPath() const override;
        void Initialize() override;
        void Draw(float deltaTimeSeconds) override;
        void OnTargetResize() override;

     private:
        void OnInputUpdate(float deltaTime, int mods) override;

        void CreateViewports();
        void AddViewport(const transform2D::ViewportSpace &viewport_space);
        void CreateTransforms();

     private:
        struct Viewport {
            transform2D::ViewportSpace space;
            glm::mat3 matrix;
        };

        transform2D::LogicSpace logic_space { 0.f, 0.f, 16.f, 9.f };

        std::vector<Viewport> viewports;
        std::vector<glm::mat3> transforms;

        std::vector<Vertex> star_vertices;
        std::vector<Triangle> star_triangles;
        std::vector<glm::mat3> star_transforms;

        bool editor_open = true;
        int grab = 0;

        bool show_star = false;
        float star_rays = 8.f;
        bool star_dirty = true;

        bool grid_enabled = true;
        bool grid_labels = true;
        bool grid_readout = true;
        int grid_cell = 2;       ///< index into the cell-size presets the - / + buttons step through
    };
}   // namespace lab
