#include "Material.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/PipelineFactory.hpp"
#include "AssetManager.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <vector>


namespace platypus
{
    static std::vector<DescriptorSetLayoutBinding> create_descriptor_set_layout_bindings(
        bool normalMapping
    )
    {
        std::vector<DescriptorSetLayoutBinding> layoutBindings;
        uint32_t textureBindingCount = normalMapping ? 3 : 2;
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
        return layoutBindings;
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
            pMesh,
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
            pMesh,
            false, // instanced
            true, // skinned
            this
        );
        _pSkinnedPipelineData->pPipeline = pPipeline;
        _pSkinnedPipelineData->pPipeline->create();
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

    std::vector<Texture*> Material::getTextures() const
    {
        std::vector<Texture*> textures;
        textures.push_back(getDiffuseTexture());
        textures.push_back(getSpecularTexture());
        if (hasNormalMap())
            textures.push_back(getNormalTexture());

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
