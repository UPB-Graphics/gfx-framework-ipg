#pragma once

#include <vector>

#include "core/gpu/buffer.h"
#include "core/gpu/vertex_format.h"
#include "utils/gl_utils.h"
#include "utils/glm_utils.h"


/*
 *  The GPU side of a mesh: a VAO together with the vertex buffers and
 *  the element buffer it references. Owns everything it holds.
 */
class GPUBuffers
{
 public:
    GPUBuffers();
    ~GPUBuffers();

    GPUBuffers(const GPUBuffers &) = delete;
    GPUBuffers &operator=(const GPUBuffers &) = delete;

    // Releases what was held before and creates a new VAO
    void CreateVAO();

    // Releases what was held before and uses a VAO created elsewhere,
    // which is not deleted by `ReleaseMemory`
    void SetExternalVAO(GLuint externalVAO);

    void ReleaseMemory();

    // Takes ownership of the buffer
    void AddVertexBuffer(Buffer *buffer);

    // Takes ownership of the buffer. The VAO must be bound, since it
    // records the element buffer binding.
    void SetElementBuffer(Buffer *buffer);

    const std::vector<Buffer *> &GetVertexBuffers() const;
    const Buffer *GetElementBuffer() const;
    GLuint GetVAO() const;

 private:
    GLuint VAO;
    bool ownsVAO;
    std::vector<Buffer *> vertexBuffers;
    Buffer *elementBuffer;
};


namespace gpu_utils
{
    // Each function fills `buffers`, releasing what it held before,
    // and leaves no VAO bound.

    void UploadData(GPUBuffers &buffers,
                    const std::vector<glm::vec3> &positions,
                    const std::vector<glm::vec3> &normals,
                    const std::vector<unsigned int> &indices);

    void UploadData(GPUBuffers &buffers,
                    const std::vector<glm::vec3> &positions,
                    const std::vector<glm::vec3> &normals,
                    const std::vector<glm::vec2> &text_coords,
                    const std::vector<unsigned int> &indices);

    void UploadData(GPUBuffers &buffers,
                    const std::vector<VertexFormat> &vertices,
                    const std::vector<unsigned int> &indices);
}   // namespace gpu_utils
