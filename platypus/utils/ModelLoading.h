#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include <vector>
#include <string>


namespace platypus
{
    bool load_gltf_model(
        const CommandPool& commandPool,
        const std::string& filepath,
        std::vector<Buffer*>& outIndexBuffers,
        std::vector<Buffer*>& outVertexBuffers
    );
}
