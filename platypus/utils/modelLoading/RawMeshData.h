#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/utils/Maths.h"
#include "platypus/utils/AnimationDataUtils.h"
#include "platypus/Common.h"
#include <vector>


namespace platypus
{
    struct MeshBufferData
    {
        size_t elementSize = 0;
        size_t length = 0; // length in elements. NOT SIZE IN BYTES!
        std::vector<PE_byte> data;
    };

    struct MeshData
    {
        VertexBufferLayout vertexBufferLayout;
        MeshBufferData vertexBufferData;
        std::vector<MeshBufferData> indexBufferData;
        Matrix4f transformationMatrix = Matrix4f(1.0f);
    };

}
