#include "core/gpu/gpu_buffers.h"
#include "core/gpu/typed_buffer.h"
#include "core/gpu/vertex_format.h"

#include "utils/memory_utils.h"


enum VERTEX_ATTRIBUTE_LOC
{
    POS,
    NORMAL,
    TEX_COORD,
};


GPUBuffers::GPUBuffers()
{
    VAO = 0;
    ownsVAO = false;
    elementBuffer = nullptr;
}


GPUBuffers::~GPUBuffers()
{
    ReleaseMemory();
}


void GPUBuffers::CreateVAO()
{
    ReleaseMemory();
    glGenVertexArrays(1, &VAO);
    ownsVAO = true;
}


void GPUBuffers::SetExternalVAO(GLuint externalVAO)
{
    ReleaseMemory();
    VAO = externalVAO;
    ownsVAO = false;
}


void GPUBuffers::ReleaseMemory()
{
    if (ownsVAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    VAO = 0;
    ownsVAO = false;

    for (auto buffer : vertexBuffers) {
        delete buffer;
    }
    vertexBuffers.clear();

    SAFE_FREE(elementBuffer);
}


void GPUBuffers::AddVertexBuffer(Buffer *buffer)
{
    vertexBuffers.push_back(buffer);
}


void GPUBuffers::SetElementBuffer(Buffer *buffer)
{
    SAFE_FREE(elementBuffer);
    elementBuffer = buffer;
    elementBuffer->Bind();
}


const std::vector<Buffer *> &GPUBuffers::GetVertexBuffers() const
{
    return vertexBuffers;
}


GLuint GPUBuffers::GetVAO() const
{
    return VAO;
}


const Buffer *GPUBuffers::GetElementBuffer() const
{
    return elementBuffer;
}


template <class T>
static Buffer *CreateBuffer(GLenum target, const std::vector<T> &data)
{
    auto buffer = new TypedBuffer<T>(target, (unsigned int)data.size(), false, GL_STATIC_DRAW);
    buffer->SetBufferSubData(data);
    return buffer;
}


// Creates a vertex buffer with a single, tightly packed attribute
template <class T>
static void AddAttribute(GPUBuffers &buffers, GLuint location, GLint components, const std::vector<T> &data)
{
    Buffer *buffer = CreateBuffer(GL_ARRAY_BUFFER, data);
    buffer->Bind();
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, components, GL_FLOAT, GL_FALSE, 0, 0);
    buffers.AddVertexBuffer(buffer);
}


void gpu_utils::UploadData(GPUBuffers &buffers,
                           const std::vector<glm::vec3> &positions,
                           const std::vector<glm::vec3> &normals,
                           const std::vector<unsigned int> &indices)
{
    buffers.CreateVAO();
    glBindVertexArray(buffers.GetVAO());

    // Generate and populate the buffers with vertex attributes and the indices
    AddAttribute(buffers, VERTEX_ATTRIBUTE_LOC::POS, 3, positions);
    AddAttribute(buffers, VERTEX_ATTRIBUTE_LOC::NORMAL, 3, normals);
    buffers.SetElementBuffer(CreateBuffer(GL_ELEMENT_ARRAY_BUFFER, indices));

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CheckOpenGLError();
}


void gpu_utils::UploadData(GPUBuffers &buffers,
                           const std::vector<glm::vec3> &positions,
                           const std::vector<glm::vec3> &normals,
                           const std::vector<glm::vec2> &text_coords,
                           const std::vector<unsigned int> &indices)
{
    buffers.CreateVAO();
    glBindVertexArray(buffers.GetVAO());

    // Generate and populate the buffers with vertex attributes and the indices
    AddAttribute(buffers, VERTEX_ATTRIBUTE_LOC::POS, 3, positions);
    AddAttribute(buffers, VERTEX_ATTRIBUTE_LOC::NORMAL, 3, normals);
    AddAttribute(buffers, VERTEX_ATTRIBUTE_LOC::TEX_COORD, 2, text_coords);
    buffers.SetElementBuffer(CreateBuffer(GL_ELEMENT_ARRAY_BUFFER, indices));

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CheckOpenGLError();
}


void gpu_utils::UploadData(GPUBuffers &buffers,
                           const std::vector<VertexFormat> &vertices,
                           const std::vector<unsigned int> &indices)
{
    buffers.CreateVAO();
    glBindVertexArray(buffers.GetVAO());

    // Generate and populate the buffers with vertex attributes and the indices
    Buffer *vertexBuffer = CreateBuffer(GL_ARRAY_BUFFER, vertices);
    vertexBuffer->Bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(sizeof(glm::vec3)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));

    buffers.AddVertexBuffer(vertexBuffer);
    buffers.SetElementBuffer(CreateBuffer(GL_ELEMENT_ARRAY_BUFFER, indices));

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CheckOpenGLError();
}
