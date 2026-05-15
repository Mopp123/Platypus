#pragma once

#include "Asset.hpp"
#include "Texture.hpp"
#include <string>
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
        UUID_t _imageID = NULL_UUID;
        UUID_t _textureID = NULL_UUID;

        unsigned int _pixelSize = 1;
        int _textureAtlasRowCount = 1;
        int _textureAtlasTileWidth = 1; // Width in pixels of a single tile inside the font's texture atlas.
        // Max height of a character
        int _maxCharHeight = 1;
        // If part of a character goes below baseline,
        // this value can be used to determine for example
        // how much a button needs to stretch vertically in
        // order for the text to be completely inside it.
        // NOTE: This shouldn't be used in rendering tho!
        int _maxBaselineDrop = 0;

        std::unordered_map<uint32_t, FontGlyphData> _glyphMapping;

    public:
        Font();
        ~Font();

        // NOTE: Some "pixelSize" values doesn't work on some fonts!
        //  -> If text looks funky try some other pixelSize values.
        // TODO: Figure out way of knowing available sizes for fonts..
        bool load(const std::string& filepath, unsigned int pixelSize);

        const Texture* getTexture() const;

        const FontGlyphData * const getGlyph(uint32_t codepoint) const;
        inline const unsigned int getPixelSize() const { return _pixelSize; }
        inline std::unordered_map<uint32_t, FontGlyphData>& getGlyphMapping() { return _glyphMapping; }
        inline const std::unordered_map<uint32_t, FontGlyphData>& getGlyphMapping() const { return _glyphMapping; }
        inline int getTextureAtlasRowCount() const { return _textureAtlasRowCount; }
        inline int getTilePixelWidth() const { return _textureAtlasTileWidth; }
        inline int getMaxCharHeight() const { return _maxCharHeight; }
        inline int getMaxBaselineDrop() const { return _maxBaselineDrop; }

        // Returns vertical boundary in which each glyph fits in.
        // -> _maxCharHeight doesn't take into account that some glyphs go below the baseline!
        inline int getFittingHeight() const { return _maxCharHeight + _maxBaselineDrop; }
        inline UUID_t getTextureID() const { return _textureID; }

    private:
        bool createFont(const std::string& filepath, std::string charsToLoad);
    };
}
