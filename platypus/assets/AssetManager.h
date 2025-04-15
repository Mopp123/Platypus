#pragma once

#include "Asset.h"
#include "Image.h"
#include "Mesh.h"
#include "Model.h"
#include "Texture.h"
#include "Font.h"
#include "platypus/graphics/CommandBuffer.h"
#include <unordered_map>
#include <vector>


namespace platypus
{
    class AssetManager
    {
    private:
        CommandPool& _commandPoolRef;
        std::unordered_map<ID_t, Asset*> _assets;

    public:
        AssetManager(CommandPool& commandPool);
        ~AssetManager();
        void destroyAssets();

        Image*  createImage(PE_ubyte* pData, int width, int height, int channels);
        Image* loadImage(const std::string& filepath);
        Texture* createTexture(ID_t imageID, const TextureSampler& sampler);
        Mesh* createMesh(
            const std::vector<float>& vertexData,
            const std::vector<uint32_t>& indexData
        );
        Model* loadModel(const std::string& filepath);
        Font* loadFont(const std::string& filepath, unsigned int pixelSize);

        Image* getImage(ID_t assetID) const;
        Texture* getTexture(ID_t assetID) const;
        Mesh* getMesh(ID_t assetID) const;
        Font* getFont(ID_t fontID) const;

    private:
        Asset* getAsset(ID_t assetID) const;
    };
}
