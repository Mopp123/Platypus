#include "Text.h"
#include "platypus/core/Application.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Debug.h"
#include <sstream>


namespace platypus
{
    namespace ui
    {
        UIElement* add_text_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::wstring& text,
            const Vector4f& color,
            const Font* pFont
        )
        {
            Layout parentLayout = pParent->getLayout();

            float charHeight = (float)pFont->getMaxCharHeight();
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::wstring finalText;
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

            Layout layout = parentLayout;
            // NOTE: If word wrapping, this scale isn't really usable for anything, since
            // it's just w*h rect and doesn't hold info about specific line sizes...
            //  -> if want to have some text mouse over, this can't be used for anything
            //  but single line text elements
            layout.scale = { maxLineWidth, charHeight * lineCount };

            UIElement* pElement = add_container(ui, pParent, layout, false, NULL_ID, pFont);
            Debug::log("___TEST___Creating text renderable");
            GUIRenderable* pTextRenderable = create_gui_renderable(
                pElement->getEntityID(),
                color
            );
            pTextRenderable->textureID = pFont->getTextureID();
            pTextRenderable->fontID = pFont->getID();
            pTextRenderable->text = finalText;

            return pElement;
        }


        std::wstring wrap_text(
            const std::wstring& text,
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
                return L"";
            }
            #endif

            outLineCount = 1;
            std::wstring wrappedText;

            float parentLayoutWidth = parentLayout.scale.x - parentLayout.padding.x;

            std::wstring fullText = text;

            std::wistringstream stream(text);
            std::wstring word;
            std::vector<std::wstring> words;
            while (getline(stream, word, L' '))
                words.push_back(word);

            float lineWidth = parentLayout.padding.x;
            float spaceWidth = get_text_scale(L" ", pFont).x;
            for (size_t i = 0; i < words.size(); ++i)
            {
                const std::wstring& str = words[i];
                Vector2f wordScale = get_text_scale(str, pFont);
                float wordWidth = wordScale.x;
                bool lastWord = i == words.size() - 1;

                if (lineWidth + wordWidth >= parentLayoutWidth)
                {
                    wrappedText += L'\n';
                    lineWidth = parentLayout.padding.x;
                    ++outLineCount;
                }

                wrappedText += str;
                if (!lastWord)
                {
                    wordWidth += spaceWidth;
                    wrappedText += L' ';
                }
                lineWidth += wordWidth;
                outMaxLineWidth = std::max(outMaxLineWidth, lineWidth);
            }
            return wrappedText;
        }


        void set_text(
            UIElement* pTextElement,
            UIElement* pParentElement,
            const std::wstring& text
        )
        {
            // TODO: Make App, SceneManager and Scene accessing safer here!
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();

            const Layout& parentLayout = pParentElement->getLayout();
            const Font* pFont = pTextElement->getFont();
            float charHeight = (float)pFont->getMaxCharHeight();
            float maxLineWidth = 0.0f;
            size_t lineCount = 1;

            std::wstring finalText;
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

        Vector2f get_text_scale(const std::wstring& text, const Font* pFont)
        {
            Vector2f scale(0, (float)pFont->getMaxCharHeight());
            for (wchar_t c : text)
            {
                const FontGlyphData * const pGlyph = pFont->getGlyph(c);
                if (pGlyph)
                {
                    scale.x += (float)(pGlyph->advance >> 6);
                }
            }
            return scale;
        }
    }
}
