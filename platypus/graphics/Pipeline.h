#pragma once

#include <cstdint>
#include <vector>
#include "Shader.h"
#include "Buffers.h"
#include "RenderPass.h"
#include "Descriptors.h"


namespace platypus
{
    // *Put this here just to get rid of VkRect2D
    struct Rect2D
    {
        int32_t offsetX;
        int32_t offsetY;
        uint32_t width;
        uint32_t height;
    };


    enum PipelineBindPoint
    {
        PIPELINE_BIND_POINT_NONE = 0,
        PIPELINE_BIND_POINT_COMPUTE = 1,
        PIPELINE_BIND_POINT_GRAPHICS = 2
    };


    enum CullMode
    {
        CULL_MODE_NONE = 0,
        CULL_MODE_FRONT = 1,
        CULL_MODE_BACK = 2
    };


    enum FrontFace
    {
        FRONT_FACE_COUNTER_CLOCKWISE = 0,
        FRONT_FACE_CLOCKWISE = 1
    };


    enum DepthCompareOperation
    {
        COMPARE_OP_NEVER = 0,
        COMPARE_OP_LESS = 1,
        COMPARE_OP_EQUAL = 2,
        COMPARE_OP_LESS_OR_EQUAL = 3,
        COMPARE_OP_GREATER = 4,
        COMPARE_OP_NOT_EQUAL = 5,
        COMPARE_OP_GREATER_OR_EQUAL = 6,
        COMPARE_OP_ALWAYS = 7,
    };


    struct PipelineImpl;
    class Pipeline
    {
    private:
        PipelineImpl* _pImpl = nullptr;

    public:
        Pipeline();
        ~Pipeline();

        void create(
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
        );

        void destroy();

        inline PipelineImpl* getImpl() const { return _pImpl; }
    };
}
