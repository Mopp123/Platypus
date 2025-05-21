#include "Material.h"
#include "AssetManager.h"
#include "platypus/core/Application.h"


namespace platypus
{
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
        _shadeless(shadeless),
    {
        _pipelineData.descriptorSetLayout = DescriptorSetLayout(
            {
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
            }
        )
    }

    Material::~Material()
    {
    }

    void Material::createPipeline()
    {
        VertexBufferLayout vertexBufferLayout = {
            {
                { 0, ShaderDataType::Float3 },
                { 1, ShaderDataType::Float3 },
                { 2, ShaderDataType::Float2 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
        VertexBufferLayout instancedVertexBufferLayout = {
            {
                { 3, ShaderDataType::Float4 },
                { 4, ShaderDataType::Float4 },
                { 5, ShaderDataType::Float4 },
                { 6, ShaderDataType::Float4 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
            1
        };
        MasterRenderer& masterRenderer = Application::get_instance()->getMasterRenderer();
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = {
            &masterRenderer.getCameraDescriptorSetLayout(),
            &masterRenderer.getDirectionalLightDescriptorSetLayout(),
            &_materialDescriptorSetLayout
        };

        Rect2D viewportScissor = { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight };
        _pipeline.create(
            renderPass,
            { vbLayout, vbLayoutInstanced },
            descriptorSetLayouts,
            _vertexShader,
            _fragmentShader,
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

    void Material::createNormalMappedPipeline()
    {
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
}
