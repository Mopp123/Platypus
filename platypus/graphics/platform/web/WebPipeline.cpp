#include "platypus/graphics/Pipeline.h"
#include "WebPipeline.h"
#include "WebShader.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    Pipeline::Pipeline()
    {
        _pImpl = new PipelineImpl;
    }

    Pipeline::~Pipeline()
    {
        destroy();
        delete _pImpl;
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
        if (_pImpl->pShaderProgram)
        {
            Debug::log(
                "@Pipeline::create "
                "Attempted to create pipeline but shader program already exists."
                "destroy() required before recreating pipeline.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (pushConstantSize > PLATYPUS_MAX_PUSH_CONSTANTS_SIZE)
        {
            Debug::log(
                "@Pipeline::create "
                "Push constants size too big: " + std::to_string(pushConstantSize) + " "
                "Maximum size is " + std::to_string(PLATYPUS_MAX_PUSH_CONSTANTS_SIZE),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl->vertexBufferLayouts = vertexBufferLayouts;
        _pImpl->descriptorSetLayouts = descriptorLayouts;
        _pImpl->viewportWidth = viewportWidth;
        _pImpl->viewportHeight = viewportHeight;
        // NOTE: on web platform we aren't using layout qualifiers
        _pImpl->pShaderProgram = new OpenglShaderProgram(
            ShaderVersion::ESSL1,
            (const ShaderImpl*)vertexShader._pImpl,
            (const ShaderImpl*)fragmentShader._pImpl
        );
        _pImpl->cullMode = cullMode;
        _pImpl->frontFace = frontFace;
        _pImpl->enableDepthTest = enableDepthTest;
        _pImpl->depthCmpOp = depthCmpOp;
        _pImpl->enableColorBlending = enableColorBlending;
    }

    void Pipeline::destroy()
    {
        if (_pImpl)
        {
            _pImpl->vertexBufferLayouts.clear();
            _pImpl->descriptorSetLayouts.clear();

            delete _pImpl->pShaderProgram;
            _pImpl->pShaderProgram = nullptr;
        }
    }
}
