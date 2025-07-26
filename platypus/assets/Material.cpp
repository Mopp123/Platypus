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

        _descriptorSetLayout = { create_descriptor_set_layout_bindings(normalTextureID != NULL_ID) };
    }

    Material::~Material()
    {
        freeDescriptorSets();

        _descriptorSetLayout.destroy();
        if (_pPipelineData)
            delete _pPipelineData;
        if (_pSkinnedPipelineData)
            delete _pSkinnedPipelineData;
    }

    void Material::createPipeline(
        const VertexBufferLayout& meshVertexBufferLayout
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

        bool normalMapping = _normalTextureID != NULL_ID;
        const std::string vertexShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            normalMapping,
            false
        );
        const std::string fragmentShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
            normalMapping,
            false
        );
        Debug::log(
            "@Material::createPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        _pPipelineData = new MaterialPipelineData{
            { meshVertexBufferLayout, instancedVertexBufferLayout },
            {
                vertexShaderFilename,
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
            },
            {
                fragmentShaderFilename,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
            }
        };

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        std::vector<DescriptorSetLayout> descriptorSetLayouts = {
            pMasterRenderer->getCameraDescriptorSetLayout(),
            pMasterRenderer->getDirectionalLightDescriptorSetLayout(),
            _descriptorSetLayout
        };

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        const Extent2D swapchainExtent = swapchain.getExtent();
        const uint32_t viewportWidth = (uint32_t)swapchainExtent.width;
        const uint32_t viewportHeight = (uint32_t)swapchainExtent.height;
        Rect2D viewportScissor = { 0, 0, viewportWidth, viewportHeight };
        _pPipelineData->pipeline.create(
            swapchain.getRenderPass(),
            _pPipelineData->vertexBufferLayouts,
            descriptorSetLayouts,
            _pPipelineData->vertexShader,
            _pPipelineData->fragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            false, // enable color blending
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void Material::createSkinnedPipeline(
        const VertexBufferLayout& meshVertexBufferLayout
    )
    {
        if (_pSkinnedPipelineData != nullptr)
        {
            Debug::log(
                "@Material::createSkinnedPipeline "
                "Skinned pipeline data already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        // NOTE: Currently not supporting normal mapping for skinned meshes
        //bool normalMapping = _normalTextureID != NULL_ID;
        const std::string vertexShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            false,
            true
        );
        const std::string fragmentShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
            false,
            true
        );
        Debug::log(
            "@Material::createSkinnedPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );
        _pSkinnedPipelineData = new MaterialPipelineData{
            { meshVertexBufferLayout },
            {
                vertexShaderFilename,
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
            },
            {
                fragmentShaderFilename,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
            }
        };

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        std::vector<DescriptorSetLayout> descriptorSetLayouts = {
            pMasterRenderer->getCameraDescriptorSetLayout(),
            pMasterRenderer->getDirectionalLightDescriptorSetLayout(),
            pMasterRenderer->getSkinnedMeshRenderer()->getDescriptorSetLayout(),
            _descriptorSetLayout
        };

        const Swapchain& swapchain = pMasterRenderer->getSwapchain();
        const Extent2D swapchainExtent = swapchain.getExtent();
        const uint32_t viewportWidth = (uint32_t)swapchainExtent.width;
        const uint32_t viewportHeight = (uint32_t)swapchainExtent.height;
        Rect2D viewportScissor = { 0, 0, viewportWidth, viewportHeight };
        _pSkinnedPipelineData->pipeline.create(
            swapchain.getRenderPass(),
            _pSkinnedPipelineData->vertexBufferLayouts,
            descriptorSetLayouts,
            _pSkinnedPipelineData->vertexShader,
            _pSkinnedPipelineData->fragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_BACK,
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
        bool created = false;

        const Swapchain& swapchain = Application::get_instance()->getMasterRenderer()->getSwapchain();
        const Extent2D swapchainExtent = swapchain.getExtent();
        const uint32_t viewportWidth = (uint32_t)swapchainExtent.width;
        const uint32_t viewportHeight = (uint32_t)swapchainExtent.height;
        Rect2D viewportScissor = { 0, 0, viewportWidth, viewportHeight };

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        if (_pPipelineData)
        {
            std::vector<DescriptorSetLayout> descriptorSetLayouts = {
                pMasterRenderer->getCameraDescriptorSetLayout(),
                pMasterRenderer->getDirectionalLightDescriptorSetLayout(),
                _descriptorSetLayout
            };

            _pPipelineData->pipeline.create(
                swapchain.getRenderPass(),
                _pPipelineData->vertexBufferLayouts,
                descriptorSetLayouts,
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
            created = true;
        }

        if (_pSkinnedPipelineData)
        {
            std::vector<DescriptorSetLayout> descriptorSetLayouts = {
                pMasterRenderer->getCameraDescriptorSetLayout(),
                pMasterRenderer->getDirectionalLightDescriptorSetLayout(),
                pMasterRenderer->getSkinnedMeshRenderer()->getDescriptorSetLayout(),
                _descriptorSetLayout
            };
            _pSkinnedPipelineData->pipeline.create(
                swapchain.getRenderPass(),
                _pSkinnedPipelineData->vertexBufferLayouts,
                descriptorSetLayouts,
                _pSkinnedPipelineData->vertexShader,
                _pSkinnedPipelineData->fragmentShader,
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
            created = true;
        }

        if (!created)
            warnUnassigned("@Material::recreateExistingPipeline");
    }

    void Material::destroyPipeline()
    {
        bool destroyed = false;
        if (_pPipelineData)
        {
            _pPipelineData->pipeline.destroy();
            destroyed = true;
            Debug::log("@Material::destroyPipeline Static pipeline destroyed");
        }

        if (_pSkinnedPipelineData)
        {
            _pSkinnedPipelineData->pipeline.destroy();
            destroyed = true;
            Debug::log("@Material::destroyPipeline Skinned pipeline destroyed");
        }

        if (!destroyed)
            warnUnassigned("@destroyPipeline");
    }

    // NOTE: This also creates the uniform buffer
    //  -> should that be a separate func or name this more clearly?
    void Material::createDescriptorSets()
    {
        if (!(_pPipelineData || _pSkinnedPipelineData))
        {
            Debug::log(
                "@Material::createDescriptorSets "
                "Pipeline data was nullptr for material with ID: " + std::to_string(getID()) + " "
                "This material may have been created but never used by any renderable component!",
                Debug::MessageType::PLATYPUS_WARNING
            );
            return;
        }

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
        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        CommandPool& commandPool = pMasterRenderer->getCommandPool();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();

        const Texture* pDiffuseTexture = getDiffuseTexture();
        const Texture* pSpecularTexture = getSpecularTexture();
        const Texture* pNormalTexture = hasNormalMap() ? getNormalTexture() : nullptr;
        Vector4f materialProperties(
            _specularStrength,
            _shininess,
            _shadeless,
            0
        );

        size_t maxFramesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
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

            if (hasNormalMap())
            {
                _descriptorSets.push_back(
                    descriptorPool.createDescriptorSet(
                        &_descriptorSetLayout,
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
                        &_descriptorSetLayout,
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
        if (!(_pPipelineData || _pSkinnedPipelineData))
        {
            Debug::log(
                "@Material::freeDescriptorSets "
                "Pipeline data was nullptr for material with ID: " + std::to_string(getID()) + " "
                "This material may have been created but never used by any renderable component!",
                Debug::MessageType::PLATYPUS_WARNING
            );
            return;
        }

        DescriptorPool& descriptorPool = Application::get_instance()->getMasterRenderer()->getDescriptorPool();
        for (Buffer* pBuffer : _uniformBuffers)
            delete pBuffer;

        _uniformBuffers.clear();

        descriptorPool.freeDescriptorSets(_descriptorSets);
        _descriptorSets.clear();
    }

    Texture* Material::getDiffuseTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _diffuseTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getSpecularTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _specularTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getNormalTexture() const
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _normalTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    void Material::warnUnassigned(const std::string& beginStr)
    {
        Debug::log(
            beginStr +
            " Pipeline data was nullptr. "
            "This Material may have been created but used by any renderable component.",
            Debug::MessageType::PLATYPUS_WARNING
        );
    }

    std::string Material::getShaderFilename(uint32_t shaderStage, bool normalMapping, bool skinned)
    {
        // Example shader names:
        // vertex shader: "StaticVertexShader", "StaticHDVertexShader", "SkinnedHDVertexShader"
        // fragment shader: "StaticFragmentShader", "StaticHDFragmentShader", "SkinnedHDFragmentShader"
        std::string shaderName = "";
        if (!skinned)
            shaderName += "Static";
        else
            shaderName += "Skinned";

        if (normalMapping)
            shaderName += "HD";

        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
            shaderName += "Vertex";
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
            shaderName += "Fragment";

        shaderName += "Shader";

        return shaderName;
    }
}
