#pragma once

#include <vector>

#include "core/gpu/buffer.h"
#include "utils/memory_utils.h"


/*
 *  Buffer of `size` elements of type `StorageEntry`, optionally mirrored
 *  by a local (CPU) copy. Offsets and sizes are given in elements.
 *
 *  Examples:
 *      TypedBuffer<VertexFormat>(GL_ARRAY_BUFFER, count)           vertex buffer
 *      TypedBuffer<unsigned int>(GL_ELEMENT_ARRAY_BUFFER, count)   element buffer
 *      TypedBuffer<Particle>(GL_SHADER_STORAGE_BUFFER, count)      SSBO
 */
template <class StorageEntry>
class TypedBuffer : public Buffer
{
 public:
    TypedBuffer(GLenum target, unsigned int size, bool createLocalBuffer = false, GLenum usage = GL_DYNAMIC_DRAW)
        : Buffer(target, size * sizeof(StorageEntry), usage)
    {
        this->size = size;
        data = createLocalBuffer ? AllocateLocal(size) : nullptr;
    }

    ~TypedBuffer() override
    {
        ::operator delete(data);
    }

    // Uploads `size` elements, the whole buffer
    void SetBufferData(const StorageEntry *data)
    {
        SetData(data, byteSize);
    }

    void SetBufferSubData(const StorageEntry *data, unsigned int offset, unsigned int count)
    {
        SetData(data, count * sizeof(StorageEntry), offset * sizeof(StorageEntry));
    }

    void SetBufferSubData(const std::vector<StorageEntry> &data, unsigned int offset = 0)
    {
        SetBufferSubData(data.data(), offset, (unsigned int)data.size());
    }

    // Downloads the whole buffer into the local copy, see `GetBuffer`
    void ReadBuffer()
    {
        if (data == nullptr)
        {
            data = AllocateLocal(size);
        }

        GetData(data, byteSize);
    }

    const StorageEntry *GetBuffer() const
    {
        return data;
    }

    unsigned int GetSize() const
    {
        return size;
    }

 private:
    // The local copy mirrors GPU memory byte for byte, so it is raw storage;
    // this also keeps `StorageEntry` free of a default constructor requirement.
    static StorageEntry *AllocateLocal(unsigned int size)
    {
        return static_cast<StorageEntry *>(::operator new(size * sizeof(StorageEntry)));
    }

 private:
    unsigned int size;
    StorageEntry *data;
};
