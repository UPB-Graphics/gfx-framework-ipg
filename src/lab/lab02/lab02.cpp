#include "lab/lab02/lab02.h"

#include <vector>

using namespace std;
using namespace lab;

// The scene of the lab. The editor (DrawUserInterface, UploadMesh) is in
// editor/lab02_user_interface.cpp.

Lab02::Lab02()
{
    // The canvas keeps its size and is stretched over the window
    followWindowSize = false;
}

std::string Lab02::GetShaderPath() const
{
    return PATH_JOIN(RESOURCE_PATH::SHADERS, "rasterizer", "Lab02.comp.glsl");
}

void Lab02::Initialize()
{
    {
        vertices = {
            { glm::vec3(290, 90, 0.5), glm::vec3(1, 0, 0) },
            { glm::vec3(1099, 450, 0.5), glm::vec3(0, 1, 0) },
            { glm::vec3(650, 719, 0.5), glm::vec3(0, 0, 1) },

            { glm::vec3(200, 450, 0), glm::vec3(0, 1, 1) },
            { glm::vec3(830, 719, 1), glm::vec3(1, 1, 0) },
            { glm::vec3(1099, 0, 1), glm::vec3(1, 0, 1) },
        };

        triangles = {
            { 0, 1, 2 },
            { 3, 4, 5 },
        };
    }

    CreateMesh("triangles", vertices, triangles, MAX_VERTICES, MAX_TRIANGLES);
    CreateMesh("circle", {}, {}, CIRCLE_MAX_VERTICES, CIRCLE_MAX_TRIANGLES);
}

// A fully saturated colour for `hue` in [0, 1)
static glm::vec3 Hue(const float hue)
{
    const glm::vec3 k = glm::abs(glm::fract(glm::vec3(hue) + glm::vec3(0.f, 2.f / 3.f, 1.f / 3.f)) * 6.f - 3.f);
    return glm::clamp(k - 1.f, 0.f, 1.f);
}

void Lab02::BuildCircle(const glm::vec2 center, const float radius, const int segments)
{
    circle_vertices.clear();
    circle_triangles.clear();

    // TODO(student): BONUS - Fill circle_vertices and circle_triangles with a circle
    // made of `segments` triangles, around `center`
}

void Lab02::Draw(float deltaTimeSeconds)
{
    // Apply the edits the UI made last frame
    if (circle_dirty) {
        BuildCircle(glm::vec2(CANVAS_SIZE) * 0.5f, 280.f, static_cast<int>(circle_segments));
        circle_dirty = false;
        mesh_dirty = true;
    }

    if (mesh_dirty) {
        UploadMesh();
        mesh_dirty = false;
    }

    RasterizeMesh(show_circle ? "circle" : "triangles");
}
