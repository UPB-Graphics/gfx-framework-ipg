#include "core/gpu/frame_buffer.h"

#include <iostream>
#include <utility>

#include "utils/gl_utils.h"


glm::vec4 FrameBuffer::defaultClearColor = glm::vec4(0);


FrameBuffer::FrameBuffer()
{
    glGenFramebuffers(1, &FBO);
    depthTexture = nullptr;
    clearColor = glm::vec4(0, 0, 0, 1);
}


FrameBuffer::~FrameBuffer()
{
    glDeleteFramebuffers(1, &FBO);
}


void FrameBuffer::AttachTexture(unsigned int index, Texture2D *texture)
{
    if (index >= textures.size()) {
        textures.resize(index + 1, nullptr);
    }
    textures[index] = texture;

    // Drop the detached attachments at the end
    while (!textures.empty() && textures.back() == nullptr) {
        textures.pop_back();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D,
                           texture ? texture->GetTextureID() : 0, 0);
    UpdateDrawBuffers();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckOpenGLError();
}


void FrameBuffer::AttachDepthTexture(Texture2D *texture)
{
    depthTexture = texture;

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           texture ? texture->GetTextureID() : 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckOpenGLError();
}


// Expects the framebuffer to be bound
void FrameBuffer::UpdateDrawBuffers() const
{
    if (textures.empty()) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        return;
    }

    std::vector<GLenum> drawBuffers(textures.size());
    for (unsigned int i = 0; i < textures.size(); i++) {
        drawBuffers[i] = textures[i] ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
    }

    glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}


bool FrameBuffer::IsComplete() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}


void FrameBuffer::Bind(bool clearBuffer) const
{
    glm::ivec2 resolution = GetResolution();

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, resolution.x, resolution.y);
    if (clearBuffer) {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}


void FrameBuffer::ClearAttachment(unsigned int index, const glm::vec4 &value) const
{
    // `glClearBufferfv` takes a draw buffer index, which matches the
    // attachment index since draw buffer i is GL_COLOR_ATTACHMENTi
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClearBufferfv(GL_COLOR, index, glm::value_ptr(value));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckOpenGLError();
}


void FrameBuffer::BlitToDefault(const glm::ivec2 &destinationSize, unsigned int index, GLenum filter) const
{
    glm::ivec2 resolution = GetResolution();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glBlitFramebuffer(
        0, 0, resolution.x, resolution.y,
        0, 0, destinationSize.x, destinationSize.y,
        GL_COLOR_BUFFER_BIT, filter);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckOpenGLError();
}


void FrameBuffer::SendResolution(Shader *shader) const
{
    glm::ivec2 resolution = GetResolution();
    glUniform2i(shader->loc_resolution, resolution.x, resolution.y);
}


void FrameBuffer::SetClearColor(glm::vec4 clearColor)
{
    this->clearColor = std::move(clearColor);
}


GLuint FrameBuffer::GetID() const
{
    return FBO;
}


glm::ivec2 FrameBuffer::GetResolution() const
{
    for (auto texture : textures) {
        if (texture) {
            return glm::ivec2(texture->GetWidth(), texture->GetHeight());
        }
    }

    if (depthTexture) {
        return glm::ivec2(depthTexture->GetWidth(), depthTexture->GetHeight());
    }

    return glm::ivec2(0);
}


unsigned int FrameBuffer::GetNumberOfRenderTargets() const
{
    return (unsigned int)textures.size();
}


void FrameBuffer::BindTexture(unsigned int index, unsigned int TextureUnit) const
{
    textures[index]->BindToTextureUnit(TextureUnit);
}


void FrameBuffer::BindDepthTexture(unsigned int TextureUnit) const
{
    depthTexture->BindToTextureUnit(TextureUnit);
}


Texture2D* FrameBuffer::GetTexture(unsigned int index) const
{
    return index < textures.size() ? textures[index] : nullptr;
}


Texture2D* FrameBuffer::GetDepthTexture() const
{
    return depthTexture;
}


unsigned int FrameBuffer::GetTextureID(unsigned int index) const
{
    return textures[index]->GetTextureID();
}


void FrameBuffer::BindAllTextures() const
{
    for (unsigned int i = 0; i < textures.size(); i++) {
        if (textures[i]) {
            textures[i]->BindToTextureUnit(GL_TEXTURE0 + i);
        }
    }
}


void FrameBuffer::BindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void FrameBuffer::BindDefault(const glm::ivec2 &viewportSize, bool clearBuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportSize.x, viewportSize.y);
    if (clearBuffer) {
        glClearColor(defaultClearColor.r, defaultClearColor.g, defaultClearColor.b, defaultClearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    CheckOpenGLError();
}


void FrameBuffer::SetViewport(const glm::ivec2 & viewportSize, const glm::ivec2 offset)
{
    glViewport(offset.x, offset.y, viewportSize.x, viewportSize.y);
}


void FrameBuffer::SetDefaultClearColor(glm::vec4 clearColor)
{
    defaultClearColor = clearColor;
}


void FrameBuffer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
