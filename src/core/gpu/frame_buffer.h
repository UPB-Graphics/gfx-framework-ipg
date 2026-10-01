#pragma once

#include <vector>

#include "core/gpu/shader.h"
#include "core/gpu/texture2D.h"
#include "utils/glm_utils.h"


/*
 *  Framebuffer object with textures attached to it. The textures are
 *  created elsewhere (see `Texture2D::Create`) and are not owned by
 *  the framebuffer.
 */
class FrameBuffer
{
 public:
    FrameBuffer();
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer &) = delete;
    FrameBuffer &operator=(const FrameBuffer &) = delete;

    // Attaches `texture` as color attachment `index` and enables it as a
    // draw buffer. Pass nullptr to detach.
    void AttachTexture(unsigned int index, Texture2D *texture);
    void AttachDepthTexture(Texture2D *texture);

    bool IsComplete() const;

    // Binds the framebuffer and sets the viewport to its resolution
    void Bind(bool clearBuffer = true) const;

    // Clears a single color attachment, whatever its format
    void ClearAttachment(unsigned int index, const glm::vec4 &value) const;

    // Copies color attachment `index` to the default framebuffer,
    // stretched to `destinationSize`
    void BlitToDefault(const glm::ivec2 &destinationSize, unsigned int index = 0, GLenum filter = GL_NEAREST) const;

    void BindTexture(unsigned int index, unsigned int TextureUnit) const;
    void BindAllTextures() const;
    void BindDepthTexture(unsigned int TextureUnit) const;

    GLuint GetID() const;
    Texture2D* GetTexture(unsigned int index) const;
    Texture2D* GetDepthTexture() const;
    unsigned int GetTextureID(unsigned int index) const;
    unsigned int GetNumberOfRenderTargets() const;

    // The resolution of the attachments
    glm::ivec2 GetResolution() const;

    void SendResolution(Shader *shader) const;
    void SetClearColor(glm::vec4 clearColor);

    static void Clear();
    static void BindDefault();
    static void BindDefault(const glm::ivec2 &viewportSize, bool clearBuffer = false);
    static void SetViewport(const glm::ivec2 &viewportSize, const glm::ivec2 offset = glm::ivec2(0, 0));
    static void SetDefaultClearColor(glm::vec4 clearColor);

 private:
    void UpdateDrawBuffers() const;

 private:
    GLuint FBO;
    std::vector<Texture2D *> textures;
    Texture2D *depthTexture;

    glm::vec4 clearColor;
    static glm::vec4 defaultClearColor;
};
