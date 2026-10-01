#include "components/rasterizer_scene.h"

#include <algorithm>
#include <iostream>

using namespace std;
using namespace gfxc;

RasterizerScene::RasterizerScene()
{
    window->SetSize(1280, 720);
}

void RasterizerScene::Init()
{
    GetCameraInput()->SetActive(false);

    {
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        if (major < 4 || (major == 4 && minor < 3)) {
            cout << "The GPU rasterizer requires OpenGL 4.3 (compute shaders), got "
                 << major << "." << minor << endl;
        }
    }

    CreateTargets(window->props.resolution);
    CreateComputeShader("rasterizer", GetShaderPath());

    Initialize();
}

Shader *RasterizerScene::CreateComputeShader(const std::string &name, const std::string &path)
{
    const string &rootDirectory = window->props.selfDir;

    auto *shader = new Shader(name);
    shader->SetIncludeDirectory(rootDirectory);
    shader->AddShader(PATH_JOIN(rootDirectory, path), GL_COMPUTE_SHADER);
    shader->CreateAndLink();
    shaders[name] = shader;

    return shader;
}

Shader *RasterizerScene::GetRasterizer() const
{
    return shaders.at("rasterizer");
}

void RasterizerScene::CreateTargets(glm::ivec2 size)
{
    targetSize = size;

    // Recreating a texture gives it new storage (and a new id), so the framebuffer has to
    // have both attached again
    if (textures.find("colorImage") == textures.end()) {
        textures["colorImage"] = new Texture2D();
        textures["depthImage"] = new Texture2D();
        frameBuffers["target"] = new FrameBuffer();
    }

    textures["colorImage"]->Create(size.x, size.y, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    textures["depthImage"]->Create(size.x, size.y, GL_R32F, GL_RED, GL_FLOAT);

    FrameBuffer *target = frameBuffers["target"];
    target->AttachTexture(0, textures["colorImage"]);
    target->AttachTexture(1, textures["depthImage"]);

    if (!target->IsComplete()) {
        cout << "RasterizerScene: target framebuffer is incomplete" << endl;
    }
}

void RasterizerScene::CreateMesh(const std::string &name,
                                 const std::vector<Vertex> &vertices,
                                 const std::vector<Triangle> &triangles,
                                 unsigned int maxVertices,
                                 unsigned int maxTriangles)
{
    MeshInfo info;
    info.triangleCount = 0;
    info.maxVertices = std::max(maxVertices, static_cast<unsigned int>(vertices.size()));
    info.maxTriangles = std::max(maxTriangles, static_cast<unsigned int>(triangles.size()));
    meshInfo[name] = info;

    buffers[name + ".vertices"] = new TypedBuffer<Vertex>(GL_SHADER_STORAGE_BUFFER, std::max(info.maxVertices, 1u));
    buffers[name + ".indices"] = new TypedBuffer<Triangle>(GL_SHADER_STORAGE_BUFFER, std::max(info.maxTriangles, 1u));

    UpdateMesh(name, vertices, triangles);
}

void RasterizerScene::UpdateMesh(const std::string &name,
                                 const std::vector<Vertex> &vertices,
                                 const std::vector<Triangle> &triangles)
{
    MeshInfo &info = meshInfo.at(name);

    if (vertices.size() > info.maxVertices || triangles.size() > info.maxTriangles) {
        cout << "RasterizerScene: mesh \"" << name << "\" is larger than it was created for" << endl;
        return;
    }

    auto *vertexBuffer = static_cast<TypedBuffer<Vertex> *>(buffers[name + ".vertices"]);
    auto *indexBuffer = static_cast<TypedBuffer<Triangle> *>(buffers[name + ".indices"]);

    if (!vertices.empty()) {
        vertexBuffer->SetBufferSubData(vertices);
    }
    if (!triangles.empty()) {
        indexBuffer->SetBufferSubData(triangles);
    }

    info.triangleCount = static_cast<unsigned int>(triangles.size());
}

void RasterizerScene::BindMesh(const std::string &name)
{
    buffers.at(name + ".vertices")->BindBase(2);
    buffers.at(name + ".indices")->BindBase(3);
}

void RasterizerScene::RasterizeMesh(const std::string &name)
{
    const Shader *shader = GetRasterizer();
    const unsigned int triangleCount = meshInfo.at(name).triangleCount;

    BindMesh(name);

    const glm::uvec2 groupCount = (glm::uvec2(targetSize) + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;
    for (unsigned int i = 0; i < triangleCount; ++i) {
        shader->SetUniform("triangle_id", i);
        glDispatchCompute(groupCount.x, groupCount.y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void RasterizerScene::FrameStart()
{
    const FrameBuffer *target = frameBuffers["target"];
    target->ClearAttachment(0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));  // color
    target->ClearAttachment(1, glm::vec4(1.0f));                    // depth

    // Make the clears visible to the image loads in the compute shader
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RasterizerScene::Update(float deltaTimeSeconds)
{
    const Shader *shader = GetRasterizer();
    if (!shader->GetProgramID()) {
        return;
    }
    shader->Use();

    const Texture2D *colorImage = textures["colorImage"];
    const Texture2D *depthImage = textures["depthImage"];
    glBindImageTexture(0, colorImage->GetTextureID(), 0, GL_FALSE, 0, GL_READ_WRITE, colorImage->GetInternalFormat());
    glBindImageTexture(1, depthImage->GetTextureID(), 0, GL_FALSE, 0, GL_READ_WRITE, depthImage->GetInternalFormat());

    Draw(deltaTimeSeconds);

    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
}

void RasterizerScene::FrameEnd()
{
    frameBuffers["target"]->BlitToDefault(window->props.resolution);
}

void RasterizerScene::OnWindowResize(int width, int height)
{
    // A minimized window has no area to render to
    if (!followWindowSize || width <= 0 || height <= 0) {
        return;
    }

    CreateTargets(glm::ivec2(width, height));
    OnTargetResize();
}
