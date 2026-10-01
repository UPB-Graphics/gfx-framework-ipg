bool DepthTest(Pixel pixel, uvec2 coordinates, image2D depth_image)
{
    return pixel.depth < imageLoad(depth_image, ivec2(coordinates)).r;
}

void WritePixel(Pixel pixel, uvec2 coordinates, image2D color_image, image2D depth_image)
{
    imageStore(depth_image, ivec2(coordinates), vec4(pixel.depth));
    imageStore(color_image, ivec2(coordinates), vec4(pixel.color, 1.0f));
}
