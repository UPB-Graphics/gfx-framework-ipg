#pragma once

#include <cstddef>
#include <cstdint>

#include "utils/glm_utils.h"

// The CPU side of the data the GPU rasterizer labs share with their shaders
// (see assets/shaders/rasterizer/InputAssembly.glsl).

namespace gfxc {
    // Matches the std430 layout of `Vertex` in assets/shaders/rasterizer/InputAssembly.glsl
    struct Vertex {
        alignas(16) glm::vec3 position;  ///< xy in the lab's 2D space (y up), z is depth in [0, 1]
        alignas(16) glm::vec3 color;     ///< rgb, each channel in [0, 1]
    };

    static_assert(sizeof(Vertex) == 32 && offsetof(Vertex, color) == 16, "Vertex must match std430");

    // A triangle is three indices into the vertex list, so vertices are shared:
    // moving one moves every triangle that references it.
    using Triangle = glm::uvec3;

    // Uploaded as a tightly packed `uint indices[]`
    static_assert(sizeof(Triangle) == 3 * sizeof(uint32_t), "Triangle must be three packed uints");
} // gfxc
