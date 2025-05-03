#include "Text.h"
#include "platypus/ecs/components/Renderable.h"


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
            Layout layout = pParent->getLayout();
            layout.scale = get_text_scale(text, pFont);

            UIElement* pElement = add_container(ui, pParent, layout, false);
            GUIRenderable* pTextRenderable = create_gui_renderable(
                pElement->getEntityID(),
                color
            );
            pTextRenderable->textureID = pFont->getTextureID();
            pTextRenderable->fontID = pFont->getID();
            pTextRenderable->text = text;

            return pElement;
        }


        Vector2f get_text_scale(const std::wstring& text, const Font* pFont)
        {
            Vector2f scale(0, (float)pFont->getMaxCharHeight());
            for (wchar_t c : text)
            {
                const FontGlyphData * const glyph = pFont->getGlyph(c);
                if (glyph)
                {
                    scale.x += ((float)(glyph->advance >> 6));
                }
            }
            return scale;
        }
    }
}
