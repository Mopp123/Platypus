#pragma once

#include "UIElement.hpp"
#include "Layout.hpp"
#include "platypus/assets/Font.hpp"
#include <string>


namespace platypus
{
    namespace ui
    {
        class UIManager;
        class Text : public UIElement
        {
        private:
            const Font* _pFont = nullptr;

        protected:
            friend class UIManager;
            // Protected since some other UIElement may extend this
            // TODO: Don't create new layout for each Text
            //  -> rather specify same layout for multiple Texts and have some "individual scale"
            //  for each UIElement (could the existing global scale be enough?)
            Text(
                UIManager& uiManager,
                UIElement* pParent,
                const Font* pFont,
                const Vector4f& color,
                const Vector4f& hoverColor,
                const Vector4f& selectedColor,
                const std::string& txt,
                uint32_t effectOnParentFlags
            );
            ~Text() { }

        public:
            void set(
                UIElement* pParentElement,
                const std::string& text
            );
            // TODO: replace above with below!
            void set(const std::string& text);

            inline const Font* getFont() const { return _pFont; }
        };

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
        );

        // Forces text to fit inside the parent.
        // If overflow happens, inserts "..." to indicate that.
        // Strips either the beginning or end of the inputted text.
        //  stripDirection == 0 : strips the end of the string
        //  stripDirection == 1 : strips the beginning of the string
        //
        // Header can be inserted to get result like:
        //  Current dir: .../Documents/someDir
        //
        // NOTE: Inefficient as fuck! +potentially dangerous:D
        //  -> Added to quickly make a filebrowser
        std::string strip_text_overflow_ellipsis(
            UIElement* pParentElement,
            const Font* pFont,
            const std::string& header,
            const std::string& text,
            TextOverflow overflow, //= TextOverflow::ELLIPSIS_RIGHT
            float* pOutWidth = nullptr
        );

        Vector2f get_char_scale(uint32_t codepoint, const Font* pFont);

        template <typename T>
        Vector2f get_text_scale(T text, const Font* pFont);
    }
}
