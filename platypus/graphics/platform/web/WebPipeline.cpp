#include "platypus/graphics/Pipeline.h"

namespace platypus
{
    Pipeline::Pipeline()
    {
    }

    Pipeline::~Pipeline()
    {
    }

    void Pipeline::create(
        const RenderPass& renderPass,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        const std::vector<const DescriptorSetLayout*>& descriptorLayouts,
        const Shader& vertexShader,
        const Shader& fragmentShader,
        float viewportWidth,
        float viewportHeight,
        const Rect2D viewportScissor,
        CullMode cullMode,
        FrontFace frontFace,
        bool enableDepthTest,
        DepthCompareOperation depthCmpOp,
        bool enableColorBlending, // TODO: more options to handle this..
        uint32_t pushConstantSize,
        uint32_t pushConstantStageFlags
    )
    {
    }

    void Pipeline::destroy()
    {
    }
}
