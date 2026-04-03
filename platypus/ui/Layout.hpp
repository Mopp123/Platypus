#pragma once

#include "platypus/utils/Maths.hpp"
#include <cstdint>


#define NULL_COLOR Vector4f(0, 0, 0, 0)
#define ERROR_COLOR Vector4f(1, 0, 1, 1)


namespace platypus
{
    namespace ui
    {
        enum class FlowProperty
        {
            DYNAMIC,
            ABSOLUTE
        };

        enum class ValueType
        {
            PIXEL,
            PERCENT
        };

        enum class HorizontalAlignment
        {
            LEFT,
            CENTER,
            RIGHT
        };

        enum class VerticalAlignment
        {
            TOP,
            CENTER,
            BOTTOM
        };

        enum class ExpandElements
        {
            DOWN,
            RIGHT
        };

        enum class WordWrap
        {
            NONE,
            NORMAL
        };

        enum class TextOverflow
        {
            NONE,
            ELLIPSIS_RIGHT,
            ELLIPSIS_LEFT
        };

        enum EffectOnParentFlagBits
        {
            NONE = 0,
            STRETCH_HORIZONTALLY = 0x1,
            STRETCH_VERTICALLY = 0x1 << 1,
            INCREMENT_POSITION = 0x1 << 2
        };

        inline constexpr uint32_t DEFAULT_EFFECT_ON_PARENT_FLAGS = (EffectOnParentFlagBits::STRETCH_HORIZONTALLY | EffectOnParentFlagBits::STRETCH_VERTICALLY | EffectOnParentFlagBits::INCREMENT_POSITION);


        class Layout
        {
        private:
            friend class UIManager;
            Layout()
            {}

        public:
            Layout& operator=(const Layout& other) = delete;

            int32_t id = -1;

            Vector2f position;
            Vector2f scale;
            Vector4f color = NULL_COLOR;
            Vector2f padding;

            // Layer in relation to the parent's layer NOT the actual
            // used layer for rendering (that gets eventually calculated from the
            // layout's layer)
            uint32_t layer = 0;

            uint32_t effectOnParentFlags = DEFAULT_EFFECT_ON_PARENT_FLAGS;

            // NOTE: Parent's content alignment override child's own alignment
            HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = VerticalAlignment::TOP;

            HorizontalAlignment horizontalContentAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalContentAlignment = VerticalAlignment::TOP;

            ExpandElements expandElements = ExpandElements::DOWN;
            float elementGap = 0.0f;
            ValueType elementGapType = ValueType::PIXEL;

            WordWrap wordWrap = WordWrap::NONE;
            TextOverflow textOverflow = TextOverflow::NONE;

            Vector4f hoverColor = ERROR_COLOR;
            Vector4f selectedColor = ERROR_COLOR;

            Vector4f borderColor = NULL_COLOR;
            uint32_t borderThickness = 0;
        };
    }
}
