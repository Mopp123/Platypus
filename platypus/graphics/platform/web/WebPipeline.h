#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "WebShader.h"


namespace platypus
{
    struct PipelineImpl
    {
        float viewportWidth = 0.0f;
        float viewportHeight = 0.0f;

        OpenglShaderProgram* pShaderProgram = nullptr;

        // Just testing atm if could get rid of the annoyting UniformInfo structs
        int useLocationIndex = 0;
    };
}
