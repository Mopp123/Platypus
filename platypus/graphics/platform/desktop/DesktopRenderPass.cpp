#include "platypus/graphics/RenderPass.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/assets/platform/desktop/DesktopTexture.h"
#include "DesktopDevice.hpp"
#include "DesktopRenderPass.h"
#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    RenderPass::RenderPass(
        RenderPassType type,
        bool offscreen
    ) :
        _type(type),
        _offscreen(offscreen)
    {
        _pImpl = new RenderPassImpl;
    }

    RenderPass::~RenderPass()
    {
        // NOTE: RenderPass's "vk state" is managed by Swapchain so the swapchain is
        // responsible for calling the RenderPass's destroy() when necessary and on exit!
        if (_pImpl)
            delete _pImpl;
    }

    void RenderPass::create(
        ImageFormat colorFormat,
        ImageFormat depthFormat,
        bool clearColorAttachment,
        bool clearDepthAttachment,
        bool continueAttachmentUsage,
        bool continueDepthAttachmentUsage
    )
    {
        _colorFormat = colorFormat;
        _depthFormat = depthFormat;

        std::vector<VkAttachmentDescription> attachmentDescriptions;
        std::vector<VkAttachmentReference> colorAttachmentReferences;
        VkAttachmentReference depthAttachmentReference{};
        VkImageLayout initialColorImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout initialDepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout finalColorImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout finalDepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (_colorFormat != ImageFormat::NONE)
        {
            VkAttachmentDescription colorAttachmentDescription{};
            colorAttachmentDescription.format = to_vk_format(_colorFormat);
            colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

            if (clearColorAttachment)
                colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            else
                colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

            colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            finalColorImageLayout = _offscreen ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            if (continueAttachmentUsage)
            {
                initialColorImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                //finalColorImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            colorAttachmentDescription.initialLayout = initialColorImageLayout;
            colorAttachmentDescription.finalLayout = finalColorImageLayout;
            attachmentDescriptions.push_back(colorAttachmentDescription);

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            // NOTE: Previously used VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL here.
            // Using newer Vulkan version with different driver this gives validation error
            // that you'd need to use synchronization2 extension for this to work.
            // Got this fixed by rather using VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentReferences.push_back(colorAttachmentRef);
        }


        if (_depthFormat != ImageFormat::NONE)
        {
            VkAttachmentDescription depthAttachmentDescription{};
            depthAttachmentDescription.format = to_vk_format(_depthFormat);
            depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

            if (clearDepthAttachment)
                depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            else
                depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

            depthAttachmentDescription.storeOp = _offscreen ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            //  For continuing using some previous pass's depth attachment
            if (continueDepthAttachmentUsage)
            {
                initialDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                finalDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            else
            {
                finalDepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            depthAttachmentDescription.initialLayout = initialDepthImageLayout;
            depthAttachmentDescription.finalLayout = finalDepthImageLayout;
            attachmentDescriptions.push_back(depthAttachmentDescription);

            // depth attachment is always the last atm...
            depthAttachmentReference.attachment = (uint32_t)colorAttachmentReferences.size();
            depthAttachmentReference.layout = finalDepthImageLayout;
        }

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = (uint32_t)colorAttachmentReferences.size();
        subpassDescription.pColorAttachments = _colorFormat != ImageFormat::NONE ? colorAttachmentReferences.data() : nullptr;
        subpassDescription.pDepthStencilAttachment = _depthFormat != ImageFormat::NONE ? &depthAttachmentReference : nullptr;

        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // here we specify which operations to wait on
        subpassDependency.srcAccessMask = 0; // no fukin idea what this is..
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        // Not really sure what kind of shit should be specified here:D seems to work...
        if (_offscreen)
		    subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // NOTE: Did earlier offscreen rendering using below from some stack overflow post.
        // Realized that I'm not using subpasses at all since I have completely different render pass for that.
        // TODO: Maybe utilize subpasses in the future to render into multiple attachments within single
        // renderpass. (Quite useful for deferred rendering?)
        //if (offscreenTarget)
        //{
        //    VkSubpassDependency subpassDependency1{};
        //    subpassDependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
		//    subpassDependency1.dstSubpass = 0;
		//    subpassDependency1.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		//    subpassDependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		//    subpassDependency1.srcAccessMask = VK_ACCESS_NONE_KHR;
		//    subpassDependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//    subpassDependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        //    VkSubpassDependency subpassDependency2{};
		//    subpassDependency2.srcSubpass = 0;
		//    subpassDependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
		//    subpassDependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		//    subpassDependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		//    subpassDependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//    subpassDependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		//    subpassDependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        //    subpassDependencies[0] = subpassDependency1;
        //    subpassDependencies[1] = subpassDependency2;
        //}

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = (uint32_t)attachmentDescriptions.size();
        createInfo.pAttachments = attachmentDescriptions.data();
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpassDescription;

        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &subpassDependency;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkResult createResult = vkCreateRenderPass(
            Device::get_impl()->device,
            &createInfo,
            nullptr,
            &renderPass
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@RenderPass::create "
                "Failed to create RenderPass! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl->handle = renderPass;
        _pImpl->initialColorImageLayout = initialColorImageLayout;
        _pImpl->initialDepthImageLayout = initialDepthImageLayout;
        _pImpl->finalColorImageLayout = finalColorImageLayout;
        _pImpl->finalDepthImageLayout = finalDepthImageLayout;

        Debug::log("RenderPass created");
    }

    void RenderPass::destroy()
    {
        vkDestroyRenderPass(Device::get_impl()->device, _pImpl->handle, nullptr);
        _pImpl->handle = VK_NULL_HANDLE;
    }
}
