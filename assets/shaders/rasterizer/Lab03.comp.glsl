#version 430 core

// Lets images without a format qualifier be read, which is needed to pass them as function
// parameters. Their format comes from glBindImageTexture.
#extension GL_EXT_shader_image_load_formatted : require

// Includes are resolved from the folder of the executable, which holds assets/ and src/
#include "assets/shaders/rasterizer/InputAssembly.glsl"
#include "assets/shaders/rasterizer/Rasterization.glsl"
#include "src/lab/lab02/gpu_programs/lab02.glsl"
#include "src/lab/lab03/gpu_programs/lab03.glsl"
#include "assets/shaders/rasterizer/DepthTest.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform image2D color_image;
layout(binding = 1) uniform image2D depth_image;

uniform uint triangle_id;

uniform mat3 model;
uniform mat3 viewport;

// One invocation per pixel of the image, for the triangle `triangle_id`
void main()
{
    uvec2 coordinates = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(coordinates, uvec2(imageSize(color_image)))))
    {
        return;
    }

    Vertex v1, v2, v3;
    AssembleTriangle(triangle_id, v1, v2, v3);

    v1 = TransformVertex(v1, model, viewport);
    v2 = TransformVertex(v2, model, viewport);
    v3 = TransformVertex(v3, model, viewport);

    Pixel pixel = RasterizeTriangle(coordinates, v1, v2, v3);
    if (!pixel.inside_triangle)
    {
        return;
    }

    if (DepthTest(pixel, coordinates, depth_image))
    {
        WritePixel(pixel, coordinates, color_image, depth_image);
    }
}
