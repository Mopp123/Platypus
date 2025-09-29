#include "Material.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/PipelineFactory.hpp"
#include "AssetManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <vector>


namespace platypus
{
    Material::Material(
        MaterialType type,
        ID_t blendmapTextureID,
        ID_t* pDiffuseTextureIDs,
        ID_t* pSpecularTextureIDs,
        ID_t* pNormalTextureIDs,
        size_t diffuseTextureCount,
        size_t specularTextureCount,
        size_t normalTextureCount,
        float specularStrength,
        float shininess,
        bool shadeless
    ) :
        Asset(AssetType::ASSET_TYPE_MATERIAL),
        _type(type),
        _blendmapTextureID(blendmapTextureID),
        _diffuseTextureCount(diffuseTextureCount),
        _specularTextureCount(specularTextureCount),
        _normalTextureCount(normalTextureCount),
        _specularStrength(specularStrength),
        _shininess(shininess),
        _shadeless(shadeless)
    {
        validateTextureCounts();

        memset(_diffuseTextureIDs, 0, sizeof(ID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_specularTextureIDs, 0, sizeof(ID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);
        memset(_normalTextureIDs, 0, sizeof(ID_t) * PE_MAX_MATERIAL_TEX_CHANNELS);

        memcpy(_diffuseTextureIDs, pDiffuseTextureIDs, sizeof(ID_t) * _diffuseTextureCount);
        memcpy(_specularTextureIDs, pSpecularTextureIDs, sizeof(ID_t) * _specularTextureCount);
        memcpy(_normalTextureIDs, pNormalTextureIDs, sizeof(ID_t) * _normalTextureCount);

        createDescriptorSetLayout();
    }

    Material::~Material()
    {
        _descriptorSetLayout.destroy();
        // TODO: Unfuck below
        // NOTE: Important that the pipeline gets destroyed before shaders,
        // since the web implementation detaches the shaders from opengl
        // shader program on pipeline destruction.
        //  -> if shaders destroyed before pipeline, the detaching in
        //  OpenglShaderProgram breaks
        if (_pPipelineData)
        {
            delete _pPipelineData->pPipeline;
            delete _pPipelineData->pVertexShader;
            delete _pPipelineData->pFragmentShader;
            delete _pPipelineData;
        }
        if (_pSkinnedPipelineData)
        {
            delete _pSkinnedPipelineData->pPipeline;
            delete _pSkinnedPipelineData->pVertexShader;
            delete _pSkinnedPipelineData->pFragmentShader;
            delete _pSkinnedPipelineData;
        }
    }

    void Material::createPipeline(
        const Mesh* pMesh
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

        const std::string vertexShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            false
        );
        const std::string fragmentShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
            false
        );
        Debug::log(
            "@Material::createPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        // TODO: Unfuck below
        _pPipelineData = new MaterialPipelineData;
        Shader* pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        Shader* pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );
        _pPipelineData->pVertexShader = pVertexShader;
        _pPipelineData->pFragmentShader = pFragmentShader;
        Pipeline* pPipeline = create_material_pipeline(
            pMesh->getVertexBufferLayout(),
            true, // instanced
            false, // skinned
            this
        );
        _pPipelineData->pPipeline = pPipeline;
        _pPipelineData->pPipeline->create();
    }

    void Material::createSkinnedPipeline(
        const Mesh* pMesh
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

        const std::string vertexShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            true
        );
        const std::string fragmentShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
            true
        );
        Debug::log(
            "@Material::createSkinnedPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        // TODO: Unfuck below
        _pSkinnedPipelineData = new MaterialPipelineData;
        Shader* pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        Shader* pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );
        _pSkinnedPipelineData->pVertexShader = pVertexShader;
        _pSkinnedPipelineData->pFragmentShader = pFragmentShader;
        Pipeline* pPipeline = create_material_pipeline(
            pMesh->getVertexBufferLayout(),
            false, // instanced
            true, // skinned
            this
        );
        _pSkinnedPipelineData->pPipeline = pPipeline;
        _pSkinnedPipelineData->pPipeline->create();
    }

    void Material::createTerrainPipeline()
    {
        if (_pPipelineData != nullptr)
        {
            Debug::log(
                "@Material::createTerrainPipeline "
                "Pipeline data already exists for material with ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const std::string vertexShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            false
        );
        const std::string fragmentShaderFilename = getShaderFilename(
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
            false
        );
        Debug::log(
            "@Material::createTerrainPipeline Using shaders:\n    " + vertexShaderFilename + "\n    " + fragmentShaderFilename
        );

        // TODO: Unfuck below
        _pPipelineData = new MaterialPipelineData;
        Shader* pVertexShader = new Shader(
            vertexShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        Shader* pFragmentShader = new Shader(
            fragmentShaderFilename,
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );
        _pPipelineData->pVertexShader = pVertexShader;
        _pPipelineData->pFragmentShader = pFragmentShader;
        Pipeline* pPipeline = create_terrain_material_pipeline(this);
        _pPipelineData->pPipeline = pPipeline;
        _pPipelineData->pPipeline->create();
    }

    void Material::recreateExistingPipeline()
    {
        bool created = false;

        if (_pPipelineData)
        {
            _pPipelineData->pPipeline->create();
            created = true;
        }

        if (_pSkinnedPipelineData)
        {
            _pSkinnedPipelineData->pPipeline->create();
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
            _pPipelineData->pPipeline->destroy();
            destroyed = true;
            Debug::log("@Material::destroyPipeline Static pipeline destroyed");
        }

        if (_pSkinnedPipelineData)
        {
            _pSkinnedPipelineData->pPipeline->destroy();
            destroyed = true;
            Debug::log("@Material::destroyPipeline Skinned pipeline destroyed");
        }

        if (!destroyed)
            warnUnassigned("@Material::destroyPipeline");
    }

    Texture* Material::getBlendmapTexture() const
    {
        if (_blendmapTextureID == NULL_ID)
        {
            Debug::log(
                "@Material::getBlendmapTexture "
                "blendmap texture asset ID was NULL_ID.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _blendmapTextureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getDiffuseTexture(size_t channel) const
    {
        if (channel >= _diffuseTextureCount)
        {
            Debug::log(
                "@Material::getDiffuseTexture "
                "channel(" + std::to_string(channel) + ") out of bounds. "
                "Material has " + std::to_string(_diffuseTextureCount) + " diffuse textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _diffuseTextureIDs[channel],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getSpecularTexture(size_t channel) const
    {
        if (channel >= _specularTextureCount)
        {
            Debug::log(
                "@Material::getSpecularTexture "
                "channel(" + std::to_string(channel) + ") out of bounds. "
                "Material has " + std::to_string(_specularTextureCount) + " specular textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _specularTextureIDs[channel],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    Texture* Material::getNormalTexture(size_t channel) const
    {
        if (channel >= _normalTextureCount)
        {
            Debug::log(
                "@Material::getNormalTexture "
                "channel(" + std::to_string(channel) + ") out of bounds. "
                "Material has " + std::to_string(_normalTextureCount) + " normal textures.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        return (Texture*)pAssetManager->getAsset(
            _normalTextureIDs[channel],
            AssetType::ASSET_TYPE_TEXTURE
        );
    }

    std::vector<Texture*> Material::getTextures() const
    {
        std::vector<Texture*> textures;
        if (_blendmapTextureID != NULL_ID)
            textures.push_back(getBlendmapTexture());

        for (size_t i = 0; i < _diffuseTextureCount; ++i)
            textures.push_back(getDiffuseTexture(i));

        for (size_t i = 0; i < _specularTextureCount; ++i)
            textures.push_back(getSpecularTexture(i));

        for (size_t i = 0; i < _normalTextureCount; ++i)
            textures.push_back(getNormalTexture(i));

        return textures;
    }

    void Material::warnUnassigned(const std::string& beginStr)
    {
        Debug::log(
            beginStr +
            " Pipeline data was nullptr. "
            "This Material may have been created but not used by any renderable component.",
            Debug::MessageType::PLATYPUS_WARNING
        );
    }

    void Material::validateTextureCounts()
    {
        if (_diffuseTextureCount == 0)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "At least one diffuse texture required!"
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_diffuseTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many diffuse textures provided(" + std::to_string(_diffuseTextureCount) + ")! "
                "Max diffuse texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_specularTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many specular textures provided(" + std::to_string(_specularTextureCount) + ")! "
                "Max specular texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_normalTextureCount > PE_MAX_MATERIAL_TEX_CHANNELS)
        {
            Debug::log(
                "@Material::validateTextureCounts "
                "Too many normal textures provided(" + std::to_string(_normalTextureCount) + ")! "
                "Max normal texture count is " + std::to_string(PE_MAX_MATERIAL_TEX_CHANNELS) + ". "
                "Material asset ID: " + std::to_string(getID()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void Material::createDescriptorSetLayout()
    {
        std::vector<DescriptorSetLayoutBinding> layoutBindings;
        uint32_t textureBindingCount = getTotalTextureCount();
        for (uint32_t textureBinding = 0; textureBinding < textureBindingCount; ++textureBinding)
        {
            layoutBindings.push_back(
                {
                    textureBinding,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { } }
                }
            );
        }

        layoutBindings.push_back(
            {
                textureBindingCount,
                1,
                DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                { { ShaderDataType::Float4 } }
            }
        );
        _descriptorSetLayout = {
            layoutBindings
        };
    }

    // TODO: Make it impossible to attempt getting skinned shader name for terrain Material
    std::string Material::getShaderFilename(uint32_t shaderStage, bool skinned)
    {
        // Example shader names:
        // vertex shader: "StaticVertexShader", "StaticVertexShader_tangent", "SkinnedVertexShader"
        // fragment shader: "StaticFragmentShader_d", "StaticFragmentShader_ds", "SkinnedFragmentShader_dsn"
        std::string shaderName = "";
        if (_type == MaterialType::TERRAIN)
        {
            shaderName += "Terrain";
        }
        else
        {
            shaderName += skinned ? "Skinned" : "Static";
        }

        // Using same vertex shader for diffuse and diffuse+specular
        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
        {
            shaderName += _normalTextureCount == 0 ? "VertexShader" : "VertexShader_tangent";
        }
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        {
            // adding d here since all materials needs to have at least one diffuse texture
            shaderName += "FragmentShader_d";
            if (_specularTextureCount > 0)
                shaderName += "s";
            if (_normalTextureCount > 0)
                shaderName += "n";
        }

        return shaderName;
    }

}
