#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include <vector>
#include <string>


namespace platypus
{
    // NOTE: Loads currently a single mesh, "whole model" loading not yet supported!
    // ALSO currently this is supposed to put a single index buffer and vertex buffer to out vals
    bool load_gltf_model(
        const CommandPool& commandPool,
        const std::string& filepath,
        std::vector<Buffer*>& outIndexBuffers,
        std::vector<Buffer*>& outVertexBuffers
    );
}
