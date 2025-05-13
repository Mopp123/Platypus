#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/utils/Maths.h"
#include <vector>
#include <string>


namespace platypus
{
    // TODO: Make less dumb
    bool load_gltf_model(
        const CommandPool& commandPool,
        const std::string& filepath,
        std::vector<std::vector<Buffer*>>& outIndexBuffers,
        std::vector<Buffer*>& outVertexBuffers,
        std::vector<Matrix4f>& outTransformationMatrices
    );
}
