#pragma once

#include "utils/gl_utils.h"


class Texture2D
{
 public:
    Texture2D();
    ~Texture2D();

    void Bind() const;
    void BindToTextureUnit(GLenum TextureUnit) const;
    void UnBind() const;

    void UploadNewData(const unsigned char *img);
    void UploadNewData(const unsigned int *img);

    void Init(GLuint gpuTextureID, unsigned int width, unsigned int height, unsigned int channels);
    void Create(const unsigned char* img, int width, int height, int chn);
    void CreateU16(const unsigned int* img, int width, int height, int chn);

    // Creates a texture with an explicit storage format, e.g.
    //   Create(w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
    //   Create(w, h, GL_R32F, GL_RED, GL_FLOAT)
    //   Create(w, h, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT)
    // `format` and `type` describe `data`, which may be null for an empty texture.
    // To render into it, attach it to a framebuffer with glFramebufferTexture2D.
    void Create(unsigned int width, unsigned int height, GLint internalFormat, GLenum format, GLenum type, const void *data = nullptr);

    void CreateCubeTexture(const float *data, unsigned int width, unsigned int height, unsigned int chn);

    bool Load2D(const char* fileName, GLenum wrappingMode = GL_REPEAT);
    void SaveToFile(const char* fileName);
    void CacheInMemory(bool state);

    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    void GetSize(unsigned int &width, unsigned int &height) const;
    unsigned char *GetImageData() const;

    unsigned int GetNrChannels() const;
    GLint GetInternalFormat() const;

    void SetWrappingMode(GLenum mode);
    void SetFiltering(GLenum minFilter, GLenum magFilter = GL_LINEAR);

    GLuint GetTextureID() const;

 private:
    void SetTextureParameters();
    void Init2DTexture(unsigned int width, unsigned int height, unsigned int channels);

 protected:
    bool cacheInMemory;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    GLint internalFormat;

    GLuint targetType;
    GLuint textureID;
    GLenum wrappingMode;
    GLenum textureMinFilter;
    GLenum textureMagFilter;

    unsigned char *imageData;
};
