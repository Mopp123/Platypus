#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Context.hpp"
#include "platypus/core/Application.h"
#include "WebPipeline.h"
#include "WebContext.hpp"
#include "platypus/core/Debug.h"

#include <GL/glew.h>


namespace platypus
{
    Pipeline::Pipeline(
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
    ) :
        _pRenderPass(pRenderPass),
        _vertexBufferLayouts(vertexBufferLayouts),
        _descriptorSetLayouts(descriptorLayouts),
        _pVertexShader(pVertexShader),
        _pFragmentShader(pFragmentShader),
        _cullMode(cullMode),
        _frontFace(frontFace),
        _enableDepthTest(enableDepthTest),
        _depthCmpOp(depthCmpOp),
        _enableColorBlending(enableColorBlending),
        _pushConstantSize(pushConstantSize),
        _pushConstantStageFlags(pushConstantStageFlags)
    {
        _pImpl = new PipelineImpl;
    }

    Pipeline::~Pipeline()
    {
        destroy();
        delete _pImpl;
    }

    void Pipeline::create(
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
        if (_pushConstantSize > PLATYPUS_MAX_PUSH_CONSTANTS_SIZE)
        {
            Debug::log(
                "@Pipeline::create "
                "Push constants size too big: " + std::to_string(_pushConstantSize) + " "
                "Maximum size is " + std::to_string(PLATYPUS_MAX_PUSH_CONSTANTS_SIZE),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        Extent2D swapchainExtent = swapchain.getExtent();
        _pImpl->viewportWidth = swapchainExtent.width;
        _pImpl->viewportHeight = swapchainExtent.height;
        OpenglShaderProgram* pShaderProgram = new OpenglShaderProgram(
            ShaderVersion::ESSL3,
            (const ShaderImpl*)_pVertexShader->_pImpl,
            (const ShaderImpl*)_pFragmentShader->_pImpl
        );
        _pImpl->pShaderProgram = pShaderProgram;

        size_t blockBindingPoint = 0;
        for (const DescriptorSetLayout& layout : _descriptorSetLayouts)
        {
            // NOTE: This currently works only because allowing single uniform buffer
            // per descriptor set!
            for (const DescriptorSetLayoutBinding& binding : layout.getBindings())
            {
                DescriptorType type = binding.getType();
                if (type == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    type == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
                {
                    uint32_t shaderBlockIndex = pShaderProgram->getUniformBlockIndex(blockBindingPoint);
                    GL_FUNC(glUniformBlockBinding(_pImpl->pShaderProgram->getID(), shaderBlockIndex, (uint32_t)blockBindingPoint));
                    ++blockBindingPoint;
                    break;
                }
            }
        }

    }

    void Pipeline::destroy()
    {
        if (_pImpl)
        {
            delete _pImpl->pShaderProgram;
            _pImpl->pShaderProgram = nullptr;
        }
    }
}
