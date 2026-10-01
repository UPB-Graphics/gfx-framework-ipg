float ComputeTriangleArea(vec2 v1, vec2 v2, vec2 v3)
{
    // TODO(student): Ex. 1

    return 0.0f;
}

bool CheckPointInsideBoundingBox(
    vec2 point,
    Vertex v1,
    Vertex v2,
    Vertex v3)
{
    // TODO(student): Ex. 1

    return false;
}

bool CheckPointInsideTriangle(
    vec2 point,
    Vertex v1,
    Vertex v2,
    Vertex v3)
{
    const float EPSILON = 5.0f;

    // TODO(student): Ex. 1

    return false;
}

vec3 ComputePixelColor(
    vec2 point,
    Vertex v1,
    Vertex v2,
    Vertex v3)
{
    // TODO(student): Ex. 2

    return v3.color;
}

float ComputePixelDepth(
    vec2 point,
    Vertex v1,
    Vertex v2,
    Vertex v3)
{
    // TODO(student): Ex. 3

    return v3.position.z;
}

Pixel RasterizeTriangle(
    uvec2 pixel_coord,
    Vertex v1,
    Vertex v2,
    Vertex v3)
{
    Pixel pixel = Pixel(vec3(0.0f), 1.0f, false);

    // TODO(student): Ex. 1 - Is the center of the pixel inside the triangle?

    // TODO(student): Ex. 2 - The color of the pixel

    // TODO(student): Ex. 3 - The depth of the pixel

    return pixel;
}
