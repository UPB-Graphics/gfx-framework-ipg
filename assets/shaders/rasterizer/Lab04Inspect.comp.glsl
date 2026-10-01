#version 430 core

// Runs the same stages as Lab04.comp.glsl over the vertices and triangles of the mesh, and
// writes out what they produce, for the vertex inspector and the wireframe of the lab's UI.
// Every position goes through the students' functions, so the inspector shows what they do.

// Includes are resolved from the folder of the executable, which holds assets/ and src/
#include "assets/shaders/rasterizer/InputAssembly.glsl"
#include "src/lab/lab04/gpu_programs/lab04.glsl"
#include "assets/shaders/rasterizer/Projection.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct InspectedVertex
{
    vec4 world;
    vec4 camera;
    vec4 clip;
    vec4 screen;
};

layout(std430, binding = 4) writeonly buffer InspectedVertices
{
    InspectedVertex inspected_vertices[];
};

// The face each triangle shows, or -1 if it is not drawn because a vertex is behind the camera
layout(std430, binding = 5) writeonly buffer InspectedFaces
{
    int inspected_faces[];
};

uniform uint vertex_count;
uniform uint triangle_count;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 viewport;
uniform bool perspective_divide;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if (id < vertex_count)
    {
        Vertex vertex = vertices[id];
        const mat4 identity = mat4(1.0f);

        // The chain stopped after the model, then after the view transformation
        InspectedVertex inspected;
        inspected.world = ComputeClipSpacePosition(vertex.position, model, identity, identity);
        inspected.camera = ComputeClipSpacePosition(vertex.position, model, view, identity);
        inspected.clip = ComputeClipSpacePosition(vertex.position, model, view, projection);

        inspected.screen = vec4(ProjectVertex(inspected.clip, viewport, perspective_divide), 0.0f);

        inspected_vertices[id] = inspected;
    }

    if (id < triangle_count)
    {
        Vertex v1, v2, v3;
        AssembleTriangle(id, v1, v2, v3);

        if (!ProjectTriangle(v1, v2, v3, model, view, projection, viewport, perspective_divide))
        {
            inspected_faces[id] = -1;
            return;
        }

        inspected_faces[id] = DetermineTriangleFace(v1.position.xy, v2.position.xy, v3.position.xy);
    }
}
