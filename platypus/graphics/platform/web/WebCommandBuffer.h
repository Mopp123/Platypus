#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platy"
#include "platypus/graphics/Buffers.h"


namespace platypus
{
    struct CommandBufferImpl
    {
        // This should be assigned when calling bind_pipeline render command
        PipelineImpl* pPipelineImpl = nullptr;

        // Used to determine glDrawElements' type
        IndexType drawIndexedType = IndexType::INDEX_TYPE_NONE;
    };
}
