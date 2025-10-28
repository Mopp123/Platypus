#pragma once

#include "Batch.hpp"
#include "platypus/graphics/RenderPass.h"


namespace platypus
{
    Batch* create_skinned_batch(
        Batcher& batcher,
        size_t maxLength,
        size_t maxJoints,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID
    );

    Batch* create_skinned_shadow_batch(
        Batcher& batcher,
        size_t maxLength,
        size_t maxJoints,
        const RenderPass* pRenderPass,
        ID_t meshID,
        ID_t materialID,
        void* pShadowPushConstants,
        size_t shadowPushConstantsSize
    );

    void add_to_skinned_batch(
        Batcher& batcher,
        ID_t batchID,
        void* pJointData,
        size_t jointDataSize,
        size_t currentFrame
    );
}
