#include "core/gpu/buffer.h"


Buffer::Buffer(GLenum target, size_t byteSize, GLenum usage)
{
    this->target = target;
    this->byteSize = byteSize;

    glGenBuffers(1, &id);
    glBindBuffer(GL_COPY_WRITE_BUFFER, id);
    glBufferData(GL_COPY_WRITE_BUFFER, byteSize, NULL, usage);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    CheckOpenGLError();
}


Buffer::~Buffer()
{
    glDeleteBuffers(1, &id);
}


GLuint Buffer::GetID() const
{
    return id;
}


GLenum Buffer::GetTarget() const
{
    return target;
}


size_t Buffer::GetByteSize() const
{
    return byteSize;
}


void Buffer::Bind() const
{
    glBindBuffer(target, id);
    CheckOpenGLError();
}


void Buffer::Unbind() const
{
    glBindBuffer(target, 0);
    CheckOpenGLError();
}


void Buffer::BindBase(GLuint index) const
{
    glBindBufferBase(target, index, id);
    CheckOpenGLError();
}


void Buffer::SetData(const void *data, size_t byteSize, size_t byteOffset)
{
    glBindBuffer(GL_COPY_WRITE_BUFFER, id);
    glBufferSubData(GL_COPY_WRITE_BUFFER, byteOffset, byteSize, data);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    CheckOpenGLError();
}


void Buffer::GetData(void *data, size_t byteSize, size_t byteOffset) const
{
    glBindBuffer(GL_COPY_READ_BUFFER, id);
    glGetBufferSubData(GL_COPY_READ_BUFFER, byteOffset, byteSize, data);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    CheckOpenGLError();
}


void Buffer::Clear() const
{
    const GLubyte zero = 0;
    glBindBuffer(GL_COPY_WRITE_BUFFER, id);
    glClearBufferData(GL_COPY_WRITE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &zero);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    CheckOpenGLError();
}
