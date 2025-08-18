#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "WebShader.h"


namespace platypus
{
    struct PipelineImpl
    {
        std::vector<VertexBufferLayout> vertexBufferLayouts;
        std::vector<DescriptorSetLayout> descriptorSetLayouts;
        float viewportWidth = 0.0f;
        float viewportHeight = 0.0f;

        OpenglShaderProgram* pShaderProgram = nullptr;

        CullMode cullMode;
        FrontFace frontFace;

        bool enableDepthTest = false;
        DepthCompareOperation depthCmpOp;

        bool enableColorBlending = false;

        // Just testing atm if could get rid of the annoyting UniformInfo structs
        int useLocationIndex = 0;
    };
}
