#include "Text.hpp"
#include "UIManager.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/utils/StringUtils.hpp"
#include <sstream>
#include <utf8.h>


namespace platypus
{
    namespace ui
    {
        Text::Text(
            UIManager& uiManager,
            UIElement* pParent,
            const Font* pFont,
            const Layout::Colors colors,
            const std::string& txt,
            uint32_t effectOnParentFlags
        ) :
            UIElement(
                uiManager,
                uiManager.createLayout(), // Need to create for inherited UIElement -> will be modified here immediately tho!
                false,
                NULL_ID,
                nullptr,
                true // NOTE: All mouse input is ignored for Text atm!! TODO: ALLOW MORE CONTROL OVER THIS!
            ),
            _pFont(pFont)
        {
            Layout* pLayout = uiManager.getLayout(_layoutID);
            WordWrap useWordWrap = WordWrap::NONE;
            TextOverflow useTextOverflow = TextOverflow::NONE;
            if (pParent)
            {
                const Layout* pParentLayout = pParent->getLayout();
                useWordWrap = pParentLayout->wordWrap;
                useTextOverflow = pParentLayout->textOverflow;
            }

            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (useWordWrap == WordWrap::NONE)
            {
                if (useTextOverflow == TextOverflow::NONE)
                {
                    finalText = txt;
                    maxLineWidth = get_text_scale(finalText, pFont).x;
                }
                else
                {
                    finalText = strip_text_overflow_ellipsis(
                        pParent,
                        pFont,
                        "",
                        txt,
                        useTextOverflow,
                        &maxLineWidth
                    );
                }
            }
            else if (useWordWrap == WordWrap::NORMAL)
            {
                finalText = wrap_text(
                    txt,
                    pFont,
                    pParent,
                    maxLineWidth,
                    lineCount
                );
            }

            // NOTE: If word wrapping, this scale isn't really usable for anything, since
            // it's just w*h rect and doesn't hold info about specific line sizes...
            //  -> if want to have some text mouse over, this can't be used for anything
            //  but single line text elements
            float totalHeight = static_cast<float>(pFont->getFittingHeight()) * static_cast<float>(lineCount);
            pLayout->scale = { maxLineWidth,  totalHeight };

            pLayout->colors = colors;
            pLayout->effectOnParentFlags = effectOnParentFlags;

            GUIRenderable* pTextRenderable = create_gui_renderable(
                _entityID,
                pFont->getTextureID(),
                pFont->getID(),
                pLayout->colors.base,
                { 0, 0, 0, 0 }, // border color
                0.0f, // border thickness
                { 0, 0 }, // texture offset
                0, // layer
                true, // isText?
                finalText
            );
        }

        // TODO: Replace this with the new one!
        void Text::set(
            UIElement* pParentElement,
            const std::string& text
        )
        {
            PLATYPUS_ASSERT(pParentElement);

            // TODO: Make App, SceneManager and Scene accessing safer here!
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            const Layout* pParentLayout = pParentElement->getLayout();
            float charHeight = static_cast<float>(_pFont->getFittingHeight());
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (pParentLayout->wordWrap == WordWrap::NONE)
            {
                if (pParentLayout->textOverflow == TextOverflow::NONE)
                {
                    maxLineWidth = get_text_scale(text, _pFont).x;
                    finalText = text;
                }
                else
                {
                    finalText = strip_text_overflow_ellipsis(
                        pParentElement,
                        _pFont,
                        "", // header... which shouldn't probably be used anymore...
                        text,
                        pParentLayout->textOverflow,
                        &maxLineWidth
                    );
                }
            }
            else if (pParentLayout->wordWrap == WordWrap::NORMAL)
            {
                finalText = wrap_text(
                    text,
                    _pFont,
                    pParentElement,
                    maxLineWidth,
                    lineCount
                );
            }

            GUIRenderable* pRenderable = (GUIRenderable*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            pRenderable->text = finalText;
            setLayoutScale({ maxLineWidth, charHeight * lineCount });
        }

        void Text::set(const std::string& text)
        {
            PLATYPUS_ASSERT(_pParent);

            // TODO: Make App, SceneManager and Scene accessing safer here!
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            const Layout* pParentLayout = _pParent->getLayout();
            float charHeight = static_cast<float>(_pFont->getFittingHeight());
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (pParentLayout->wordWrap == WordWrap::NONE)
            {
                if (pParentLayout->textOverflow == TextOverflow::NONE)
                {
                    maxLineWidth = get_text_scale(text, _pFont).x;
                    finalText = text;
                }
                else
                {
                    finalText = strip_text_overflow_ellipsis(
                        _pParent,
                        _pFont,
                        "", // header... which shouldn't probably be used anymore...
                        text,
                        pParentLayout->textOverflow,
                        &maxLineWidth
                    );
                }
            }
            else if (pParentLayout->wordWrap == WordWrap::NORMAL)
            {
                finalText = wrap_text(
                    text,
                    _pFont,
                    _pParent,
                    maxLineWidth,
                    lineCount
                );
            }

            GUIRenderable* pRenderable = (GUIRenderable*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            pRenderable->text = finalText;
            setLayoutScale({ maxLineWidth, charHeight * lineCount });
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
            const Layout* pParentLayout = pParentElement->getLayout();
            #ifdef PLATYPUS_DEBUG
            if (pParentLayout->wordWrap != WordWrap::NORMAL)
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

            float parentLayoutWidth = pParentLayout->scale.x - pParentLayout->padding.x;

            std::istringstream stream(text);
            std::string word;
            std::vector<std::string> words;
            // NOTE: Not sure if splitting into words safe enough if varying unicode char
            // sizes? -> Should be tho?
            while (getline(stream, word, ' '))
                words.push_back(word);

            float lineWidth = pParentLayout->padding.x;
            float spaceWidth = get_char_scale(0x20, pFont).x;
            for (size_t i = 0; i < words.size(); ++i)
            {
                std::string str = words[i];
                Vector2f wordScale = get_text_scale(str, pFont);
                float wordWidth = wordScale.x;
                bool lastWord = i == words.size() - 1;

                // If single word goes out of bounds split into 2 words
                if (pParentLayout->padding.x + wordWidth >= parentLayoutWidth)
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
                        if (pParentLayout->padding.x + partialWordWidth < parentLayoutWidth)
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
                    lineWidth = pParentLayout->padding.x;
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

        // TODO: remove
        /*
        void set_text(
            UIElement* pTextElement,
            UIElement* pParentElement,
            const std::string& text
        )
        {
            // TODO: Make App, SceneManager and Scene accessing safer here!
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();

            const Layout* pParentLayout = pParentElement->getLayout();
            const Font* pFont = pTextElement->getFont();
            float charHeight = static_cast<float>(pFont->getFittingHeight());
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::string finalText;
            if (pParentLayout->wordWrap == WordWrap::NONE)
            {
                maxLineWidth = get_text_scale(text, pFont).x;

                if (pParentLayout->textOverflow == TextOverflow::NONE)
                {
                    finalText = text;
                }
                else
                {
                    finalText = strip_text_overflow_ellipsis(
                        pParentElement,
                        pFont,
                        "", // header... which shouldn't probably be used anymore...
                        text,
                        pParentLayout->textOverflow
                    );
                }
            }
            else if (pParentLayout->wordWrap == WordWrap::NORMAL)
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
            pTextElement->setLayoutScale({ maxLineWidth, charHeight * lineCount });
        }
        */


        std::string strip_text_overflow_ellipsis(
            UIElement* pParentElement,
            const Font* pFont,
            const std::string& header,
            const std::string& text,
            TextOverflow overflow,
            float* pOutWidth
        )
        {
            PLATYPUS_ASSERT((overflow == TextOverflow::ELLIPSIS_LEFT) || (overflow == TextOverflow::ELLIPSIS_RIGHT));
            const std::string fullText = header + text;
            if (fullText.empty())
            {
                if (pOutWidth)
                    *pOutWidth = 0.0f;
                return fullText;
            }

            const float fullVisualWidth = ui::get_text_scale(fullText, pFont).x;

            const Layout* pParentLayout = pParentElement->getLayout();
            const float parentWidth = pParentElement->getGlobalScale().x;
            const float parentContentWidth = parentWidth - pParentLayout->padding.x * 2;
            if (fullVisualWidth < parentContentWidth)
            {
                if (pOutWidth)
                    *pOutWidth = fullVisualWidth;
                return header + text;
            }

            const size_t size = text.size();
            char* pData = (char*)text.data();
            //utf8::iterator<char*> it(pData + size - 1, pData, pData + size);
            //utf8::iterator<char*> beginIt(pData, pData, pData + size);
            utf8::iterator<char*> it;
            utf8::iterator<char*> stopIt;
            if (overflow == TextOverflow::ELLIPSIS_LEFT)
            {
                // Need to start past the last element and decrement the it immediately
                // if the last char more than 1 byte so were at the actual last element's
                // beginning!
                it = utf8::iterator<char*>(pData + size, pData, pData + size);
                --it;
                stopIt = utf8::iterator<char*>(pData, pData, pData + size);
            }
            //if (overflow == TextOverflow::ELLIPSIS_RIGHT)
            else
            {
                it = utf8::iterator<char*>(pData, pData, pData + size);
                stopIt = utf8::iterator<char*>(pData + size, pData, pData + size);
                // Need to decrement the stopIt to get the actual last elem ( similar thing
                // to the above case -> last elem might not be pData + size - 1, if its more
                // than one bytes!)
                --stopIt;
            }

            const std::string visualBuffer = "...";
            const float visualBufferWidth = ui::get_text_scale(visualBuffer, pFont).x;

            float headerVisualWidth = ui::get_text_scale(header, pFont).x;
            float currentWidth = headerVisualWidth + visualBufferWidth;
            std::string strippedStr;
            while (true)
            {
                uint32_t codepoint = static_cast<uint32_t>(*it);
                std::string charStr;
                util::str::append_utf8(codepoint, charStr);
                float charVisualWidth = ui::get_text_scale(charStr, pFont).x;

                if (currentWidth + charVisualWidth < parentContentWidth)
                {
                    currentWidth += charVisualWidth;
                    util::str::append_utf8(codepoint, strippedStr);
                }

                if (it == stopIt)
                    break;

                if (overflow == TextOverflow::ELLIPSIS_LEFT)
                    --it;
                else
                    ++it;
            }

            if (pOutWidth)
                *pOutWidth = currentWidth;
            if (overflow == TextOverflow::ELLIPSIS_LEFT)
                return header + visualBuffer + util::str::reverse(strippedStr);
            else
                return header + strippedStr + visualBuffer;
        }


        Vector2f get_char_scale(uint32_t codepoint, const Font* pFont)
        {
            const FontGlyphData * const pGlyph = pFont->getGlyph(codepoint);
            if (pGlyph)
            {
                return {
                    static_cast<float>(pGlyph->advance >> 6),
                    static_cast<float>(pFont->getFittingHeight())
                    //static_cast<float>(pFont->getMaxCharHeight())
                };
            }
            return { 0, 0 };
        }

        template <typename T>
        Vector2f get_text_scale(T text, const Font* pFont)
        {
            Vector2f scale(
                0,
                static_cast<float>(pFont->getFittingHeight())
            );

            const size_t textSize = text.size();
            const char* pData = static_cast<const char*>(text.data());
            utf8::iterator it(pData, pData, pData + textSize);
            utf8::iterator endIt(pData + textSize, pData, pData + textSize);
            while (it != endIt)
            {
                uint32_t codepoint = static_cast<uint32_t>(*it);
                const FontGlyphData * const pGlyph = pFont->getGlyph(codepoint);
                if (pGlyph)
                {
                    scale.x += static_cast<float>(pGlyph->advance >> 6);
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
