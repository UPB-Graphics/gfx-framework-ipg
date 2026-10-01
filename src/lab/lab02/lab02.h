#pragma once

#include <vector>

#include "components/rasterizer_scene.h"
#include "core/gui.h"

namespace lab
{
    using gfxc::Vertex;
    using gfxc::Triangle;

    class Lab02 : public gfxc::RasterizerScene, public GUIScene
    {
     public:
        Lab02();

        void DrawUserInterface() override;

        static constexpr glm::uvec2 CANVAS_SIZE { 1280, 720 };

        static constexpr int MAX_VERTICES = 512;
        static constexpr int MAX_TRIANGLES = 256;

        static constexpr int CIRCLE_MIN_SEGMENTS = 3;
        static constexpr int CIRCLE_MAX_SEGMENTS = 128;
        static constexpr int CIRCLE_MAX_VERTICES = CIRCLE_MAX_SEGMENTS + 1;
        static constexpr int CIRCLE_MAX_TRIANGLES = CIRCLE_MAX_SEGMENTS;

        void BuildCircle(glm::vec2 center, float radius, int segments);

     protected:
        std::string GetShaderPath() const override;
        void Initialize() override;
        void Draw(float deltaTimeSeconds) override;

     private:
        void UploadMesh();

     private:
        std::vector<Vertex> vertices;
        std::vector<Triangle> triangles;

        std::vector<Vertex> circle_vertices;
        std::vector<Triangle> circle_triangles;

        bool editor_open = true;
        enum class Tool { Select, AddTriangle };
        Tool tool = Tool::Select;
        int selected_vertex = -1;
        int dragging_vertex = -1;
        int popup_vertex = -1;
        std::vector<glm::uint> pending_corners;
        int grab = 0;
        bool mesh_dirty = false;

        bool show_circle = false;
        float circle_segments = 24.f;
        bool circle_dirty = true;
    };
}   // namespace lab
