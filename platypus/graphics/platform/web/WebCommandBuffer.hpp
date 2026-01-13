#pragma once

#include "platypus/graphics/Pipeline.hpp"
#include "platypus/graphics/Buffers.hpp"
#include "WebPipeline.hpp"


namespace platypus
{
    struct CommandBufferImpl
    {
        // This should be assigned when calling bind_pipeline render command
        const Pipeline* pBoundPipeline = nullptr;

        // Used to determine glDrawElements' type
        IndexType drawIndexedType = IndexType::INDEX_TYPE_NONE;
    };
}
