#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/utils/StringUtils.hpp"
#include <sstream>
#include <utf8.h>


namespace platypus
{
    namespace ui
    {
        UIElement* add_text_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::string& text,
            const Vector4f& color,
            const Font* pFont
        )
        {
            Layout parentLayout = pParent->getLayout();

            float charHeight = static_cast<float>(pFont->getMaxCharHeight());
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (parentLayout.wordWrap == WordWrap::NONE)
            {
                maxLineWidth = get_text_scale(text, pFont).x;
                finalText = text;
            }
            else if (parentLayout.wordWrap == WordWrap::NORMAL)
            {
                finalText = wrap_text(
                    text,
                    pFont,
                    pParent,
                    maxLineWidth,
                    lineCount
                );
            }

            Layout layout;
            // NOTE: If word wrapping, this scale isn't really usable for anything, since
            // it's just w*h rect and doesn't hold info about specific line sizes...
            //  -> if want to have some text mouse over, this can't be used for anything
            //  but single line text elements
            layout.scale = { maxLineWidth, charHeight * lineCount };

            UIElement* pElement = add_container(ui, pParent, layout, false, NULL_ID, pFont);
            GUIRenderable* pTextRenderable = create_gui_renderable(
                pElement->getEntityID(),
                pFont->getTextureID(),
                pFont->getID(),
                color,
                { 0, 0, 0, 0 }, // border color
                0.0f, // border thickness
                { 0, 0 }, // texture offset
                0, // layer
                true, // isText?
                finalText
            );

            return pElement;
        }

        // NOTE: VERY inefficient!
        // DO NOT USE if editing some text, like input field
        //  -> those should modify just the parts that are
        //  actually modified!
        std::string wrap_text(
            const std::string& text,
            const Font* pFont,
            const UIElement* pParentElement,
            float& outMaxLineWidth,
            size_t& outLineCount
        )
        {
            const Layout& parentLayout = pParentElement->getLayout();
            #ifdef PLATYPUS_DEBUG
            if (parentLayout.wordWrap != WordWrap::NORMAL)
            {
                Debug::log(
                    "@ui::wrap_text "
                    "Only WordWrap::NORMAL currently supported!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
            #endif

            outLineCount = 1;
            std::string wrappedText;

            float parentLayoutWidth = parentLayout.scale.x - parentLayout.padding.x;

            std::istringstream stream(text);
            std::string word;
            std::vector<std::string> words;
            // NOTE: Not sure if splitting into words safe enough if varying unicode char
            // sizes? -> Should be tho?
            while (getline(stream, word, ' '))
                words.push_back(word);

            float lineWidth = parentLayout.padding.x;
            float spaceWidth = get_char_scale(0x20, pFont).x;
            for (size_t i = 0; i < words.size(); ++i)
            {
                std::string str = words[i];
                Vector2f wordScale = get_text_scale(str, pFont);
                float wordWidth = wordScale.x;
                bool lastWord = i == words.size() - 1;

                // If single word goes out of bounds split into 2 words
                if (parentLayout.padding.x + wordWidth >= parentLayoutWidth)
                {
                    const size_t wordSize = str.size();
                    const char* pWordData = reinterpret_cast<const char*>(str.data());
                    utf8::iterator charIt(pWordData, pWordData, pWordData + wordSize);
                    utf8::iterator charEndIt(pWordData + wordSize, pWordData, pWordData + wordSize);
                    float partialWordWidth = 0.0;
                    std::string s1;
                    std::string s2;
                    while (charIt != charEndIt)
                    {
                        uint32_t codepoint = (uint32_t)*charIt;
                        const float charWidth = get_char_scale(codepoint, pFont).x;
                        partialWordWidth += charWidth;
                        if (parentLayout.padding.x + partialWordWidth < parentLayoutWidth)
                        {
                            util::str::append_utf8(codepoint, s1);
                        }
                        else
                        {
                            util::str::append_utf8(codepoint, s2);
                        }
                        ++charIt;
                    }
                    str = s1;
                    Vector2f s1Scale = get_text_scale(str, pFont);
                    wordWidth = s1Scale.x;

                    words.erase(words.begin() + i);
                    words.insert(words.begin() + i, s1);
                    words.insert(words.begin() + i + 1, s2);
                }
                if (lineWidth + wordWidth >= parentLayoutWidth)
                {
                    wrappedText += '\n';
                    lineWidth = parentLayout.padding.x;
                    ++outLineCount;
                }

                wrappedText += str;

                if (!lastWord)
                {
                    wordWidth += spaceWidth;
                    wrappedText += ' ';
                }
                lineWidth += wordWidth;
                outMaxLineWidth = std::max(outMaxLineWidth, lineWidth);
            }
            return wrappedText;
        }

        void set_text(
            UIElement* pTextElement,
            UIElement* pParentElement,
            const std::string& text
        )
        {
            // TODO: Make App, SceneManager and Scene accessing safer here!
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();

            const Layout& parentLayout = pParentElement->getLayout();
            const Font* pFont = pTextElement->getFont();
            float charHeight = static_cast<float>(pFont->getMaxCharHeight());
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (parentLayout.wordWrap == WordWrap::NONE)
            {
                maxLineWidth = get_text_scale(text, pFont).x;
                finalText = text;
            }
            else if (parentLayout.wordWrap == WordWrap::NORMAL)
            {
                finalText = wrap_text(
                    text,
                    pFont,
                    pParentElement,
                    maxLineWidth,
                    lineCount
                );
            }

            GUIRenderable* pRenderable = (GUIRenderable*)pScene->getComponent(
                pTextElement->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            pRenderable->text = finalText;

            pTextElement->setScale({ maxLineWidth, charHeight * lineCount });
        }

        Vector2f get_char_scale(uint32_t codepoint, const Font* pFont)
        {
            const FontGlyphData * const pGlyph = pFont->getGlyph(codepoint);
            if (pGlyph)
            {
                return {
                    static_cast<float>(pGlyph->advance >> 6),
                    static_cast<float>(pFont->getMaxCharHeight())
                };
            }
            return { 0, 0 };
        }

        template <typename T>
        Vector2f get_text_scale(T text, const Font* pFont)
        {
            Vector2f scale(0, static_cast<float>(pFont->getMaxCharHeight()));

            const size_t textSize = text.size();
            const char* pData = (const char*)text.data();
            utf8::iterator it(pData, pData, pData + textSize);
            utf8::iterator endIt(pData + textSize, pData, pData + textSize);
            while (it != endIt)
            {
                uint32_t codepoint = (uint32_t)*it;
                const FontGlyphData * const pGlyph = pFont->getGlyph(codepoint);
                if (pGlyph)
                {
                    scale.x += (float)(pGlyph->advance >> 6);
                }
                ++it;
            }
            return scale;
        }

        template Vector2f get_text_scale<std::string>(std::string text, const Font* pFont);
        template Vector2f get_text_scale<const std::string&>(const std::string& text, const Font* pFont);
        template Vector2f get_text_scale<std::string_view>(std::string_view text, const Font* pFont);
    }
}
