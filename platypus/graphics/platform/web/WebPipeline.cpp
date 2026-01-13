#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Context.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "WebPipeline.h"
#include "WebContext.hpp"

#include <GL/glew.h>


namespace platypus
{
    static void validate_single_buffer_per_descriptor_set(
        const std::vector<DescriptorSetLayoutBinding>& layoutBindings
    )
    {
        int bufferCount = 0;
        for (const DescriptorSetLayoutBinding& binding : layoutBindings)
        {
            DescriptorType type = binding.getType();
            if (type == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                type == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
            {
                ++bufferCount;
            }
        }
        if (bufferCount > 1)
        {
            Debug::log(
                "@validate_single_buffer_per_descriptor_set(@WebPipeline) "
                "Web implementation can currentlyhendle only a single uniform buffer "
                "per descriptor set. Descriptor set layout had " + std::to_string(bufferCount) + " "
                "bindings for uniform buffers!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    Pipeline::Pipeline(
        const RenderPass* pRenderPass,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        const std::vector<DescriptorSetLayout>& descriptorLayouts,
        const Shader* pVertexShader,
        const Shader* pFragmentShader,
        CullMode cullMode,
        FrontFace frontFace,
        bool enableDepthTest,
        bool enableDepthWrite,
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
        _enableDepthWrite(enableDepthWrite),
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

        OpenglShaderProgram* pShaderProgram = new OpenglShaderProgram(
            ShaderVersion::OPENGLES_GLSL_300,
            (const ShaderImpl*)_pVertexShader->_pImpl,
            (const ShaderImpl*)_pFragmentShader->_pImpl
        );
        _pImpl->pShaderProgram = pShaderProgram;

        // Specify uniform block binding points
        // NOTE: This currently works only because allowing single uniform buffer
        // per descriptor set!
        //  -> No way of specifying buffers per set AND per binding?
        //      -> UNLESS: have some naming convention where able to parse glsl
        //      in a way that it's possible to have per set and binding
        size_t blockBindingPoint = 0;
        for (const DescriptorSetLayout& layout : _descriptorSetLayouts)
        {
            #ifdef PLATYPUS_DEBUG
            validate_single_buffer_per_descriptor_set(layout.getBindings());
            #endif
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
