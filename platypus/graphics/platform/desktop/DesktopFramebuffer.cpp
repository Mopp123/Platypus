#include "platypus/graphics/Framebuffer.hpp"
#include "DesktopFramebuffer.hpp"
#include "platypus/graphics/Device.hpp"
#include "DesktopDevice.hpp"
#include "DesktopRenderPass.h"
#include "platypus/assets/platform/desktop/DesktopTexture.h"
#include "platypus/core/Debug.h"

#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    Framebuffer::Framebuffer(
        const RenderPass& renderPass,
        const std::vector<Texture*>& colorAttachments,
        Texture* pDepthAttachment,
        uint32_t width,
        uint32_t height
    ) :
        _width(width),
        _height(height),
        _colorAttachments(colorAttachments),
        _pDepthAttachment(pDepthAttachment)
    {
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass.getImpl()->handle;

        // NOTE: If color attachments provided, the depth attachment is always the last one
        std::vector<VkImageView> vkAttachments;
        for (Texture* pColorTexture : _colorAttachments)
            vkAttachments.push_back(pColorTexture->getImpl()->imageView);

        if (_pDepthAttachment)
            vkAttachments.push_back(_pDepthAttachment->getImpl()->imageView);

        createInfo.attachmentCount = (uint32_t)vkAttachments.size();
        createInfo.pAttachments = vkAttachments.data();
        createInfo.width = _width;
        createInfo.height = _height;
        createInfo.layers = 1;

        VkFramebuffer handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateFramebuffer(
            Device::get_impl()->device,
            &createInfo,
            nullptr,
            &handle
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Framebuffer::Framebuffer "
                "Failed to create framebuffer! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new FramebufferImpl;
        _pImpl->handle = handle;
    }

    Framebuffer::~Framebuffer()
    {
        if (_pImpl)
        {
            vkDestroyFramebuffer(
                Device::get_impl()->device,
                _pImpl->handle,
                nullptr
            );
            delete _pImpl;
        }
    }
}
