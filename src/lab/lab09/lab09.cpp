#include "lab/lab09/lab09.h"

#include <vector>
#include <iostream>

using namespace std;
using namespace lab;

/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */

Lab09::Lab09()
{
    angle = 0;

    light_position = glm::vec3(7, 8, 0);
    light_direction = glm::normalize(glm::vec3(-1, -1, 0));

    light_space_near_plane = 0.1f;
    light_space_far_plane = 30.0f;

    draw_framebuffer_textures = true;

    framebuffer_object = 0;
    color_texture = 0;
    depth_texture = 0;
}


Lab09::~Lab09()
{
}


void Lab09::Init()
{
    auto camera = GetSceneCamera();
    camera->SetPositionAndRotation(glm::vec3(-8.5, 1, -1.6), glm::quat(glm::vec3(3.1, -1.3, 3.1)));
    camera->Update();

    // Load a mesh from file into GPU memory
    {
        Mesh* mesh = new Mesh("bamboo");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "vegetation", "bamboo"), "bamboo.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("quad");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "quad.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("sphere");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Mesh* mesh = new Mesh("sponza");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, "src/lab", "lab09/models"), "SponzaNoFlag.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    // Create the shaders for rendering the scene from the
    // perspective of the light source, drawing textures from
    // the framebuffer on the screen and shadows
    LoadShader("ShadowMappingPassOne");
    LoadShader("ShadowMappingPassTwo");
    LoadShader("ViewColorTexture");
    LoadShader("ViewDepthTexture");

    // Create the framebuffer on which the scene is rendered
    // from the perspective of the spot light source
    CreateFramebuffer(4096, 4096);
}

void Lab09::CreateFramebuffer(int width, int height)
{
    // TODO(student): In this method, use the attributes
    // 'framebuffer_object', 'color_texture' and 'depth_texture'
    // declared in lab09.h

    // TODO(student): Generate and bind the framebuffer

    // TODO(student): Generate, bind and initialize the color texture

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // TODO(student): Bind the color texture to the
    // framebuffer as a color attachment at position 0

    // TODO(student): Generate, bind and initialize the depth texture

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // TODO(student): Bind the depth texture to the framebuffer as a depth attachment

    // TODO(student): Set the color texture as the draw texture

    // TODO(student): Check the status of the framebuffer

    // Bind the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Lab09::FrameStart()
{
    ClearScreen();
}

void Lab09::Update(float deltaTimeSeconds)
{
    angle += 0.5f * deltaTimeSeconds;

    // Save camera position and rotation
    auto camera = GetSceneCamera();

    glm::vec3 camPosition = camera->m_transform->GetWorldPosition();
    glm::quat camRotation = camera->m_transform->GetWorldRotation();
    auto projectionInfo = camera->GetProjectionInfo();

    // Render the scene from the perspective of the spot light
    // source. It uses a perspective projection centered in the
    // position of the light source with the view direction in
    // the direction of illumination of the source.
    {
        camera->SetPosition(light_position);
        camera->SetRotation(glm::quatLookAt(light_direction, glm::vec3(0, 1, 0)));

        // The spot light source has an angle opening of 90
        // degrees, so a perspective projection is used with
        // a viewing angle of 90 degrees both vertically and
        // horizontally. For this reason, an aspect ratio of
        // 1 is used.
        camera->SetPerspective(90 /* vert fov */, 1 /* aspect ratio */,
            light_space_near_plane, light_space_far_plane);

        // Save the view and projection matrix
        light_space_view = camera->GetViewMatrix();
        light_space_projection = camera->GetProjectionMatrix();

        // TODO(student): Bind the framebuffer created before
        // and clear the color and depth textures

        // TODO(student): Use glViewport to specify the render
        // area whose size (width, height) is the resolution of
        // the textures in the framebuffer
        //
        // glViewport(start_X, start_Y, width, height);

        // TODO(student): Use DrawObjects to render the objects
        // with "ShadowMappingPassOne" shader
    }

    // Render the scene with shadows
    {
        camera->SetPosition(camPosition);
        camera->SetRotation(camRotation);
        camera->SetProjection(projectionInfo);

        // TODO(student): Bind the default framebuffer
        // and clear the color and depth textures

        // TODO(student): Use glViewport to specify the render
        // area whose size is the resolution of the window

        DrawObjects(shaders["ShadowMappingPassTwo"]);
    }

    // Render the point light in the scene
    {
        glm::mat4 modelMatrix = glm::mat4(1);
        modelMatrix = glm::translate(modelMatrix, light_position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));
        RenderMesh(meshes["sphere"], shaders["Simple"], modelMatrix);
    }

    if (draw_framebuffer_textures) {
        // Render on the screen the color and depth textures
        // of the previously created framebuffer
        DrawFramebufferTextures();
    }
}

void Lab09::DrawObjects(Shader *shader)
{
    for (int i = 0; i < 3; i++)
    {
        float rotateAngle = (angle + i) * ((i % 2) * 2 - 1);
        glm::vec3 position = glm::vec3(-4 + (i % 3) * 2.5, 0, 0);

        glm::mat4 modelMatrix = glm::translate(glm::mat4(1), position);
        modelMatrix = glm::rotate(modelMatrix, rotateAngle, glm::vec3(0, 1, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));

        RenderSimpleMesh(meshes["bamboo"], shader, modelMatrix);
    }

    glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(0.01f));

    RenderSimpleMesh(meshes["sponza"], shader, modelMatrix);
}

void Lab09::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix)
{
    if (!mesh || !shader || !shader->GetProgramID())
        return;

    // Render an object using the specified shader and the specified position
    glUseProgram(shader->program);

    // Set shader uniforms for light properties
    GLint loc_light_position = glGetUniformLocation(shader->program, "light_position");
    glUniform3f(loc_light_position, light_position.x, light_position.y, light_position.z);

    GLint loc_light_direction = glGetUniformLocation(shader->program, "light_direction");
    glUniform3f(loc_light_direction, light_direction.x, light_direction.y, light_direction.z);

    // Set eye position (camera position) uniform
    glm::vec3 eyePosition = GetSceneCamera()->m_transform->GetWorldPosition();
    GLint eye_position = glGetUniformLocation(shader->program, "eye_position");
    glUniform3f(eye_position, eyePosition.x, eyePosition.y, eyePosition.z);

    // Set model matrix uniform
    GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
    glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // Set view matrix uniform
    glm::mat4 viewMatrix = GetSceneCamera()->GetViewMatrix();
    int loc_view_matrix = glGetUniformLocation(shader->program, "View");
    glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    // Set projection matrix uniform
    glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
    int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
    glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // Set light space view matrix uniform
    GLint loc_light_space_view = glGetUniformLocation(shader->program, "light_space_view");
    glUniformMatrix4fv(loc_light_space_view, 1, GL_FALSE, glm::value_ptr(light_space_view));

    // Set light space projection matrix uniform
    GLint loc_light_space_projection = glGetUniformLocation(shader->program, "light_space_projection");
    glUniformMatrix4fv(loc_light_space_projection, 1, GL_FALSE, glm::value_ptr(light_space_projection));

    // Set uniform for the far plane of the
    // projection transformation in the light space
    GLint loc_light_space_far_plane = glGetUniformLocation(shader->program, "light_space_far_plane");
    glUniform1f(loc_light_space_far_plane, light_space_far_plane);

    // TODO(student): Activate texture location 1, bind
    // the depth texture and send the uniform value

    // Draw the object
    mesh->Render();
}

void Lab09::DrawFramebufferTextures()
{
    int screenPosX = window->GetResolution().x - 950;
    int screenPosY = 100;

    int width = 400;
    int height = 400;

    if (color_texture)
    {
        // Render the color texture on the screen
        glViewport(screenPosX, screenPosY, width, height);

        RenderTextureScreen(shaders["ViewColorTexture"], color_texture);
    }

    if (depth_texture)
    {
        // Render the depth texture on the screen
        glViewport(screenPosX + 450, screenPosY, width, height);

        RenderTextureScreen(shaders["ViewDepthTexture"], depth_texture);
    }
}

void Lab09::RenderTextureScreen(Shader *shader, unsigned int textureID)
{
    if (!shader || !shader->GetProgramID())
        return;

    // Render an object using the specified shader and the specified position
    glUseProgram(shader->program);

    // Set uniforms for near and far plane of the
    // projection transformation in the light space
    GLint loc_light_space_near_plane = glGetUniformLocation(shader->program, "light_space_near_plane");
    glUniform1f(loc_light_space_near_plane, light_space_near_plane);

    GLint loc_light_space_far_plane = glGetUniformLocation(shader->program, "light_space_far_plane");
    glUniform1f(loc_light_space_far_plane, light_space_far_plane);

    // Set texture uniform
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);

    // Draw the object
    glBindVertexArray(meshes["quad"]->GetBuffers()->GetVAO());
    glDrawElements(meshes["quad"]->GetDrawMode(), static_cast<int>(meshes["quad"]->indices.size()), GL_UNSIGNED_INT, 0);
}

void Lab09::FrameEnd()
{

}

void Lab09::LoadShader(const std::string& name)
{
    std::string shaderPath = PATH_JOIN(window->props.selfDir, "src/lab", "lab09", "shaders");

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

void Lab09::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input

    float speed = 2;

    if (!window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
    {
        glm::vec3 up = glm::vec3(0, 1, 0);
        glm::vec3 forward = glm::normalize(glm::vec3(light_direction.x, 0, light_direction.z));
        glm::vec3 right = glm::cross(up, forward);

        // Control light position using on W, A, S, D, E, Q
        if (window->KeyHold(GLFW_KEY_W)) light_position += forward * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_A)) light_position += right * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_S)) light_position -= forward * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_D)) light_position -= right * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_E)) light_position += up * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_Q)) light_position -= up * deltaTime * speed;
        if (window->KeyHold(GLFW_KEY_LEFT)) {
            light_direction = glm::mat3(glm::rotate(glm::mat4(1), deltaTime, up)) * light_direction;
        }
        if (window->KeyHold(GLFW_KEY_RIGHT)) {
            light_direction = glm::mat3(glm::rotate(glm::mat4(1), -deltaTime, up)) * light_direction;
        }
        if (window->KeyHold(GLFW_KEY_UP)) {
            light_direction = glm::mat3(glm::rotate(glm::mat4(1), -deltaTime, right)) * light_direction;
        }
        if (window->KeyHold(GLFW_KEY_DOWN)) {
            light_direction = glm::mat3(glm::rotate(glm::mat4(1), deltaTime, right)) * light_direction;
        }
    }
}

void Lab09::OnKeyPress(int key, int mods)
{
    // Add key press event

    // Switch between viewing and hiding the color
    // and depth textures in the framebuffer
    if (key == GLFW_KEY_F1) {
        draw_framebuffer_textures = !draw_framebuffer_textures;
    }
}


void Lab09::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab09::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab09::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab09::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab09::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Lab09::OnWindowResize(int width, int height)
{
    // Treat window resize event
}
