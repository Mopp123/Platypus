#pragma once

#include <cstdint>
#include <vector>
#include "Shader.h"
#include "Buffers.h"
#include "RenderPass.hpp"
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

        const RenderPass* _pRenderPass;
        std::vector<VertexBufferLayout> _vertexBufferLayouts;
        std::vector<DescriptorSetLayout> _descriptorSetLayouts;
        const Shader* _pVertexShader = nullptr;
        const Shader* _pFragmentShader = nullptr;
        CullMode _cullMode;
        FrontFace _frontFace;
        bool _enableDepthTest = false;
        DepthCompareOperation _depthCmpOp;
        bool _enableColorBlending = false;
        uint32_t _pushConstantSize = 0;
        uint32_t _pushConstantStageFlags = 0;

    public:
        // NOTE: All pipelines currently uses the swapchain's extent as viewport extent
        Pipeline(
            const RenderPass* pRenderPass,
            const std::vector<VertexBufferLayout>& vertexBufferLayouts,
            const std::vector<DescriptorSetLayout>& descriptorLayouts,
            const Shader* pVertexShader,
            const Shader* pFragmentShader,
            CullMode cullMode,
            FrontFace frontFace,
            bool enableDepthTest,
            DepthCompareOperation depthCmpOp,
            bool enableColorBlending, // TODO: more options to handle this..
            uint32_t pushConstantSize,
            uint32_t pushConstantStageFlags
        );
        ~Pipeline();

        void create();
        void destroy();

        inline const std::vector<VertexBufferLayout>& getVertexBufferLayouts() const { return _vertexBufferLayouts; }
        inline const std::vector<DescriptorSetLayout>& getDescriptorSetLayouts() const { return _descriptorSetLayouts; }
        inline const Shader* getVertexShader() const { return _pVertexShader; }
        inline const Shader* getFragmentShader() const { return _pFragmentShader; }
        inline CullMode getCullMode() const { return _cullMode; }
        inline FrontFace getFaceWindingOrder() const { return _frontFace; }
        inline bool isDepthTestEnabled() const { return _enableDepthTest; }
        inline DepthCompareOperation getDepthCompareOperation() const { return _depthCmpOp; }
        inline bool isColorBlendEnabled() const { return _enableColorBlending; }
        inline uint32_t getPushConstantsSize() const { return _pushConstantSize; }
        inline uint32_t getPushConstantsStageFlags() { return _pushConstantStageFlags; }

        inline PipelineImpl* getImpl() const { return _pImpl; }
    };
}
