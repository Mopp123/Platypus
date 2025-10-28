#pragma once

#include "Batch.hpp"
#include "platypus/graphics/RenderPass.h"


namespace platypus
{
    Batch* create_terrain_batch(
        Batcher& batcher,
        size_t maxLength,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID
    );

    void add_to_terrain_batch(
        Batcher& batcher,
        ID_t batchID,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    );
}
