#pragma once

#include "Asset.h"
#include "Texture.h"
#include <unordered_map>


namespace platypus
{
    struct FontGlyphData
    {
        // Offset in font texture atlas, where we get this glyph's visual
        int32_t textureOffsetX = 0;
        int32_t textureOffsetY = 0;

        // Scale of this glyph
        int32_t width = 0;
        int32_t height = 0;

        // Offset from baseline to left/top of glyph
        int32_t bearingX = 0;
        int32_t bearingY = 0;

        // Offset to advance to next glyph
        uint32_t advance = 0;
    };


    class Font : public Asset
    {
    private:
        ID_t _imageID = NULL_ID;
        ID_t _textureID = NULL_ID;

        unsigned int _pixelSize = 1;
        int _textureAtlasRowCount = 1;
        int _textureAtlasTileWidth = 1; // Width in pixels of a single tile inside the font's texture atlas.
        // Max height of a character
        int _charHeight = 1;

        std::unordered_map<char, FontGlyphData> _glyphMapping;

    public:
        Font();
        ~Font();

        // NOTE: Some "pixelSize" values doesn't work on some fonts!
        //  -> If text looks funky try some other pixelSize values.
        // TODO: Figure out way of knowing available sizes for fonts..
        bool load(const std::string& filepath, unsigned int pixelSize);

        const FontGlyphData * const getGlyph(char c) const;
        inline const unsigned int getPixelSize() const { return _pixelSize; }
        inline std::unordered_map<char, FontGlyphData>& getGlyphMapping() { return _glyphMapping; }
        inline const std::unordered_map<char, FontGlyphData>& getGlyphMapping() const { return _glyphMapping; }
        inline int getTextureAtlasRowCount() const { return _textureAtlasRowCount; }
        inline int getTilePixelWidth() const { return _textureAtlasTileWidth; }
        inline int getMaxCharHeight() const { return _charHeight; }

    private:
        bool createFont(const std::string& filepath, const std::string& charsToLoad);
    };
}
