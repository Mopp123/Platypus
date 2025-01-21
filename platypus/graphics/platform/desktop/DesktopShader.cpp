#include "platypus/graphics/Shader.h"
#include "DesktopShader.h"
#include "DesktopContext.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/FileUtils.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    Shader::Shader(const std::string& filepath, ShaderStageFlagBits stage) :
        _stage(stage)
    {
        std::vector<char> source = load_file(filepath);
        if (source.empty())
        {
            Debug::log(
                "@Shader::Shader "
                "Shader source was empty!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = source.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(source.data());

        VkShaderModule shaderModule;
        VkResult createResult = vkCreateShaderModule(
            Context::get_pimpl()->device,
            &createInfo,
            nullptr,
            &shaderModule
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Shader::Shader "
                "Failed to create shader module! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _pImpl = new ShaderImpl{
            shaderModule
        };
    }

    Shader::~Shader()
    {
        vkDestroyShaderModule(Context::get_pimpl()->device, _pImpl->shaderModule, nullptr);
        if (_pImpl)
            delete _pImpl;
    }
}
