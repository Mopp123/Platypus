#pragma once

#include "Asset.h"
#include "Texture.h"


namespace platypus
{
    class Material : public Asset
    {
    private:
        ID_t _diffuseTextureID = NULL_ID;
        ID_t _specularTextureID = NULL_ID;
        ID_t _normalTextureID = NULL_ID;

        float _specularStrength = 1.0f;
        float _shininess = 1.0f;

        bool _shadeless = false;

    public:
        Material(
            ID_t diffuseTextureID,
            ID_t specularTextureID,
            ID_t normalTextureID,
            float specularStrength = 1.0f,
            float shininess = 1.0f,
            bool shadeless = false
        );
        ~Material();

        Texture* getDiffuseTexture() const;
        Texture* getSpecularTexture() const;
        Texture* getNormalTexture() const;

        inline float getSpecularStrength() const { return _specularStrength; }
        inline float getShininess() const { return _shininess; }
        inline bool isShadeless() const { return _shadeless; }
        inline bool hasNormalMap() const { return _normalTextureID != NULL_ID;  }
    };
}
