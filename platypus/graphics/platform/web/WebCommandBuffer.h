#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Buffers.h"
#include "WebPipeline.h"


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
