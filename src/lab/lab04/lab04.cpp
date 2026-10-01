#include "lab/lab04/lab04.h"

#include <algorithm>
#include <vector>

#include "components/transform.h"

// The scene of the lab. The interface (DrawUserInterface) is in editor/lab04_user_interface.cpp.

using namespace std;
using namespace lab;

std::string Lab04::GetShaderPath() const
{
    return PATH_JOIN(RESOURCE_PATH::SHADERS, "rasterizer", "Lab04.comp.glsl");
}

void Lab04::Initialize()
{
    {
        GetCameraInput()->SetActive(true);

        gfxc::Camera *camera = GetSceneCamera();
        camera->SetPositionAndRotation(glm::vec3(0, 1, 0), glm::quatLookAt(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)));
        camera->Update();
    }

    {
        vertices = {
            { glm::vec3(-0.5, -0.5,  0.5), glm::vec3(1, 0, 0) },
            { glm::vec3( 0.5, -0.5,  0.5), glm::vec3(0, 1, 0) },
            { glm::vec3(-0.5,  0.5,  0.5), glm::vec3(0, 0, 1) },
            { glm::vec3( 0.5,  0.5,  0.5), glm::vec3(0, 1, 1) },
            { glm::vec3(-0.5, -0.5, -0.5), glm::vec3(1, 1, 0) },
            { glm::vec3( 0.5, -0.5, -0.5), glm::vec3(1, 0, 1) },
            { glm::vec3(-0.5,  0.5, -0.5), glm::vec3(1, 1, 1) },
            { glm::vec3( 0.5,  0.5, -0.5), glm::vec3(0, 0, 0) },
        };

        triangles = {
            { 0, 1, 2 }, { 1, 3, 2 },
            { 2, 3, 7 }, { 2, 7, 6 },
            { 1, 7, 3 }, { 1, 5, 7 },
            { 6, 7, 4 }, { 7, 5, 4 },
            { 0, 4, 1 }, { 1, 4, 5 },
            { 2, 6, 4 }, { 0, 2, 4 },
        };

        CreateMesh("cube", vertices, triangles);
    }

    {
        BuildTetrahedron();
        CreateMesh("tetrahedron", tetrahedron_vertices, tetrahedron_triangles, TETRAHEDRON_MAX_VERTICES, TETRAHEDRON_MAX_TRIANGLES);
    }

    // Large enough for whichever mesh is shown
    const auto maxVertices = static_cast<unsigned int>(std::max<size_t>(vertices.size(), TETRAHEDRON_MAX_VERTICES));
    const auto maxTriangles = static_cast<unsigned int>(std::max<size_t>(triangles.size(), TETRAHEDRON_MAX_TRIANGLES));

    CreateComputeShader("inspector", PATH_JOIN(RESOURCE_PATH::SHADERS, "rasterizer", "Lab04Inspect.comp.glsl"));
    buffers["inspected_vertices"] = new TypedBuffer<InspectedVertex>(GL_SHADER_STORAGE_BUFFER, maxVertices);
    buffers["inspected_faces"] = new TypedBuffer<int>(GL_SHADER_STORAGE_BUFFER, maxTriangles);
}

void Lab04::BuildTetrahedron()
{
    tetrahedron_vertices.clear();
    tetrahedron_triangles.clear();

    // TODO(student): BONUS - A tetrahedron, with the vertices of every triangle in counterclockwise
    // order when seen from outside, and a different color for each vertex
}

std::string Lab04::ShownMesh() const
{
    return show_tetrahedron ? "tetrahedron" : "cube";
}

const std::vector<Vertex> &Lab04::ShownVertices() const
{
    return show_tetrahedron ? tetrahedron_vertices : vertices;
}

const std::vector<Triangle> &Lab04::ShownTriangles() const
{
    return show_tetrahedron ? tetrahedron_triangles : triangles;
}

glm::mat4 Lab04::ModelTransformation() const
{
    glm::mat4 transformation = glm::mat4(1);

    transformation *= transform3D::Translate(mesh_position.x, mesh_position.y, mesh_position.z);
    transformation *= transform3D::RotateOZ(glm::radians(mesh_rotation.z));
    transformation *= transform3D::RotateOY(glm::radians(mesh_rotation.y));
    transformation *= transform3D::RotateOX(glm::radians(mesh_rotation.x));
    transformation *= transform3D::Scale(mesh_scale.x, mesh_scale.y, mesh_scale.z);

    return transformation;
}

Lab04::Matrices Lab04::ComputeMatrices() const
{
    const gfxc::Transform *camera = GetSceneCamera()->m_transform;
    const float aspect = static_cast<float>(targetSize.x) / static_cast<float>(targetSize.y);

    Matrices matrices;
    matrices.model = ModelTransformation();
    matrices.view = transform3D::View(
        camera->GetWorldPosition(),
        camera->GetLocalOZVector(),
        camera->GetLocalOXVector(),
        camera->GetLocalOYVector());
    matrices.projection = transform3D::Perspective(glm::radians(fov), aspect, z_near, z_far);
    matrices.viewport = transform2D::Viewport(
        transform2D::LogicSpace(-1, -1, 2, 2),
        transform2D::ViewportSpace(0, 0, targetSize.x, targetSize.y));

    return matrices;
}

void Lab04::SetMatrices(const Shader *shader, const Matrices &matrices) const
{
    shader->SetUniform("model", matrices.model);
    shader->SetUniform("view", matrices.view);
    shader->SetUniform("projection", matrices.projection);
    shader->SetUniform("viewport", matrices.viewport);
    shader->SetUniform("perspective_divide", perspective_divide ? 1 : 0);
}

void Lab04::Draw(float deltaTimeSeconds)
{
    if (spin) {
        mesh_rotation.x = glm::mod(mesh_rotation.x + 20.f * deltaTimeSeconds + 180.f, 360.f) - 180.f;
    }

    const Matrices matrices = ComputeMatrices();

    // Culling both faces leaves nothing to draw
    if (cull_face_option != BOTH_FACES) {
        const Shader *shader = GetRasterizer();
        SetMatrices(shader, matrices);
        shader->SetUniform("cull_face", cull_face_option);
        shader->SetUniform("depth_test", depth_test ? 1 : 0);

        RasterizeMesh(ShownMesh());
    }

    Inspect(matrices);
}

void Lab04::Inspect(const Matrices &matrices)
{
    const Shader *shader = shaders["inspector"];
    if (!shader->GetProgramID()) {
        return;
    }
    shader->Use();

    const std::vector<Vertex> &vertices = ShownVertices();
    const std::vector<Triangle> &triangles = ShownTriangles();

    SetMatrices(shader, matrices);
    shader->SetUniform("vertex_count", static_cast<unsigned int>(vertices.size()));
    shader->SetUniform("triangle_count", static_cast<unsigned int>(triangles.size()));

    BindMesh(ShownMesh());
    buffers["inspected_vertices"]->BindBase(4);
    buffers["inspected_faces"]->BindBase(5);

    const auto count = static_cast<unsigned int>(std::max(vertices.size(), triangles.size()));
    glDispatchCompute((count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    inspected_vertices.resize(vertices.size());
    inspected_faces.resize(triangles.size());
    buffers["inspected_vertices"]->GetData(inspected_vertices.data(), inspected_vertices.size() * sizeof(InspectedVertex));
    buffers["inspected_faces"]->GetData(inspected_faces.data(), inspected_faces.size() * sizeof(int));
}

void Lab04::OnKeyPress(int key, int mods)
{
    if (key == GLFW_KEY_F) {
        cull_face_option = static_cast<CULL_FACE_OPTION>((cull_face_option + 1) % CULL_FACE_OPTION::COUNT);
    }
}
