const int BACK = 0;
const int FRONT = 1;
const int NONE = 2;

vec4 ComputeClipSpacePosition(
    vec3 position,
    mat4 model,
    mat4 view,
    mat4 projection)
{
    // TODO(student): Ex. 1

    return vec4(position, 1.0f);
}

vec3 ComputeScreenSpacePosition(
    vec4 clip_space_position,
    mat3 viewport)
{
    vec3 position = clip_space_position.xyz;

    // TODO(student): Ex. 3 - The perspective division

    return vec3((viewport * vec3(position.xy, 1.0f)).xy, position.z * 0.5f + 0.5f);
}

int DetermineTriangleFace(
    vec2 v1,
    vec2 v2,
    vec2 v3)
{
    vec3 cross_product = cross(vec3(v2 - v1, 0.0f), vec3(v3 - v1, 0.0f));

    // TODO(student): Ex. 5

    return NONE;
}
