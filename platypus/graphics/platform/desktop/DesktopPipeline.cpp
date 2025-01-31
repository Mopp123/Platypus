#include "platypus/graphics/Pipeline.h"
#include "DesktopPipeline.h"
#include "platypus/graphics/Buffers.h"
#include "DesktopBuffers.h"
#include "DesktopShader.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "platypus/core/Debug.h"
#include "DesktopRenderPass.h"
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

    Pipeline::Pipeline()
    {
        _pImpl = new PipelineImpl;
    }

    Pipeline::~Pipeline()
    {
        if (_pImpl)
            delete _pImpl;
    }

    // TODO: Implement descriptor sets (configure pipeline layout)
    // TODO: Enable depth (and stencil) testing (assign pipelineCreateInfo.pDepthStencilState which is nullptr atm)
    void Pipeline::create(
        const RenderPass& renderPass,
        const std::vector<VertexBufferLayout>& vertexBufferLayouts,
        //const std::vector<DescriptorSetLayout>& descriptorLayouts,
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
        // Get vulkan handles from out vertexBufferLayouts
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttribDescriptions;

        for (const VertexBufferLayout& layout : vertexBufferLayouts)
        {
            vertexBindingDescriptions.push_back(layout._pImpl->bindingDescription);
            for (const VertexBufferElement& element : layout.getElements())
                vertexAttribDescriptions.push_back(element._pImpl->attribDescription);
        }
        // Configure programmable stages
        // NOTE: Atm vertex and fragment shaders are REQUIRED!
        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
            get_pipeline_shader_stage_create_info(vertexShader, vertexShader._pImpl),
            get_pipeline_shader_stage_create_info(fragmentShader, fragmentShader._pImpl)
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
        // ----------------
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = viewportWidth;
        viewport.height = viewportHeight;
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
        rasterizationCreateInfo.cullMode = to_vk_cull_mode(cullMode);
        rasterizationCreateInfo.frontFace = to_vk_front_face(frontFace);
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
        /*
        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
        depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE; // DISABLED ATM!
        depthStencilCreateInfo.minDepthBounds = 0.0f; // Optional
        depthStencilCreateInfo.maxDepthBounds = 1.0f; // Optional
        depthStencilCreateInfo.stencilTestEnable = VK_FALSE; // DISABLED ATM!
        depthStencilCreateInfo.front = {}; // Optional (*related to stencilTestEnable)
        depthStencilCreateInfo.back = {}; // Optional (*related to stencilTestEnable)
        */

        // Color blending (Theres 2 ways to do blending -> "blendEnable" or "logicOpEnable")
        //---------------
        // *if blending enabled -> configure those things here..
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        if (enableColorBlending)
        {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
        colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendCreateInfo.attachmentCount = 1;
        colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

        // Dynamic state (DISABLED ATM)
        //--------------
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};

        // Pipeline layout (push constants and uniforms)
        //----------------
        VkPipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // TODO: Descriptors
        // *Uniforms:
        //layoutCreateInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
        // *Combine the VkDescriptorLayouts from the descriptor layouts into a single contiguous container
        /*
        std::vector<VkDescriptorSetLayout> vkDescSetLayouts;
        for (int i = 0; i < descriptorLayouts.size(); ++i)
        {
            VkDescriptorSetLayout vkLayout = descriptorLayouts[i].getVkDescriptorSetLayout();
            vkDescSetLayouts.push_back(vkLayout);
        }
        */
        // *Push constants:
        VkPushConstantRange pushConstantRange{};

        if (pushConstantSize > 0)
        {
            pushConstantRange.offset = 0;
            pushConstantRange.size = pushConstantSize;
            VkShaderStageFlags vkPushConstantStageFlags = to_vk_shader_stage_flags(pushConstantStageFlags);
            pushConstantRange.stageFlags = vkPushConstantStageFlags;

            layoutCreateInfo.pushConstantRangeCount = 1; // allow just 1 range atm..
            layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        }

        // Assign these both..
        //layoutCreateInfo.pSetLayouts = vkDescSetLayouts.data();

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkResult createPipelineLayoutResult = vkCreatePipelineLayout(
            Context::get_pimpl()->device,
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
        pipelineCreateInfo.pDepthStencilState = nullptr; //&depthStencilCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
        pipelineCreateInfo.pDynamicState = nullptr; // not used atm!
        pipelineCreateInfo.layout = pipelineLayout;

        pipelineCreateInfo.renderPass = renderPass.getImpl()->handle;
        pipelineCreateInfo.subpass = 0;

        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkResult createResult = vkCreateGraphicsPipelines(
            Context::get_pimpl()->device,
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
        VkDevice device = Context::get_pimpl()->device;
        vkDestroyPipeline(device, _pImpl->handle, nullptr);
        vkDestroyPipelineLayout(device, _pImpl->layout, nullptr);
        _pImpl->handle = VK_NULL_HANDLE;
        _pImpl->layout = VK_NULL_HANDLE;
    }
}
