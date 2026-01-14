#include "platypus/graphics/Framebuffer.hpp"
#include "WebFramebuffer.hpp"
#include "platypus/assets/Texture.hpp"
#include "platypus/assets/platform/web/WebTexture.hpp"
#include "WebContext.hpp"
#include "platypus/core/Debug.hpp"
#include <GL/glew.h>

#include <unordered_map>


namespace platypus
{
    // First = attachment asset ID, second = framebuffer ID
    std::unordered_map<ID_t, uint32_t> s_boundFramebufferAttachments;

    // TODO: Deal with attachments individually
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
        if (colorAttachments.size() > 1)
        {
            Debug::log(
                "@Framebuffer::Framebuffer "
                "Provided " + std::to_string(_colorAttachments.size()) + " color attachments. "
                "The web implementation currently supports only a single color attachment!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        uint32_t id = 0;
        GL_FUNC(glGenFramebuffers(1, &id));
        GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, id));

        if (_colorAttachments.size() > 0)
            GL_FUNC(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorAttachments[0]->getImpl()->id, 0));

        ID_t originalDepthAttachmentAssetID = NULL_ID;
        Texture* pCopyDepthAttachment = nullptr;
        uint32_t copySourceID = 0;
        uint32_t depthRenderBuffer = 0;
        if (_pDepthAttachment)
        {
            // If another framebuffer is using the depth attachment, make a copy in which we
            // blit the original when we begin render pass using this framebuffer.
            //  -> so we can sample that attachment
            Texture* pUseDepthAttachment = _pDepthAttachment;
            std::unordered_map<ID_t, uint32_t>::const_iterator existingIt = s_boundFramebufferAttachments.find(_pDepthAttachment->getID());
            if (existingIt != s_boundFramebufferAttachments.end())
            {
                // TODO: Some way to get sampler from input attachments
                TextureSampler sampler(
                    TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR,
                    TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                    false,
                    0
                );
                pCopyDepthAttachment = new Texture(
                    TextureType::DEPTH_TEXTURE,
                    sampler,
                    _pDepthAttachment->getImageFormat(),
                    width,
                    height
                );
                pUseDepthAttachment = pCopyDepthAttachment;
                copySourceID = existingIt->second;
            }
            else
            {
                originalDepthAttachmentAssetID = _pDepthAttachment->getID();
                s_boundFramebufferAttachments[originalDepthAttachmentAssetID] = id;
            }

            GL_FUNC(glGenRenderbuffers(1, &depthRenderBuffer));
            GL_FUNC(glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer));
            GL_FUNC(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height));
            GL_FUNC(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer));
            GL_FUNC(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pUseDepthAttachment->getImpl()->id, 0));
        }

        // Set the list of draw buffers.
        GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        GL_FUNC(glDrawBuffers(1, drawBuffers));

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Debug::log(
                "@Framebuffer::Framebuffer "
                "Failed to create Framebuffer!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        GL_FUNC(glBindRenderbuffer(GL_RENDERBUFFER, 0));
        GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        _pImpl = new FramebufferImpl;
        _pImpl->id = id;
        _pImpl->depthRenderBuffer = depthRenderBuffer;

        _pImpl->originalDepthAttachmentAssetID = originalDepthAttachmentAssetID;
        _pImpl->copySourceID = copySourceID;
        _pImpl->pCopyDepthAttachment = pCopyDepthAttachment;
    }

    Framebuffer::~Framebuffer()
    {
        if (_pImpl)
        {
            if (_pImpl->pCopyDepthAttachment)
            {
                delete _pImpl->pCopyDepthAttachment;
            }
            else
            {
                if (_pImpl->originalDepthAttachmentAssetID != NULL_ID)
                {
                    if (s_boundFramebufferAttachments.erase(_pImpl->originalDepthAttachmentAssetID) != 1)
                    {
                        Debug::log(
                            "@Framebuffer::~Framebuffer "
                            "Failed to erase _pImpl->originalDepthAttachmentAssetID from "
                            "s_boundFramebufferAttachments(size = " + std::to_string(s_boundFramebufferAttachments.size()) + ")",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                    }
                }
            }

            GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            GL_FUNC(glDeleteRenderbuffers(1, &_pImpl->depthRenderBuffer));
            GL_FUNC(glDeleteFramebuffers(1, &_pImpl->id));
            delete _pImpl;
        }
    }
}
