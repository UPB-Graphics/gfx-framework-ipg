struct Pixel
{
    vec3 color;
    float depth;
    bool inside_triangle;
};

vec2 PixelCenter(uvec2 coordinates)
{
    return vec2(coordinates) + 0.5f;
}
