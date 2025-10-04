#include "platypus/graphics/Framebuffer.hpp"
#include "WebFramebuffer.hpp"
#include "platypus/assets/Texture.h"
#include "platypus/assets/platform/web/WebTexture.h"
#include "WebContext.hpp"
#include "platypus/core/Debug.h"
#include <GL/glew.h>


namespace platypus
{
    // TODO: Deal with attachments individually
    Framebuffer::Framebuffer(
        const RenderPass& renderPass,
        const std::vector<Texture*>& attachments,
        uint32_t width,
        uint32_t height
    ) :
        _width(width),
        _height(height),
        _attachments(attachments)
    {
        uint32_t id = 0;
        GL_FUNC(glGenFramebuffers(1, &id));
        GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, id));

        // NOTE: Not sure if need to also bind the texture(s) here...
        uint32_t depthRenderBuffer;
        GL_FUNC(glGenRenderbuffers(1, &depthRenderBuffer));
        GL_FUNC(glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer));
        GL_FUNC(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height));
        GL_FUNC(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer));

        GL_FUNC(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, attachments[0]->getImpl()->id, 0));
        GL_FUNC(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachments[1]->getImpl()->id, 0));

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
    }

    Framebuffer::~Framebuffer()
    {
        if (_pImpl)
        {
            GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            GL_FUNC(glDeleteRenderbuffers(1, &_pImpl->depthRenderBuffer));
            GL_FUNC(glDeleteFramebuffers(1, &_pImpl->id));
            delete _pImpl;
        }
    }
}
