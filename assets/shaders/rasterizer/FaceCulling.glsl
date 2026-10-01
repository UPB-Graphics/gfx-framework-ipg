bool IsTriangleCulled(Vertex v1, Vertex v2, Vertex v3, int cull_face)
{
    return DetermineTriangleFace(v1.position.xy, v2.position.xy, v3.position.xy) == cull_face;
}
