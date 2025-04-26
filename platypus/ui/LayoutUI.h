#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/InputEvent.h"
#include "platypus/ecs/Entity.h"
#include <vector>


#define NULL_COLOR Vector4f(0, 0, 0, 0)

#define VPIXEL_T(x) {platypus::ui::VType::PX, x}
#define VPERCENT_T(x) {platypus::ui::VType::PR, x}

/*
    Issues with prev UI systems:
        *That fucked up constraint thing
        *Unable to specify "containers'" layout for it's elements / child components


    TODO:
        *Create the full "layout composition", immediate mode style
            -> have some "pool" of components to use for rendering
                -> render the layout composition

        *System where I can just say, give me some container, specify how it should look,
        specify way it's added elements will be layed out, how things should scale

        *Container can have Elements and other Containers whose layout depends on their parent container
*/

namespace platypus
{
    namespace ui
    {
        enum class VType
        {
            PX, // pixel
            PR, // percent
            F // float
        };

        struct Value
        {
            VType type = VType::PX;
            float value = 0.0f;
        };

        enum class HorizontalAlignment
        {
            NONE,
            LEFT,
            CENTER,
            RIGHT
        };

        enum class VerticalAlignment
        {
            NONE,
            TOP,
            CENTER,
            BOTTOM
        };

        enum class ExpandElements
        {
            DOWN,
            RIGHT,
            UP,
            LEFT
        };

        struct Layout
        {
            Value position[2];
            Value width;
            Value height;
            Vector4f color = NULL_COLOR;
            Value paddingX;
            Value paddingY;
            // If align != NONE, it overrides position
            HorizontalAlignment horizontalAlignment = HorizontalAlignment::NONE;
            VerticalAlignment verticalAlignment = VerticalAlignment::NONE;
            ExpandElements expandElements = ExpandElements::DOWN;
            uint32_t layer = 0;
            bool visible = true;
        };

        struct UIElement
        {
            entityID_t entityID = NULL_ENTITY_ID;
            Layout layout;
        };

        class LayoutUI
        {
        private:
            class ResizeEvent : public WindowResizeEvent
            {
            public:
                LayoutUI& _uiRef;
                ResizeEvent(LayoutUI& uiRef) : _uiRef(uiRef) {}
                virtual void func(int w, int h);
            };

            Scene* _pScene = nullptr;

            int _windowWidth = 0;
            int _windowHeight = 0;

            std::vector<UIElement> _elements;

        public:
            void init(Scene* pScene, InputManager& inputManager);
            UIElement& createElement(const Layout& layout);
            UIElement& addChild(UIElement& parent, const Layout& layout);

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
