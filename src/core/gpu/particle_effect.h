#pragma once

#include <vector>
#include <chrono>

#include "utils/gl_utils.h"
#include "utils/glm_utils.h"

#include "components/camera.h"
#include "components/transform.h"

#include "core/gpu/shader.h"
#include "core/gpu/texture2D.h"
#include "core/gpu/typed_buffer.h"


// TODO(developer): Decouple gfxc components from this class
template <class T>
class ParticleEffect
{
 public:
    ParticleEffect();
    virtual ~ParticleEffect();

    virtual void Generate(unsigned int particleCount, bool createLocalBuffer = false);
    virtual void FillRandomData(std::function<T(void)> generator);
    virtual void Render(gfxc::Camera *camera, Shader *shader, unsigned int nrParticles = -1);

    virtual TypedBuffer<T>* GetParticleBuffer() const
    {
        return particles;
    }

    virtual unsigned int GetSize() const
    {
        return particleCount;
    }

 public:
    gfxc::Transform * source;

 protected:
    unsigned int particleCount;
    GLuint VAO;
    TypedBuffer<unsigned int> *indexBuffer;
    TypedBuffer<T> *particles;
};


template <class T>
ParticleEffect<T>::ParticleEffect()
{
    source = new gfxc::Transform();
    VAO = 0;
    indexBuffer = nullptr;
    particles = nullptr;
}


template <class T>
ParticleEffect<T>::~ParticleEffect()
{
    SAFE_FREE(source);
    SAFE_FREE(indexBuffer);
    SAFE_FREE(particles);
    glDeleteVertexArrays(1, &VAO);
}


template <class T>
void ParticleEffect<T>::Render(gfxc::Camera *camera, Shader *shader, unsigned int nrParticles)
{
    // Bind MVP
    glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(source->GetModel()));
    glUniformMatrix4fv(shader->loc_view_matrix, 1, false, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(shader->loc_projection_matrix, 1, false, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniform3fv(shader->loc_eye_pos, 1, glm::value_ptr(camera->m_transform->GetWorldPosition()));

    // Bind Particle Storage
    particles->BindBase(0);

    // Render Particles
    glBindVertexArray(VAO);
    glDrawElements(GL_POINTS, MIN(particleCount, nrParticles), GL_UNSIGNED_INT, 0);
}


template <class T>
void ParticleEffect<T>::Generate(unsigned int particleCount, bool createLocalBuffer)
{
    this->particleCount = particleCount;

    SAFE_FREE(particles);
    particles = new TypedBuffer<T>(GL_SHADER_STORAGE_BUFFER, particleCount, createLocalBuffer);

    std::vector<unsigned int> indices(particleCount);
    for (unsigned int i = 0; i < particleCount; i++)
    {
        indices[i] = i;
    }

    SAFE_FREE(indexBuffer);
    indexBuffer = new TypedBuffer<unsigned int>(GL_ELEMENT_ARRAY_BUFFER, particleCount, false, GL_STATIC_DRAW);
    indexBuffer->SetBufferSubData(indices);

    glDeleteVertexArrays(1, &VAO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // The element buffer binding is stored in the VAO
    indexBuffer->Bind();

    glBindVertexArray(0);
}


template <class T>
void ParticleEffect<T>::FillRandomData(std::function<T(void)> generator)
{
    particles->ReadBuffer();
    auto data = const_cast<T*>(particles->GetBuffer());
    for (unsigned int i = 0; i < particleCount; i++) {
        data[i] = generator();
    }
    particles->SetBufferData(data);
}
