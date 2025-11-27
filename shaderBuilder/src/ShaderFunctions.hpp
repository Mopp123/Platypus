#pragma once

#include "platypus/graphics/Shader.h"
#include <vector>


namespace platypus
{
    namespace shaderBuilder
    {
        std::vector<std::string> get_func_def(ShaderVersion shaderVersion, const std::string& funcName);
    }
}
