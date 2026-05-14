#include "Asset.hpp"
#include "platypus/core/Application.hpp"
#include "AssetManager.hpp"


namespace platypus
{
    std::string asset_type_to_string(AssetType type)
    {
        switch (type)
        {
            case AssetType::ASSET_TYPE_MESH: return "ASSET_TYPE_MESH";
            case AssetType::ASSET_TYPE_MODEL: return "ASSET_TYPE_MODEL";
            case AssetType::ASSET_TYPE_IMAGE: return "ASSET_TYPE_IMAGE";
            case AssetType::ASSET_TYPE_TEXTURE: return "ASSET_TYPE_TEXTURE";
            case AssetType::ASSET_TYPE_MATERIAL: return "ASSET_TYPE_MATERIAL";
            case AssetType::ASSET_TYPE_FONT: return "ASSET_TYPE_FONT";
            case AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA: return "ASSET_TYPE_SKELETAL_ANIMATION_DATA";
            default: return "ASSET_TYPE_NONE";
        }
    }

    Asset::Asset(
        AssetType type,
        const std::string& name,
        ID_t id,
        bool persistent
    ) :
        _type(type),
        _name(name),
        _persistent(persistent)
    {
        if (id != NULL_ID)
        {
            _id = id;
            ID::occupy(id);
        }
        else
        {
            _id = ID::generate();
        }
    }

    Asset::~Asset()
    {
        ID::erase(_id);
    }
}
