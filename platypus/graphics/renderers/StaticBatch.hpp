#pragma once

#include "Batch.hpp"
#include "platypus/graphics/RenderPass.h"


namespace platypus
{
    Batch* create_static_batch(
        Batcher& batcher,
        size_t maxLength,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID
    );

    Batch* create_static_shadow_batch(
        Batcher& batcher,
        size_t maxLength,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID,
        void* pShadowPushConstants,
        size_t shadowPushConstantsSize
    );

    void add_to_static_batch(
        Batcher& batcher,
        ID_t batchID,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    );
}
