#pragma once

#include "Buffers.h"


namespace platypus
{
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_NONE = 0x0,
        DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 0x1,
        DESCRIPTOR_TYPE_UNIFORM_BUFFER = 0x2
    };


    /*  Specifies Uniform buffer struct's layout
     *  NOTE: This is only used by web platform (webgl1/ES2)
     *  since no actual uniform buffers available
     *
     *  Example:
     *
     *  struct DirectionalLight
     *  {
     *      vec3 direction;
     *      vec3 color;
     *  };
     *
     *  above should specify UniformBufferInfo's layout as:
     *  {
     *      ShaderDataType::Float3, ShaderDataType::Float3
     *  }
     *
     *  location index means how many"th" uniform in shader's all uniforms this one is at
     * */
    struct UniformInfo
    {
        int locationIndex;
        ShaderDataType type;
        int arrayLen = 1;
    };

}
