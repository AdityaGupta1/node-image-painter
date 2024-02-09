#pragma once

#include <glm/glm.hpp>
#include <cuda_runtime.h>

#include <functional>

struct ResolutionHash
{
    size_t operator()(const glm::ivec2& k) const
    {
        return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
    }
};

struct Texture
{
    glm::vec4* dev_pixels;
    glm::ivec2 resolution;
    int numReferences{ 0 };
};