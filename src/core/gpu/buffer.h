#pragma once

#include <cstddef>

#include "utils/gl_utils.h"


/*
 *  Untyped GPU buffer. The target decides how it is used:
 *   - GL_ARRAY_BUFFER            vertex buffer
 *   - GL_ELEMENT_ARRAY_BUFFER    element (index) buffer
 *   - GL_SHADER_STORAGE_BUFFER   shader storage buffer (SSBO)
 *   - any other buffer target (GL_UNIFORM_BUFFER, ...)
 *
 *  Data transfers go through GL_COPY_WRITE_BUFFER / GL_COPY_READ_BUFFER,
 *  so they never change the element buffer bound to the current VAO.
 */
class Buffer
{
 public:
    Buffer(GLenum target, size_t byteSize, GLenum usage = GL_DYNAMIC_DRAW);
    virtual ~Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    GLuint GetID() const;
    GLenum GetTarget() const;
    size_t GetByteSize() const;

    // Binds the buffer to its target. For an element buffer, do this while
    // the VAO that should use it is bound.
    void Bind() const;
    void Unbind() const;

    // Binds the buffer to an indexed binding point of its target
    // (`layout(binding = index)` in shaders). Only valid for indexed targets
    // such as GL_SHADER_STORAGE_BUFFER or GL_UNIFORM_BUFFER.
    void BindBase(GLuint index) const;

    void SetData(const void *data, size_t byteSize, size_t byteOffset = 0);
    void GetData(void *data, size_t byteSize, size_t byteOffset = 0) const;

    // Fills the whole buffer with zeros
    void Clear() const;

 protected:
    GLuint id;
    GLenum target;
    size_t byteSize;
};
