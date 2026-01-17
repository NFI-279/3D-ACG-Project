#pragma once

#include <vector>
#include <../glm/glm.hpp>
#include "../Model Loading/mesh.h"

struct MountainConfig
{
    float mapMinX;
    float mapMaxX;
    float mapMinZ;
    float mapMaxZ;

    float baseHeight;
    float buffer;
    float ramp;
    float maxHeight;
    float padding;

    int gridResolution;
};

Mesh generateMountainMesh(const MountainConfig& cfg,
    const std::vector<Texture>& textures);
