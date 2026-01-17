#include "mountain.h"
#include <cmath>
#include <algorithm>

// Math
static float smoothstep01(float t)
{
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Math+
static float hash2D(int x, int z)
{
    int n = x * 374761393 + z * 668265263;
    n = (n ^ (n >> 13)) * 1274126177;
    n = n ^ (n >> 16);
    return (n & 0x7fffffff) / 2147483648.0f;
}

// Math++
static float valueNoise(float x, float z)
{
    int x0 = (int)std::floor(x);
    int z0 = (int)std::floor(z);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float tx = x - x0;
    float tz = z - z0;

    float sx = smoothstep01(tx);
    float sz = smoothstep01(tz);

    float v00 = hash2D(x0, z0);
    float v10 = hash2D(x1, z0);
    float v01 = hash2D(x0, z1);
    float v11 = hash2D(x1, z1);

    float a = glm::mix(v00, v10, sx);
    float b = glm::mix(v01, v11, sx);
    return glm::mix(a, b, sz);
}

// Math+++
static float ridgedFbm(float x, float z)
{
    const int OCTAVES = 6;
    const float BASE_FREQ = 0.0065f;

    float sum = 0.0f;
    float amp = 1.0f;
    float freq = BASE_FREQ;
    float ampSum = 0.0f;

    for (int i = 0; i < OCTAVES; i++)
    {
        float n = valueNoise(x * freq, z * freq);
        n = n * 2.0f - 1.0f;
        float ridge = 1.0f - std::abs(n);
        ridge *= ridge;

        sum += ridge * amp;
        ampSum += amp;

        freq *= 2.0f;
        amp *= 0.5f;
    }

    return (ampSum > 0.0f) ? sum / ampSum : 0.0f;
}

// Math+++
static float distanceToPlayableEdge(float x, float z,
    const MountainConfig& c)
{
    float cx = (c.mapMinX + c.mapMaxX) * 0.5f;
    float cz = (c.mapMinZ + c.mapMaxZ) * 0.5f;

    float halfX = (c.mapMaxX - c.mapMinX) * 0.5f + c.buffer;
    float halfZ = (c.mapMaxZ - c.mapMinZ) * 0.5f + c.buffer;

    float dx = std::max(std::abs(x - cx) - halfX, 0.0f);
    float dz = std::max(std::abs(z - cz) - halfZ, 0.0f);

    return std::sqrt(dx * dx + dz * dz);
}

// Math++++
static float mountainHeight(float x, float z,
    const MountainConfig& c)
{
    float d = distanceToPlayableEdge(x, z, c);
    if (d <= 0.0f)
        return 0.0f;

    float t = glm::clamp(d / c.ramp, 0.0f, 1.0f);
    t = smoothstep01(t);

    float h = ridgedFbm(x, z) * c.maxHeight;
    return t * h;
}

// Constructor
Mesh generateMountainMesh(const MountainConfig& cfg,
    const std::vector<Texture>& textures)
{
    const int GRID = cfg.gridResolution;

    float startX = cfg.mapMinX - cfg.padding;
    float endX = cfg.mapMaxX + cfg.padding;
    float startZ = cfg.mapMinZ - cfg.padding;
    float endZ = cfg.mapMaxZ + cfg.padding;

    std::vector<Vertex> vertices;
    std::vector<int> indices;

    vertices.reserve((GRID + 1) * (GRID + 1));
    indices.reserve(GRID * GRID * 6);

    for (int z = 0; z <= GRID; z++)
    {
        for (int x = 0; x <= GRID; x++)
        {
            float fx = glm::mix(startX, endX, x / (float)GRID);
            float fz = glm::mix(startZ, endZ, z / (float)GRID);

            float h = mountainHeight(fx, fz, cfg);

            Vertex v;
            v.pos = glm::vec3(fx, cfg.baseHeight + h, fz);
            v.textureCoords = glm::vec2(fx * 0.02f, fz * 0.02f);
            v.normals = glm::vec3(0);

            vertices.push_back(v);
        }
    }

    for (int z = 0; z < GRID; z++)
    {
        for (int x = 0; x < GRID; x++)
        {
            int i0 = z * (GRID + 1) + x;
            int i1 = i0 + 1;
            int i2 = (z + 1) * (GRID + 1) + x;
            int i3 = i2 + 1;

            indices.insert(indices.end(),
                { i0, i2, i1, i1, i2, i3 });
        }
    }

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        auto& a = vertices[indices[i]];
        auto& b = vertices[indices[i + 1]];
        auto& cV = vertices[indices[i + 2]];

        glm::vec3 n = glm::normalize(
            glm::cross(b.pos - a.pos, cV.pos - a.pos));

        a.normals += n;
        b.normals += n;
        cV.normals += n;
    }

    for (auto& v : vertices)
        v.normals = glm::normalize(v.normals);

    return Mesh(vertices, indices, textures);
}
