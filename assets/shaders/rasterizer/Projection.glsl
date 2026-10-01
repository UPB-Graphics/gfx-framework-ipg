// A clip space position to screen space. With `perspective_divide` off (a toggle in the lab's
// UI), w is replaced by 1, so the division leaves the position as it is.
vec3 ProjectVertex(vec4 clip_space_position, mat3 viewport, bool perspective_divide)
{
    if (!perspective_divide)
    {
        clip_space_position.w = 1.0f;
    }

    return ComputeScreenSpacePosition(clip_space_position, viewport);
}

// Moves the triangle from object space to screen space. Returns false if a vertex is behind
// the camera (w <= 0), where the perspective division would mirror it through the origin.
bool ProjectTriangle(inout Vertex v1, inout Vertex v2, inout Vertex v3,
                     mat4 model, mat4 view, mat4 projection, mat3 viewport, bool perspective_divide)
{
    vec4 p1 = ComputeClipSpacePosition(v1.position, model, view, projection);
    vec4 p2 = ComputeClipSpacePosition(v2.position, model, view, projection);
    vec4 p3 = ComputeClipSpacePosition(v3.position, model, view, projection);

    if (perspective_divide && (p1.w <= 0.0f || p2.w <= 0.0f || p3.w <= 0.0f))
    {
        return false;
    }

    v1.position = ProjectVertex(p1, viewport, perspective_divide);
    v2.position = ProjectVertex(p2, viewport, perspective_divide);
    v3.position = ProjectVertex(p3, viewport, perspective_divide);

    return true;
}
