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
        _shadeless(shadeless)
    {
    }

    Material::~Material()
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
