#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "WebShader.hpp"


namespace platypus
{
    struct PipelineImpl
    {
        float viewportWidth = 0.0f;
        float viewportHeight = 0.0f;

        OpenglShaderProgram* pShaderProgram = nullptr;

        // Just testing atm if could get rid of the annoyting UniformInfo structs or at least the
        // requirement to specify uniform locations
        // NOTE:
        //  *Force pushing constants before binding descriptor sets.
        //  constantsPushed gets set to false after binding descriptor sets.
        //  firstDescriptorSetLocation gets set to 0 when binding pipeline.
        bool constantsPushed = false;
        int firstDescriptorSetLocation = 0;

        std::set<uint32_t> boundUniformBuffers;
    };
}
