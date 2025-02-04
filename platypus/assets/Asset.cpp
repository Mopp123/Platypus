#include "Asset.h"

namespace platypus
{
    std::string asset_type_to_string(AssetType type)
    {
        switch (type)
        {
            case AssetType::ASSET_TYPE_MESH: return "ASSET_TYPE_MESH";
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
