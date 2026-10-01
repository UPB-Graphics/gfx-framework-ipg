#include "lab/lab10/lab10.h"

#include <vector>
#include <iostream>

using namespace std;
using namespace lab;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Lab10::Lab10()
{
}


Lab10::~Lab10()
{
}


void Lab10::Init()
{
    auto camera = GetSceneCamera();
    camera->SetPosition(glm::vec3(0, 2, 5));
    camera->Update();

    {
        Texture2D* texture = new Texture2D();
        texture->Load2D("assets/models/characters/archer/Akai_E_Espiritu.fbm/akai_diffuse.png");
        mapTextures["archer"] = texture;
    }

    // Load a mesh from file into GPU memory
    {
        Mesh* mesh = new Mesh("archer");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "characters", "archer"), "Archer.fbx");
        meshes[mesh->GetMeshID()] = mesh;
    }

    // Create a simple triangle
    {
        vector<VertexFormat> vertices
        {
            VertexFormat(glm::vec3(-1.0f, 0.0f, 0.0f)),
            VertexFormat(glm::vec3(1.0f, 0.0f, 0.0f)),
            VertexFormat(glm::vec3(0.0f, 1.5f, 0.0f)),
        };

        vector<unsigned int> indices =
        {
            0, 1, 2
        };

        Mesh* mesh = new Mesh("triangle");
        mesh->InitFromData(vertices, indices);
        meshes[mesh->GetMeshID()] = mesh;
    }

    // TODO(student): Load other shaders. You can reuse a file for a
    // certain type of shader for several programs.
    LoadShader("LabShader", "WorldSpace", "Texture", "LabShader");
    LoadShader("Triangle", "WorldSpace", "Color", "Triangle");
}

void Lab10::FrameStart()
{
    ClearScreen(glm::vec3(1, 1, 0.81));
}

void Lab10::Update(float deltaTimeSeconds)
{
    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-3, 0, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01));
        RenderSimpleMesh(meshes["archer"], shaders["LabShader"], modelMatrix, mapTextures["archer"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-2, 0, 2));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01));
        RenderSimpleMesh(meshes["archer"], shaders["LabShader"], modelMatrix, mapTextures["archer"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(2, 0, 2));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01));
        RenderSimpleMesh(meshes["archer"], shaders["LabShader"], modelMatrix, mapTextures["archer"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 1, 1));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5));
        RenderSimpleMesh(meshes["triangle"], shaders["LabShader"], modelMatrix);
    }
}


void Lab10::FrameEnd()
{
    DrawCoordinateSystem();
}


void Lab10::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, Texture2D* texture)
{
    if (!mesh || !shader || !shader->GetProgramID())
        return;

    // Render an object using the specified shader and the specified position
    glUseProgram(shader->program);

    // Bind model matrix
    GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
    glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // Bind view matrix
    glm::mat4 viewMatrix = GetSceneCamera()->GetViewMatrix();
    int loc_view_matrix = glGetUniformLocation(shader->program, "View");
    glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    // Bind projection matrix
    glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
    int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
    glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // TODO(student): Set any other shader uniforms that you need

    if (texture) {
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, texture->GetTextureID());

        glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);
    }

    // Draw the object
    glBindVertexArray(mesh->GetBuffers()->GetVAO());
    glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}

void Lab10::LoadShader(const std::string& name, const std::string &VS, const std::string& FS, const std::string& GS)
{
    std::string shaderPath = PATH_JOIN(window->props.selfDir, "src/lab", "lab10", "shaders");

    // Create a shader program for particle system
    {
        Shader *shader = new Shader(name);
        shader->AddShader(PATH_JOIN(shaderPath, VS + ".VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, FS + ".FS.glsl"), GL_FRAGMENT_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, GS + ".GS.glsl"), GL_GEOMETRY_SHADER);

        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }
}

/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Lab10::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input with window->KeyHold()
}


void Lab10::OnKeyPress(int key, int mods)
{
    // Add key press event
}


void Lab10::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab10::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab10::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab10::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab10::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Lab10::OnWindowResize(int width, int height)
{
    // Treat window resize event
}
