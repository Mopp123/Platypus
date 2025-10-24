#pragma once

#include "platypus/assets/Image.h"
#include <string>


namespace platypus
{
    enum class RenderPassType
    {
        SCENE_PASS,
        SHADOW_PASS
    };

    static inline std::string render_pass_type_to_string(RenderPassType type)
    {
        switch (type)
        {
            case RenderPassType::SCENE_PASS:    return "SCENE_PASS";
            case RenderPassType::SHADOW_PASS:   return "SHADOW_PASS";
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
        bool _offscreen = false;

    public:
        RenderPass(RenderPassType type, bool offscreen);
        ~RenderPass();

        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat
        );

        void destroy();

        inline RenderPassType getType() const { return _type; }
        inline bool isOffscreenPass() const { return _offscreen; }
        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
