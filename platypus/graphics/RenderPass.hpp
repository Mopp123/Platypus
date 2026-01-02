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


    enum RenderPassAttachmentUsageFlagBits
    {
        RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE = 0x1,
        RENDER_PASS_ATTACHMENT_USAGE_COLOR_CONTINUE = 0x1 << 1,
        RENDER_PASS_ATTACHMENT_USAGE_DEPTH_DISCRETE = 0x1 << 2,
        RENDER_PASS_ATTACHMENT_USAGE_DEPTH_CONTINUE = 0x1 << 3
    };

    enum RenderPassAttachmentClearFlagBits
    {
        RENDER_PASS_ATTACHMENT_CLEAR_COLOR = 0x1,
        RENDER_PASS_ATTACHMENT_CLEAR_DEPTH = 0x1 << 1
    };

    class Swapchain;

    // TODO: Get rid of this stuff and start using 1.4's dynamic rendering?
    struct RenderPassImpl;
    class RenderPass
    {
    private:
        RenderPassImpl* _pImpl = nullptr;
        RenderPassType _type;
        ImageFormat _colorFormat = ImageFormat::NONE;
        ImageFormat _depthFormat = ImageFormat::NONE;
        bool _offscreen = false;
        // *Meaning does this render pass continue using previous render pass's attachments
        uint32_t _attachmentUsageFlags = 0;
        uint32_t _attachmentClearFlags = 0;

    public:
        // TODO: Args how attachments will be used?
        // Will attachments be cleared, are we continuing using some previous pass's attachments, etc?
        RenderPass(
            RenderPassType type,
            bool offscreen,
            uint32_t attachmentUsageFlags,
            uint32_t attachmentClearFlags
        );
        ~RenderPass();

        // continueAttachmentUsage is for passes that continue using some
        // previous pass' attachments instead of having its' own dedicated
        // attachments.
        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat
        );
        void destroy();

        inline RenderPassType getType() const { return _type; }
        inline ImageFormat getColorFormat() const { return _colorFormat; }
        inline ImageFormat getDepthFormat() const { return _depthFormat; }
        inline bool isOffscreenPass() const { return _offscreen; }
        inline uint32_t getAttachmentUsageFlags() const { return _attachmentUsageFlags; }
        inline uint32_t getAttachmentClearFlags() const { return _attachmentClearFlags; }
        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
