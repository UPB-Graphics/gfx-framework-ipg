#include "lab/lab08/lab08.h"

#include <vector>
#include <string>
#include <iostream>
#include <stb/stb_image.h>

using namespace std;
using namespace lab;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Lab08::Lab08()
{
}


Lab08::~Lab08()
{
}


void Lab08::Init()
{
    // Load textures
    {
        Texture2D* texture = LoadTexture("src\\lab\\lab08\\images\\grass_bilboard.png");
        mapTextures["grass"] = texture;
    }

    {
        Texture2D* texture = LoadTexture("src\\lab\\lab08\\images\\crate.jpg");
        mapTextures["crate"] = texture;
    }

    {
        Texture2D* texture = LoadTexture("src\\lab\\lab08\\images\\earth.png");
        mapTextures["earth"] = texture;
    }

    {
        Texture2D* texture = LoadTexture("src\\lab\\lab08\\images\\noise.png");
        mapTextures["noise"] = texture;
    }

    {
        Texture2D* texture = LoadTexture("assets\\models\\vegetation\\bamboo\\bamboo.png");
        mapTextures["bamboo"] = texture;
    }

    {
        Texture2D* texture = LoadTexture("assets\\textures\\ground.jpg");
        mapTextures["ground"] = texture;
    }

    {
        mapTextures["striped"] = CreateStripedTexture();
    }

    // Load meshes
    {
        Mesh* mesh = new Mesh("plane");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "plane50.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("box");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("sphere");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("bamboo");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "vegetation", "bamboo"), "bamboo.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    // Create a simple quad
    {
        vector<glm::vec2> textureCoords
        {
            // TODO(student): Specify the texture coordinates for the square
            glm::vec2(0.0f, 0.0f),  // top right
            glm::vec2(0.0f, 0.0f),  // bottom right
            glm::vec2(0.0f, 0.0f),  // bottom left
            glm::vec2(0.0f, 0.0f)   // top left
        };

        vector<VertexFormat> vertices
        {
            VertexFormat(glm::vec3(0.5f,   0.5f, 0.0f), glm::vec3(0), glm::vec3(0), textureCoords[0]),
            VertexFormat(glm::vec3(0.5f,   -0.5f, 0.0f), glm::vec3(0), glm::vec3(0), textureCoords[1]),
            VertexFormat(glm::vec3(-0.5f,   -0.5f, 0.0f), glm::vec3(0), glm::vec3(0), textureCoords[2]),
            VertexFormat(glm::vec3(-0.5f,   0.5f, 0.0f), glm::vec3(0), glm::vec3(0), textureCoords[3]),
        };

        vector<unsigned int> indices =
        {
            0, 1, 3,
            1, 2, 3
        };

        Mesh* mesh = new Mesh("square");
        mesh->InitFromData(vertices, indices);
        meshes[mesh->GetMeshID()] = mesh;
    }

    // TODO(student): Load other shaders
    LoadShader("LabShader");
}

Texture2D* Lab08::LoadTexture(const char* imagePath)
{
    int width, height, channels;
    unsigned char* imageData = stbi_load(imagePath, &width, &height, &channels, 0);

    return CreateTexture(width, height, channels, imageData);
}

Texture2D *Lab08::CreateTexture(unsigned int width, unsigned int height,
    unsigned int channels, unsigned char* data)
{
    GLuint textureID = 0;
    unsigned int size = width * height * channels;

    // TODO(student): Generate and bind the new texture ID

    if (GLEW_EXT_texture_filter_anisotropic) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
    }
    // TODO(student): Set the texture parameters (MIN_FILTER and MAG_FILTER) using glTexParameteri

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    CheckOpenGLError();


    // TODO(student): Use the "glTexImage2D" directive to load the information
    // into the graphics processor's memory. Use the correct format based on
    // the number of components:
    //   - 1 color channel - GL_RED
    //   - 2 color channels - GL_RG
    //   - 3 color channels - GL_RGB
    //   - 4 color channels - GL_RGBA

    // TODO(student): Generate texture mip-maps

    CheckOpenGLError();

    // Save the texture into a wrapper Texture2D class for using easier later during rendering phase
    Texture2D* texture = new Texture2D();
    texture->Init(textureID, width, height, channels);

    SAFE_FREE_ARRAY(data);
    return texture;
}

Texture2D *Lab08::CreateStripedTexture()
{
    unsigned int width = 1024;
    unsigned int height = 256;
    unsigned int channels = 3;

    unsigned int size = width * height * channels;
    unsigned char *data = new unsigned char[size];
    // TODO(student): Generate the information for a striped image,
    // where all the pixels on the same line have the same color.
    // The color of the pixels of a line is chosen randomly.


    return CreateTexture(width, height, channels, data);
}

void Lab08::FrameStart()
{
    // Clears the color buffer (using the previously set color) and depth buffer
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::ivec2 resolution = window->GetResolution();
    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);
}


void Lab08::Update(float deltaTimeSeconds)
{
    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(1, 1, -3));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2));
        RenderSimpleMesh(meshes["sphere"], shaders["LabShader"], modelMatrix, mapTextures["earth"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(2, 0.5f, 0));
        modelMatrix = glm::rotate(modelMatrix, RADIANS(60.0f), glm::vec3(1, 0, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f));
        RenderSimpleMesh(meshes["box"], shaders["LabShader"], modelMatrix, mapTextures["crate"], mapTextures["striped"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-2, 0.5f, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.75f));
        modelMatrix = glm::rotate(modelMatrix, RADIANS(75.0f), glm::vec3(1, 1, 0));
        RenderSimpleMesh(meshes["box"], shaders["LabShader"], modelMatrix, mapTextures["striped"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.5f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));
        RenderSimpleMesh(meshes["square"], shaders["LabShader"], modelMatrix, mapTextures["grass"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(-2, -0.5f, -3));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));
        RenderSimpleMesh(meshes["bamboo"], shaders["LabShader"], modelMatrix, mapTextures["bamboo"]);
    }

    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0.1, -15));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f));
        RenderSimpleMesh(meshes["plane"], shaders["LabShader"], modelMatrix, mapTextures["ground"], mapTextures["noise"]);
    }
}


void Lab08::FrameEnd()
{
    DrawCoordinateSystem();
}


void Lab08::RenderSimpleMesh(Mesh *mesh, Shader *shader, const glm::mat4 &modelMatrix, Texture2D *texture1, Texture2D *texture2)
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

    if (texture1)
    {
        // TODO(student): Do these:
        // - activate texture location 0
        // - bind the texture1 ID
        // - send theuniform value

    }

    if (texture2)
    {
        // TODO(student): Do these:
        // - activate texture location 1
        // - bind the texture2 ID
        // - send the uniform value

    }

    // Draw the object
    glBindVertexArray(mesh->GetBuffers()->GetVAO());
    glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}

void Lab08::LoadShader(const std::string& name)
{
    std::string shaderPath = PATH_JOIN(window->props.selfDir, "src/lab", "lab08", "shaders");

    // Create a shader program
    {
        Shader* shader = new Shader(name);
        shader->AddShader(PATH_JOIN(shaderPath, name + ".VS.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, name + ".FS.glsl"), GL_FRAGMENT_SHADER);

        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }
}

/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Lab08::OnInputUpdate(float deltaTime, int mods)
{
}


void Lab08::OnKeyPress(int key, int mods)
{
    // Add key press event
}


void Lab08::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab08::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab08::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab08::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab08::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
}


void Lab08::OnWindowResize(int width, int height)
{
}
