#include "platypus/graphics/Pipeline.h"
#include "DesktopPipeline.h"
#include "platypus/graphics/Buffers.h"
#include "DesktopBuffers.h"
#include "DesktopShader.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/platform/desktop/DesktopDevice.hpp"
#include "platypus/graphics/Context.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "DesktopRenderPass.h"
#include "DesktopDescriptors.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    static VkCullModeFlags to_vk_cull_mode(CullMode mode)
    {
        switch (mode)
        {
            case CullMode::CULL_MODE_NONE: return VK_CULL_MODE_NONE;
            case CullMode::CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
            case CullMode::CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
            default:
                Debug::log("@to_vk_cull_mode Invalid mode", Debug::MessageType::PLATYPUS_ERROR);
        }
        return VK_CULL_MODE_NONE;
    }


    static VkFrontFace to_vk_front_face(FrontFace face)
    {
        switch (face)
        {
            case FrontFace::FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FrontFace::FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
            default:
                Debug::log("@to_vk_face Invalid face", Debug::MessageType::PLATYPUS_ERROR);
        }
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }


    static VkCompareOp to_vk_depth_compare_op(DepthCompareOperation op)
    {
        switch (op)
        {
            case COMPARE_OP_NEVER:              return VK_COMPARE_OP_NEVER;
            case COMPARE_OP_LESS:               return VK_COMPARE_OP_LESS;
            case COMPARE_OP_EQUAL:              return VK_COMPARE_OP_EQUAL;
            case COMPARE_OP_LESS_OR_EQUAL:      return VK_COMPARE_OP_LESS_OR_EQUAL;
            case COMPARE_OP_GREATER:            return VK_COMPARE_OP_GREATER;
            case COMPARE_OP_NOT_EQUAL:          return VK_COMPARE_OP_NOT_EQUAL;
            case COMPARE_OP_GREATER_OR_EQUAL:   return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case COMPARE_OP_ALWAYS:             return VK_COMPARE_OP_ALWAYS;
            default:
                Debug::log(
                    "@to_vk_depth_compare_op Invalid operation: " + std::to_string(op),
                    Debug::MessageType::PLATYPUS_ERROR
                );
        }
        return VK_COMPARE_OP_NEVER;
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
        if (_pImpl)
        {
            if (_pImpl->handle != VK_NULL_HANDLE && _pImpl->layout != VK_NULL_HANDLE)
                destroy();

            delete _pImpl;
        }
    }

    // TODO: Enable depth (and stencil) testing (assign pipelineCreateInfo.pDepthStencilState which is nullptr atm)
    void Pipeline::create()
    {
        VkDevice device = Device::get_impl()->device;
        // Get vulkan handles from out vertexBufferLayouts
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttribDescriptions;

        for (const VertexBufferLayout& layout : _vertexBufferLayouts)
        {
            vertexBindingDescriptions.push_back(layout._pImpl->bindingDescription);
            for (const VertexBufferElement& element : layout.getElements())
                vertexAttribDescriptions.push_back(element._pImpl->attribDescription);
        }
        // Configure programmable stages
        // NOTE: Atm vertex and fragment shaders are REQUIRED!
        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
            get_pipeline_shader_stage_create_info(_pVertexShader, _pVertexShader->_pImpl),
            get_pipeline_shader_stage_create_info(_pFragmentShader, _pFragmentShader->_pImpl)
        };

        // Fixed function stages

        // Vertex input (describes vertex data inputted to vertex shader)
        // ------------
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions.size();
        vertexInputCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
        vertexInputCreateInfo.vertexAttributeDescriptionCount = (uint32_t)vertexAttribDescriptions.size();
        vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttribDescriptions.data();

        // Input assembly (describes what kind of geometry we'll draw)
        // --------------
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        // Specify viewport
        // TODO: Allow specifying other than swapchain's extent
        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        Extent2D swapchainExtent = swapchain.getExtent();
        Rect2D viewportScissor = {
            0, 0, swapchainExtent.width, swapchainExtent.height
        };
        // ----------------
        // NOTE: Flipping the viewpot height here to have y point up so
        // it's consistent with opengl and how gltf files' vertices go
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = (float)swapchainExtent.height;
        viewport.width = (float)swapchainExtent.width;
        viewport.height = -((float)swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkPipelineViewportStateCreateInfo viewportCreateInfo{};
        viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports = &viewport;
        viewportCreateInfo.scissorCount = 1;
        VkRect2D vkScissor{
            { viewportScissor.offsetX, viewportScissor.offsetY },
            { viewportScissor.width, viewportScissor.height }
        };
        viewportCreateInfo.pScissors = &vkScissor;

        // Rasterization
        // -------------
        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizationCreateInfo.depthClampEnable = VK_FALSE; // useful for shadowmaps? (doesnt discard outside min and max depths)
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE; // disables output to framebuffer
        // *"in what way we rasterize" (lines, points, fill)(*any other than filling requires enabling GPU feature!)
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.lineWidth = 1.0f;
        // *Face culling
        rasterizationCreateInfo.cullMode = to_vk_cull_mode(_cullMode);
        rasterizationCreateInfo.frontFace = to_vk_front_face(_frontFace);
        // *Can be used to help with shadowmapping (bias thing to prevent acne?)
        rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

        // Multisampling ("Enabling" this requires enabling also GPU feature!) (DISABLED ATM)
        // -------------
        VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
        multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleCreateInfo.minSampleShading = 1.0f;
        multisampleCreateInfo.pSampleMask = nullptr;
        multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

        // Depth and stencil testing
        //--------------------------
        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
        if (_enableDepthTest)
        {
            depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilCreateInfo.depthTestEnable = VK_TRUE;
            depthStencilCreateInfo.depthWriteEnable = _enableDepthWrite ? VK_TRUE : VK_FALSE;
            depthStencilCreateInfo.depthCompareOp = to_vk_depth_compare_op(_depthCmpOp);
            depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
            depthStencilCreateInfo.minDepthBounds = 0.0f; // Optional
            depthStencilCreateInfo.maxDepthBounds = 1.0f; // Optional
            depthStencilCreateInfo.stencilTestEnable = VK_FALSE; // DISABLED ATM!
            depthStencilCreateInfo.front = {}; // Optional (*related to stencilTestEnable)
            depthStencilCreateInfo.back = {}; // Optional (*related to stencilTestEnable)
        }
        else
        {
            /*
            Debug::log(
                "@Pipeline::create "
                "Depth testing was disabled for pipeline but currently "
                "using only a single render pass which requires pipeline to "
                "configure VkPipelineDepthStencilStateCreateInfo! "
                "...sorry for the inconveniance:D",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            */
        }

        // Color blending (Theres 2 ways to do blending -> "blendEnable" or "logicOpEnable")
        //---------------
        // *if blending enabled -> configure those things here..
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        if (_enableColorBlending)
        {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = _enableDepthWrite ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
        colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendCreateInfo.attachmentCount = 1;
        colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

        // Dynamic state
        //--------------
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        // Pipeline layout (push constants and uniforms)
        //----------------
        VkPipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // TODO: Descriptors
        // *Uniforms:
        layoutCreateInfo.setLayoutCount = (uint32_t)_descriptorSetLayouts.size();
        // *Combine the VkDescriptorLayouts from the descriptor layouts into a single contiguous container
        std::vector<VkDescriptorSetLayout> vkDescSetLayouts;
        for (int i = 0; i < _descriptorSetLayouts.size(); ++i)
            vkDescSetLayouts.push_back(_descriptorSetLayouts[i].getImpl()->handle);
        // *Push constants:
        VkPushConstantRange pushConstantRange{};

        if (_pushConstantSize > 0)
        {
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
            pushConstantRange.offset = 0;
            pushConstantRange.size = _pushConstantSize;
            VkShaderStageFlags vkPushConstantStageFlags = to_vk_shader_stage_flags(_pushConstantStageFlags);
            pushConstantRange.stageFlags = vkPushConstantStageFlags;

            layoutCreateInfo.pushConstantRangeCount = 1; // allow just 1 range atm..
            layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        }

        // Assign these both..
        layoutCreateInfo.pSetLayouts = vkDescSetLayouts.data();

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkResult createPipelineLayoutResult = vkCreatePipelineLayout(
            device,
            &layoutCreateInfo,
            nullptr,
            &pipelineLayout
        );
        if (createPipelineLayoutResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createPipelineLayoutResult));
            Debug::log(
                "@Pipeline::create "
                "Failed to create VkPipelineLayout! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // Finally the pipeline creation..
        //--------------------------------
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStageCreateInfos;
        pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState = _enableDepthTest ? &depthStencilCreateInfo: nullptr;
        pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = pipelineLayout;

        pipelineCreateInfo.renderPass = _pRenderPass->getImpl()->handle;
        pipelineCreateInfo.subpass = 0;

        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkResult createResult = vkCreateGraphicsPipelines(
            device,
            VK_NULL_HANDLE,
            1,
            &pipelineCreateInfo,
            nullptr,
            &pipeline
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Pipeline::create "
                "Failed to create pipeline! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // NOTE: Not sure do we actually need the layout after creation
        _pImpl->layout = pipelineLayout;
        _pImpl->handle = pipeline;

        Debug::log("Pipeline created");
    }

    void Pipeline::destroy()
    {
        VkDevice device = Device::get_impl()->device;
        vkDestroyPipeline(device, _pImpl->handle, nullptr);
        vkDestroyPipelineLayout(device, _pImpl->layout, nullptr);
        _pImpl->handle = VK_NULL_HANDLE;
        _pImpl->layout = VK_NULL_HANDLE;
    }
}
