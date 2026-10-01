struct Vertex
{
    vec3 position;
    vec3 color;
};

layout(std430, binding = 2) readonly buffer Vertices
{
    Vertex vertices[];
};

layout(std430, binding = 3) readonly buffer Indices
{
    uint indices[];
};

void AssembleTriangle(uint triangle_id, out Vertex v1, out Vertex v2, out Vertex v3)
{
    v1 = vertices[indices[triangle_id * 3 + 0]];
    v2 = vertices[indices[triangle_id * 3 + 1]];
    v3 = vertices[indices[triangle_id * 3 + 2]];
}
