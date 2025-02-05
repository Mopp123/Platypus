#include "platypus/graphics/RenderPass.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Context.h"
#include "DesktopRenderPass.h"
#include "DesktopContext.h"
#include "DesktopSwapchain.h"
#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    RenderPass::RenderPass()
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

    void RenderPass::create(const Swapchain& swapchain)
    {
        if (!swapchain._pImpl)
        {
            Debug::log(
                "@RenderPass::create "
                "Swapchain not initialized!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        VkFormat colorImageFormat = swapchain._pImpl->imageFormat;

        VkAttachmentDescription colorAttachmentDescription{};
        colorAttachmentDescription.format = colorImageFormat;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkFormat depthImageFormat = swapchain._pImpl->depthImageFormat;
        VkAttachmentDescription depthAttachmentDescription{};
        depthAttachmentDescription.format = depthImageFormat;
        depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentRef;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency subPassDependency{};
        subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subPassDependency.dstSubpass = 0;
        subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // here we specify which operations to wait on
        subPassDependency.srcAccessMask = 0; // no fukin idea what this is..
        subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkAttachmentDescription renderPassAttachments[] = { colorAttachmentDescription, depthAttachmentDescription };

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = renderPassAttachments;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpassDescription;

        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &subPassDependency;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkResult createResult = vkCreateRenderPass(
            Context::get_instance()->getImpl()->device,
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

        Debug::log("RenderPass created");
    }

    void RenderPass::destroy()
    {
        vkDestroyRenderPass(Context::get_instance()->getImpl()->device, _pImpl->handle, nullptr);
        _pImpl->handle = VK_NULL_HANDLE;
    }
}
