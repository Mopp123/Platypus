#include "Font.h"
#include "platypus/core/Application.h"
#include "AssetManager.h"
#include "platypus/core/Debug.h"

#include <cmath>

#include <ft2build.h>
#include FT_FREETYPE_H


namespace platypus
{
    Font::Font() :
        Asset(AssetType::ASSET_TYPE_FONT)
    {
    }

    Font::~Font()
    {}

    bool Font::load(const std::string& filepath, unsigned int pixelSize)
    {
        _pixelSize = pixelSize;
        // NOTE: Iterating all available glyphs with FT_Get_First_Char and FT_Get_Next_Char
        // doesn't include all glyphs, like scands, so need to do it like this atm...
        // TODO: Figure out a better way!
        return createFont(
            filepath,
            L" qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890.,:;?!&_'+-*^/()[]{}<>|ÖöÄä"
        );
    }

    // TODO: Put into constructor -> doesnt need to be own func anymore?
    bool Font::createFont(const std::string& filepath, const std::wstring& charsToLoad)
    {
        FT_Library freetypeLib;
        if (FT_Init_FreeType(&freetypeLib))
        {
            Debug::log(
                "@Font::createFont "
                "Failed to init Freetype library",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        FT_Face fontFace;
        if (FT_New_Face(freetypeLib, filepath.c_str(), 0, &fontFace))
        {
            Debug::log(
                "@Font::createFont "
                "Failed create new font face using file: " + filepath,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        if (FT_Set_Pixel_Sizes(fontFace, 0, _pixelSize))
        {
            Debug::log(
                "@Font::createFont "
                "Failed to set pixel sizes to: " + std::to_string(_pixelSize),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }

        // Load data from each glyph
        float charArraysqrt = sqrt((float)charsToLoad.size());
        int textureAtlasRowCount = (int)std::ceil(charArraysqrt);
        _textureAtlasTileWidth = 1; // width in pixels of a single tile inside the font texture atlas

        int textureOffsetX = 0;
        int textureOffsetY = 0;

        int maxGlyphWidth = 0;
        int maxGlyphHeight = 0;


        FT_Select_Charmap(fontFace, FT_ENCODING_UNICODE);

        // NOTE: Originally the pair's first was ptr to heap allocated bitmap data
        // -> sanitizer complained about it so switched to vector for now...
        std::vector<std::pair<std::vector<unsigned char>, FontGlyphData>> loadedGlyphs;
        for (int i = 0; i < charsToLoad.size(); ++i)
        {
            wchar_t c = charsToLoad[i];

            if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
            {
                Debug::log(
                    "@Font::createFont "
                    "Failed to load character '" + std::to_string(c) + "' from file " + filepath,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return false;
            }

            FT_GlyphSlot ftGlyph = fontFace->glyph;
            int currentGlyphWidth = ftGlyph->bitmap.width;
            int currentGlyphHeight = ftGlyph->bitmap.rows;

            FontGlyphData gd =
            {
                textureOffsetX,
                textureOffsetY,

                currentGlyphWidth,
                currentGlyphHeight,

                ftGlyph->bitmap_left,
                ftGlyph->bitmap_top,

                (uint32_t)ftGlyph->advance.x
            };

            maxGlyphWidth = std::max(maxGlyphWidth, currentGlyphWidth);
            maxGlyphHeight = std::max(maxGlyphHeight, currentGlyphHeight);

            const int glyphBitmapSize = currentGlyphWidth * currentGlyphHeight;
            std::vector<unsigned char> bitmap(glyphBitmapSize);
            // If empty space -> make some empty tile, otherwise get the glyph bitmap
            if (c == ' ')
                memset(bitmap.data(), 0, glyphBitmapSize);
            else
                memcpy(bitmap.data(), ftGlyph->bitmap.buffer, sizeof(unsigned char) * glyphBitmapSize);

            loadedGlyphs.push_back(std::make_pair(bitmap, gd));

            textureOffsetX++;
            if (textureOffsetX >= textureAtlasRowCount)
            {
                textureOffsetX = 0;
                textureOffsetY++;
            }
            _glyphMapping.insert(std::make_pair(c, gd));
        }
        // We want each glyph in the texture atlas to have perfect square space, for simplicity's sake..
        // (This results in many unused pixels tho..)
        _textureAtlasTileWidth = std::max(maxGlyphWidth, maxGlyphHeight);
        // ...but we also need max glyph height to render properly...
        _charHeight = std::max(maxGlyphHeight, maxGlyphHeight);

        // Combine all the loaded glyphs bitmaps into a single large texture atlas
        const unsigned int combinedGlyphBitmapWidth = textureAtlasRowCount * _textureAtlasTileWidth;
        const unsigned int combinedGlyphBitmapSize = combinedGlyphBitmapWidth * combinedGlyphBitmapWidth; // font texture atlas size in bytes

        unsigned char* combinedGlyphBitmap = new unsigned char[combinedGlyphBitmapSize];
        memset(combinedGlyphBitmap, 0, combinedGlyphBitmapSize);

        for (unsigned int apy = 0; apy < combinedGlyphBitmapWidth; ++apy)
        {
            for (unsigned int apx = 0; apx < combinedGlyphBitmapWidth; ++apx)
            {
                unsigned int glyphIndexX = std::floor(apx / _textureAtlasTileWidth);
                unsigned int glyphIndexY = std::floor(apy / _textureAtlasTileWidth);

                if (glyphIndexX + glyphIndexY * textureAtlasRowCount < (unsigned int)loadedGlyphs.size())
                {
                    std::pair<std::vector<unsigned char>, FontGlyphData>& g = loadedGlyphs[glyphIndexX + glyphIndexY * textureAtlasRowCount];
                    if (g.first.empty())
                        continue;

                    unsigned int glyphPixelX = apx % _textureAtlasTileWidth;
                    unsigned int glyphPixelY = apy % _textureAtlasTileWidth;

                    if (glyphPixelX < g.second.width && glyphPixelY < g.second.height)
                        combinedGlyphBitmap[apx + apy * combinedGlyphBitmapWidth] = g.first[glyphPixelX + glyphPixelY * g.second.width];
                }
            }
        }
        // NOTE: loadedGlyphs bitmaps were allocated using new previously -> switched to std::vector since sanitizer complained..
        // delete all heap allocated glyph bitmaps
        /*
        for (std::pair<unsigned char*, FontGlyphData>& p : loadedGlyphs)
        {
            delete p.first;
            p.first = nullptr;
        }
        */

        // Create resources through AssetManager
        Application* pApp = Application::get_instance();
        if (!pApp)
        {
            Debug::log(
                "@Font::createFont Application was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            // TODO: Separate asserts for ones remaining in release and ones used only in debug?
            PLATYPUS_ASSERT(false);
            return false;
        }
        AssetManager* pAssetManager = pApp->getAssetManager();

        Image* pFontImgData = pAssetManager->createImage(
            combinedGlyphBitmap,
            combinedGlyphBitmapWidth,
            combinedGlyphBitmapWidth,
            1
        );
        _imageID = pFontImgData->getID();

        Texture* pTexture = pAssetManager->createTexture(
            _imageID,
            ImageFormat::R8_UNORM,
            TextureSampler(
                TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
                TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                1,
                0
            ),
            textureAtlasRowCount
        );
        _textureID = pTexture->getID();

        _textureAtlasRowCount = textureAtlasRowCount;

        FT_Done_Face(fontFace);
        FT_Done_FreeType(freetypeLib);

        return true;
    }

    const Texture* Font::getTexture() const
    {
        return (const Texture*)Application::get_instance()->getAssetManager()->getAsset(_textureID, AssetType::ASSET_TYPE_TEXTURE);
    }

    const FontGlyphData * const Font::getGlyph(wchar_t c) const
    {
        std::unordered_map<wchar_t, FontGlyphData>::const_iterator it = _glyphMapping.find(c);
        if (it != _glyphMapping.end())
            return &it->second;
        return nullptr;
    }
}
