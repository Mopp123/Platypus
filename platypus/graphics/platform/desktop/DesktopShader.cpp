#include "platypus/graphics/Shader.h"
#include "DesktopShader.h"
#include "DesktopContext.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/FileUtils.h"
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    VkPipelineShaderStageCreateInfo get_pipeline_shader_stage_create_info(
        const Shader& shader,
        const ShaderImpl * const pImpl
    )
    {
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.stage = shader.getStage() == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfo.module = pImpl->shaderModule;
        shaderStageCreateInfo.pName = "main";
        return shaderStageCreateInfo;
    }


    VkShaderStageFlags to_vk_shader_stage_flags(uint32_t shaderStageFlags)
    {
        switch (shaderStageFlags)
        {
            case ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
            default:
                Debug::log(
                    "@to_vk_shader_stage_flags "
                    "Invalid shaderStageFlags: " + std::to_string(shaderStageFlags) + " "
                    "Available flag bits are currently: "
                    "VK_SHADER_STAGE_VERTEX_BIT, "
                    "VK_SHADER_STAGE_FRAGMENT_BIT",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
        }
        return 0;
    }

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
            Context::get_impl()->device,
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
        vkDestroyShaderModule(Context::get_impl()->device, _pImpl->shaderModule, nullptr);
        if (_pImpl)
            delete _pImpl;
    }
}
