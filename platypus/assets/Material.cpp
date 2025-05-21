#include "Material.h"
#include "AssetManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static std::vector<DescriptorSetLayoutBinding> create_descriptor_set_layout_bindings(
        bool normalMapping
    )
    {
        if (!normalMapping)
        {
            return {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 5 } }
                },
                {
                    1,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 6 } }
                },
                {
                    2,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 7, ShaderDataType::Float4 } }
                }
            };
        }
        else
        {
            return {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 5 } }
                },
                {
                    1,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 6 } }
                },
                {
                    2,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 7 } }
                },
                {
                    3,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 8, ShaderDataType::Float4 } }
                }
            };
        }
    }


    Material::Material(
        ID_t diffuseTextureID,
        ID_t specularTextureID,
        ID_t normalTextureID,
        float specularStrength,
        float shininess,
        bool shadeless
    ) :
        Asset(AssetType::ASSET_TYPE_MATERIAL),
        _diffuseTextureID(diffuseTextureID),
        _specularTextureID(specularTextureID),
        _normalTextureID(normalTextureID),
        _specularStrength(specularStrength),
        _shininess(shininess),
        _shadeless(shadeless)
    {
    }

    Material::~Material()
    {
        freeDescriptorSets();
        size_t materialDescriptorSetLayoutIndex = _pPipelineData->descriptorSetLayouts.size() - 1;
        _pPipelineData->descriptorSetLayouts[materialDescriptorSetLayoutIndex].destroy();
        delete _pPipelineData;
    }

    void Material::createPipeline(
        const VertexBufferLayout& meshVertexBufferLayout,
        bool skinned
    )
    {
        if (_pPipelineData != nullptr)
        {
            Debug::log(
                "@Material::createPipeline "
                "Pipeline data already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // Add instanced vertex buffer layout (mat4)
        uint32_t meshVbLayoutElements = (uint32_t)meshVertexBufferLayout.getElements().size();
        VertexBufferLayout instancedVertexBufferLayout = {
            {
                { meshVbLayoutElements, ShaderDataType::Float4 },
                { meshVbLayoutElements + 1, ShaderDataType::Float4 },
                { meshVbLayoutElements + 2, ShaderDataType::Float4 },
                { meshVbLayoutElements + 3, ShaderDataType::Float4 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
            1
        };

        MasterRenderer& masterRenderer = Application::get_instance()->getMasterRenderer();
        bool normalMapping = _normalTextureID != NULL_ID;
        _pPipelineData = new MaterialPipelineData{
            { meshVertexBufferLayout, instancedVertexBufferLayout },
            {
                masterRenderer.getCameraDescriptorSetLayout(),
                masterRenderer.getDirectionalLightDescriptorSetLayout(),
                { create_descriptor_set_layout_bindings(normalMapping) },
            },
            {
                getVertexShaderFilename(ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, normalMapping),
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
            },
            {
                getVertexShaderFilename(ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, normalMapping),
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
            }
        };

        const Swapchain& swapchain = masterRenderer.getSwapchain();
        const Extent2D swapchainExtent = swapchain.getExtent();
        const uint32_t viewportWidth = (uint32_t)swapchainExtent.width;
        const uint32_t viewportHeight = (uint32_t)swapchainExtent.height;
        Rect2D viewportScissor = { 0, 0, viewportWidth, viewportHeight };
        _pPipelineData->pipeline.create(
            swapchain.getRenderPass(),
            _pPipelineData->vertexBufferLayouts,
            _pPipelineData->descriptorSetLayouts,
            _pPipelineData->vertexShader,
            _pPipelineData->fragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_NONE,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            false, // enable color blending
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void Material::recreateExistingPipeline()
    {
        if (_pPipelineData == nullptr)
        {
            Debug::log(
                "@Material::recreateExistingPipeline "
                "Pipeline data was nullptr for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer().getSwapchain();
        const Extent2D swapchainExtent = swapchain.getExtent();
        const uint32_t viewportWidth = (uint32_t)swapchainExtent.width;
        const uint32_t viewportHeight = (uint32_t)swapchainExtent.height;
        Rect2D viewportScissor = { 0, 0, viewportWidth, viewportHeight };
        _pPipelineData->pipeline.create(
            swapchain.getRenderPass(),
            _pPipelineData->vertexBufferLayouts,
            _pPipelineData->descriptorSetLayouts,
            _pPipelineData->vertexShader,
            _pPipelineData->fragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_NONE,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            false, // enable color blending
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void Material::destroyPipeline()
    {
        if (!_pPipelineData)
        {
            Debug::log(
                "@Material::destroyPipeline "
                "Pipeline data was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _pPipelineData->pipeline.destroy();
    }

    void Material::createDescriptorSets()
    {
        if (!_descriptorSets.empty())
        {
            Debug::log(
                "@Material::createDescriptorSets "
                "Descriptor sets already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (!_uniformBuffers.empty())
        {
            Debug::log(
                "@Material::createDescriptorSets "
                "Descriptor set uniform buffers already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        MasterRenderer& masterRenderer = Application::get_instance()->getMasterRenderer();
        CommandPool& commandPool = masterRenderer.getCommandPool();
        DescriptorPool& descriptorPool = masterRenderer.getDescriptorPool();

        const Texture* pDiffuseTexture = getDiffuseTexture();
        const Texture* pSpecularTexture = getSpecularTexture();
        const Texture* pNormalTexture = hasNormalMap() ? getNormalTexture() : nullptr;
        Vector4f materialProperties(
            _specularStrength,
            _shininess,
            _shadeless,
            0
        );

        size_t maxFramesInFlight = Application::get_instance()->getMasterRenderer().getSwapchain().getMaxFramesInFlight();
        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                commandPool,
                &materialProperties,
                sizeof(Vector4f),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _uniformBuffers.push_back(pUniformBuffer);

            size_t materialDescriptorSetLayoutIndex = _pPipelineData->descriptorSetLayouts.size() - 1;
            std::vector<DescriptorSetComponent> descriptorSetComponents;
            if (hasNormalMap())
            {
                _descriptorSets.push_back(
                    descriptorPool.createDescriptorSet(
                        &_pPipelineData->descriptorSetLayouts[materialDescriptorSetLayoutIndex],
                        {
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pDiffuseTexture },
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pSpecularTexture },
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pNormalTexture },
                            { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pUniformBuffer }
                        }
                    )
                );
            }
            else
            {
                _descriptorSets.push_back(
                    descriptorPool.createDescriptorSet(
                        &_pPipelineData->descriptorSetLayouts[materialDescriptorSetLayoutIndex],
                        {
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pDiffuseTexture },
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pSpecularTexture },
                            { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pUniformBuffer }
                        }
                    )
                );
            }
        }

        Debug::log(
            "@Material::createDescriptorSets "
            "New descriptor sets created for material: " + std::to_string(getID())
        );
    }

    void Material::freeDescriptorSets()
    {
        DescriptorPool& descriptorPool = Application::get_instance()->getMasterRenderer().getDescriptorPool();
        for (Buffer* pBuffer : _uniformBuffers)
            delete pBuffer;

        _uniformBuffers.clear();

        descriptorPool.freeDescriptorSets(_descriptorSets);
        _descriptorSets.clear();
    }

    Texture* Material::getDiffuseTexture() const
    {
        AssetManager& assetManager = Application::get_instance()->getAssetManager();
        return (Texture*)assetManager.getAsset(
            _diffuseTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getSpecularTexture() const
    {
        AssetManager& assetManager = Application::get_instance()->getAssetManager();
        return (Texture*)assetManager.getAsset(
            _specularTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getNormalTexture() const
    {
        AssetManager& assetManager = Application::get_instance()->getAssetManager();
        return (Texture*)assetManager.getAsset(
            _normalTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    std::string Material::getVertexShaderFilename(uint32_t shaderStage, bool normalMapping)
    {
        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
        {
            if (!normalMapping)
                return "StaticVertexShader";
            else
                return "StaticHDVertexShader";
        }
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        {
            if (!normalMapping)
                return "StaticFragmentShader";
            else
                return "StaticHDFragmentShader";
        }
        else
        {
            Debug::log(
                "@Material::getVertexShaderFilename "
                "Invalid shader stage: " + shader_stage_to_string(shaderStage),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }
}
