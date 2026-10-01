#include "lab/lab03/lab03.h"

#include <vector>

// The scene of the lab. The editor (DrawUserInterface) is in editor/lab03_user_interface.cpp.

using namespace std;
using namespace lab;

std::string Lab03::GetShaderPath() const
{
    return PATH_JOIN(RESOURCE_PATH::SHADERS, "rasterizer", "Lab03.comp.glsl");
}

void Lab03::Initialize() {
    {
        vector<Vertex> vertices
        {
            { glm::vec3(0, 0, 0.5), glm::vec3(1, 0, 0) },
            { glm::vec3(0, 1, 0.5), glm::vec3(0, 1, 0) },
            { glm::vec3(1, 0, 0.5), glm::vec3(0, 0, 1) },
            { glm::vec3(1, 1, 0.5), glm::vec3(0, 1, 1) },
        };

        vector<Triangle> triangles
        {
            { 0, 1, 2 },
            { 1, 2, 3 },
        };

        CreateMesh("square", vertices, triangles);
    }

    BuildStarRay();
    CreateMesh("star", star_vertices, star_triangles, STAR_MAX_VERTICES, STAR_MAX_TRIANGLES);

    CreateViewports();
    CreateTransforms();
}

void Lab03::BuildStarRay()
{
    star_vertices.clear();
    star_triangles.clear();

    // TODO(student): BONUS - The vertices and indices of one ray of the star, a triangle
}

void Lab03::BuildStarTransforms(int rays)
{
    star_transforms.clear();

    // TODO(student): BONUS - One transformation for each of the `rays` rays, placing
    // the ray around the center of the star
}

void Lab03::AddViewport(const transform2D::ViewportSpace &viewport_space)
{
    viewports.push_back({ viewport_space, transform2D::Viewport(logic_space, viewport_space) });
}

void Lab03::CreateViewports()
{
    viewports.clear();

    // TODO(student): Ex. 4 - Divide the screen into four quadrants, with a viewport in each

    AddViewport(transform2D::ViewportSpace(0, 0, targetSize.x, targetSize.y));
}

void Lab03::CreateTransforms()
{
    transforms.clear();

    {
        glm::mat3 transformation = transform2D::Translate(1, 6);

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(4, 6);

        // TODO(student): Ex. 2 - Apply a uniform scaling transformation,
        // which halves the scale of the square. Apply the
        // transformation from the bottom-left corner of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(7, 6);

        // TODO(student): Ex. 2 - Apply a uniform scaling transformation,
        // which doubles the scale of the square. Apply the
        // transformation from the bottom-left corner of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(10, 6);

        // TODO(student): Ex. 2 - Apply a 45 degree rotation transformation
        // to the lower-left corner of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(13, 6);

        // TODO(student): Ex. 2 - Apply two transformations together, one
        // of non-uniform scaling with the scaling vector (1, 2)
        // and a rotation transformation of 45 degrees. Apply both
        // transformations to the lower left corner of the square.

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(1, 2);

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(4, 2);

        // TODO(student): Ex. 3 - Apply a uniform scaling transformation,
        // which halves the scale of the square. Apply the
        // transformation from the center of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(7, 2);

        // TODO(student): Ex. 3 - Apply a uniform scaling transformation,
        // which doubles the scale of the square. Apply the
        // transformation from the center of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(10, 2);

        // TODO(student): Ex. 3 - Apply a 45 degree rotation transformation
        // to the center of the square

        transforms.push_back(transformation);
    }

    {
        glm::mat3 transformation = transform2D::Translate(13, 2);

        // TODO(student): Ex. 3 - Apply two transformations together, one
        // of non-uniform scaling with the scaling vector (1, 2)
        // and a rotation transformation of 45 degrees. Apply both
        // transformations to the center of the square

        transforms.push_back(transformation);
    }
}

void Lab03::Draw(float deltaTimeSeconds)
{
    // Apply the edits the UI made last frame
    if (star_dirty) {
        BuildStarTransforms(static_cast<int>(star_rays));
        star_dirty = false;
    }

    const Shader *shader = GetRasterizer();
    const std::string mesh = show_star ? "star" : "square";
    const std::vector<glm::mat3> &meshTransforms = show_star ? star_transforms : transforms;

    for (const Viewport &viewport : viewports) {
        shader->SetUniform("viewport", viewport.matrix);

        for (const glm::mat3 &transform : meshTransforms) {
            shader->SetUniform("model", transform);
            RasterizeMesh(mesh);
        }
    }
}

void Lab03::OnTargetResize()
{
    CreateViewports();
}

void Lab03::OnInputUpdate(float deltaTime, int mods)
{
    const glm::vec2 before { logic_space.x, logic_space.y };

    if (window->KeyHold(GLFW_KEY_UP)) {
        logic_space.y += 9 * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_DOWN)) {
        logic_space.y -= 9 * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_RIGHT)) {
        logic_space.x += 16 * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_LEFT)) {
        logic_space.x -= 16 * deltaTime;
    }

    if (before != glm::vec2(logic_space.x, logic_space.y)) {
        CreateViewports();
    }
}
