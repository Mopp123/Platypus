#include "Texture.hpp"

namespace platypus
{
    std::string texture_sampler_filter_mode_to_string(TextureSamplerFilterMode mode)
    {
        switch (mode)
        {
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR : return "SAMPLER_FILTER_MODE_LINEAR";
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR: return "SAMPLER_FILTER_MODE_NEAR";
        }
    }

    std::string texture_sampler_address_mode_to_string(TextureSamplerAddressMode mode)
    {
        switch (mode)
        {
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT: return "SAMPLER_ADDRESS_MODE_REPEAT";
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return "SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE";
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return "SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER";
        }
    }
}
