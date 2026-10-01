#pragma once

#include "utils/glm_utils.h"

namespace transform2D
{
    // Translation matrix
    inline glm::mat3 Translate(float translateX, float translateY)
    {
        // TODO(student): Ex. 1
        return glm::mat3(1);
    }

    // Scaling matrix
    inline glm::mat3 Scale(float scaleX, float scaleY)
    {
        // TODO(student): Ex. 1
        return glm::mat3(1);
    }

    // Rotation matrix
    inline glm::mat3 Rotate(float radians)
    {
        // TODO(student): Ex. 1
        return glm::mat3(1);
    }

    struct ViewportSpace
    {
        ViewportSpace() : x(0), y(0), width(1), height(1) {}
        ViewportSpace(int x, int y, int width, int height)
            : x(x), y(y), width(width), height(height) {}
        int x;
        int y;
        int width;
        int height;
    };

    struct LogicSpace
    {
        LogicSpace() : x(0), y(0), width(1), height(1) {}
        LogicSpace(float x, float y, float width, float height)
            : x(x), y(y), width(width), height(height) {}
        float x;
        float y;
        float width;
        float height;
    };

    // Viewport transformation
    inline glm::mat3 Viewport(
        const LogicSpace& logic_space,
        const ViewportSpace& viewport_space)
    {
        // TODO(student): Ex. 1 - You can use the translation and scaling transformations

        return glm::mat3(1);
    }

}   // namespace transform2D
