#pragma once

#include "platypus/assets/Image.h"
#include <vector>
#include <string>


namespace platypus
{
    enum class RenderPassType
    {
        SHADOW_PASS,
        OPAQUE_PASS,
        TRANSPARENT_PASS,
        SCREEN_PASS
    };

    static inline std::string render_pass_type_to_string(RenderPassType type)
    {
        switch (type)
        {
            case RenderPassType::SHADOW_PASS:    return "SHADOW_PASS";
            case RenderPassType::OPAQUE_PASS:   return "OPAQUE_PASS";
            case RenderPassType::TRANSPARENT_PASS:   return "TRANSPARENT_PASS";
            case RenderPassType::SCREEN_PASS:   return "SCREEN_PASS";
            default:                            return "<Invalid RenderPassType>";
        }
    }


    class Swapchain;

    struct RenderPassImpl;
    class RenderPass
    {
    private:
        RenderPassImpl* _pImpl = nullptr;
        RenderPassType _type;
        ImageFormat _colorFormat = ImageFormat::NONE;
        ImageFormat _depthFormat = ImageFormat::NONE;
        bool _offscreen = false;

    public:
        // TODO: Args how attachments will be used?
        // Will attachments be cleared, are we continuing using some previous pass's attachments, etc?
        RenderPass(
            RenderPassType type,
            bool offscreen
        );
        ~RenderPass();

        // continueAttachmentUsage is for passes that continue using some
        // previous pass' attachments instead of having its' own dedicated
        // attachments.
        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat,
            bool clearColorAttachment = true,
            bool clearDepthAttachment = true,
            bool continueAttachmentUsage = false
        );
        void destroy();

        inline RenderPassType getType() const { return _type; }
        inline ImageFormat getColorFormat() const { return _colorFormat; }
        inline ImageFormat getDepthFormat() const { return _depthFormat; }
        inline bool isOffscreenPass() const { return _offscreen; }
        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
