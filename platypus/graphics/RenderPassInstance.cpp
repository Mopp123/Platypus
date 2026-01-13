#include "RenderPassInstance.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    RenderPassInstance::RenderPassInstance(
        const RenderPass& renderPass,
        uint32_t framebufferWidth,
        uint32_t framebufferHeight,
        const TextureSampler& textureSampler,
        bool useWindowDimensions
    ) :
        _renderPassRef(renderPass),
        _framebufferWidth(framebufferWidth),
        _framebufferHeight(framebufferHeight),
        _textureSampler(textureSampler),
        _useWindowDimensions(useWindowDimensions)
    {
    }

    RenderPassInstance::~RenderPassInstance()
    {
        destroy();
    }

    void RenderPassInstance::create()
    {
        if (_useWindowDimensions)
            matchWindowDimensions();

        // TODO: Allow having different dimensions for the framebuffer and attachment
        // (Atm attachment has same dimensions as the framebuffer)
        std::vector<Texture*> colorAttachments;
        if (_renderPassRef.getColorFormat() != ImageFormat::NONE)
        {
            _pColorAttachment = new Texture(
                TextureType::COLOR_TEXTURE,
                _textureSampler,
                _renderPassRef.getColorFormat(),
                _framebufferWidth,
                _framebufferHeight
            );
            colorAttachments.push_back(_pColorAttachment);
            Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pColorAttachment);
        }
        if (_renderPassRef.getDepthFormat() != ImageFormat::NONE)
        {
            _pDepthAttachment = new Texture(
                TextureType::DEPTH_TEXTURE,
                _textureSampler,
                _renderPassRef.getDepthFormat(),
                _framebufferWidth,
                _framebufferHeight
            );
            Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pDepthAttachment);
        }
        // TODO: Allow creating multiple framebuffers for each frame in flight?
        _framebuffers.push_back(
            new Framebuffer(
                _renderPassRef,
                colorAttachments,
                _pDepthAttachment,
                _framebufferWidth,
                _framebufferHeight
            )
        );
    }

    void RenderPassInstance::destroy()
    {
        // TODO: If dimensions don't change, etc don't destroy and recreate
        // unless RenderPassInstance gets destroyed!
        if (_pColorAttachment)
            Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pColorAttachment);
        if (_pDepthAttachment)
            Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pDepthAttachment);

        _pColorAttachment = nullptr;
        _pDepthAttachment = nullptr;

        for (Framebuffer* pFramebuffer : _framebuffers)
            delete pFramebuffer;

        _framebuffers.clear();
    }

    Framebuffer* RenderPassInstance::getFramebuffer(size_t frame) const
    {
        if (_framebuffers.empty())
        {
            Debug::log(
                "@RenderPassInstance::getFramebuffer "
                "No framebuffers exist.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        const size_t framebufferCount = _framebuffers.size();
        if (framebufferCount == 1)
            return _framebuffers[0];

        if (frame >= framebufferCount)
        {
            Debug::log(
                "@RenderPassInstance::getFramebuffer "
                "Requested framebuffer at index: " + std::to_string(frame) + " "
                "framebuffer count was: " + std::to_string(framebufferCount) + ". "
                "Framebuffer count has to be either 1 or "
                "match the count of max frames in flight!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        return _framebuffers[frame];
    }

    void RenderPassInstance::matchWindowDimensions()
    {
        Extent2D swapchainExtent = Application::get_instance()->getSwapchain()->getExtent();
        _framebufferWidth = swapchainExtent.width;
        _framebufferHeight = swapchainExtent.height;
    }
}
