#pragma once

#include "platypus/graphics/Buffers.h"
#include <vector>
#include <string>


namespace platypus
{
    bool load_gltf_model(
        const std::string& filepath,
        std::vector<Buffer*>& outVertexBuffers,
        std::vector<Buffer*>& outIndexBuffers
    );
}
