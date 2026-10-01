#pragma once

#include <vector>

#include "components/rasterizer_scene.h"
#include "core/gui.h"
#include "lab/lab03/transform2D.h"
#include "lab/lab04/transform3D.h"

namespace lab
{
    using gfxc::Vertex;
    using gfxc::Triangle;

    // Which face of the triangles is not drawn
    enum CULL_FACE_OPTION
    {
        BACK_FACES = 0,
        FRONT_FACES = 1,
        NO_FACES = 2,
        BOTH_FACES = 3,
        COUNT
    };

    class Lab04 : public gfxc::RasterizerScene, public GUIScene
    {
     public:
        void DrawUserInterface() override;

        static constexpr int TETRAHEDRON_MAX_VERTICES = 16;
        static constexpr int TETRAHEDRON_MAX_TRIANGLES = 16;

        void BuildTetrahedron();

     protected:
        std::string GetShaderPath() const override;
        void Initialize() override;
        void Draw(float deltaTimeSeconds) override;

     private:
        void OnKeyPress(int key, int mods) override;

        glm::mat4 ModelTransformation() const;

        struct Matrices {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat3 viewport;
        };
        Matrices ComputeMatrices() const;
        void SetMatrices(const Shader *shader, const Matrices &matrices) const;

        void Inspect(const Matrices &matrices);

        std::string ShownMesh() const;
        const std::vector<Vertex> &ShownVertices() const;
        const std::vector<Triangle> &ShownTriangles() const;

     private:
        std::vector<Vertex> vertices;
        std::vector<Triangle> triangles;

        std::vector<Vertex> tetrahedron_vertices;
        std::vector<Triangle> tetrahedron_triangles;
        bool show_tetrahedron = false;

        struct InspectedVertex {
            glm::vec4 world;
            glm::vec4 camera;
            glm::vec4 clip;
            glm::vec4 screen;
        };
        std::vector<InspectedVertex> inspected_vertices;
        std::vector<int> inspected_faces;    ///< BACK (0), FRONT (1), NONE (10), or -1 behind the camera

        glm::vec3 mesh_position { 0.f, 1.f, -3.f };
        glm::vec3 mesh_rotation { 45.f, 45.f, 45.f };
        glm::vec3 mesh_scale { 1.25f };
        bool spin = false;

        float fov = 60.f;       ///< vertical, in degrees
        float z_near = 0.1f;
        float z_far = 100.f;

        CULL_FACE_OPTION cull_face_option = NO_FACES;
        bool perspective_divide = true;
        bool depth_test = true;

        bool show_wireframe = true;
        bool show_winding = true;
        bool show_vertices = true;
        int selected_vertex = -1;    ///< pinned by a click on its handle; -1 is none
        int hovered_vertex = -1;

        bool editor_open = true;
        int grab = 0;               ///< the control being dragged (see ui_utils.h)
    };
}   // namespace lab
