#include "Asset.h"

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
            case AssetType::ASSET_TYPE_FONT: return "ASSET_TYPE_FONT";
            default: return "ASSET_TYPE_NONE";
        }
    }

    Asset::Asset(AssetType type) :
        _type(type)
    {
        _id = ID::generate();
    }

    Asset::~Asset()
    {
        ID::erase(_id);
    }
}
