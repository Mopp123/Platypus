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
            HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = VerticalAlignment::TOP;
            ExpandElements expandElements = ExpandElements::DOWN;
            Value elementGap;
            uint32_t layer = 0;
            std::vector<Layout> children;
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

            UIElement create(
                const Layout& layout,
                const Layout* pParentLayout,
                entityID_t parentEntity,
                const Vector2f& previousItemPosition = Vector2f(0, 0),
                const Vector2f& previousItemScale = Vector2f(0, 0),
                int childIndex = 0
            );

        private:
            Vector2f calcPosition(
                const Layout& layout,
                const Layout* pParentLayout,
                entityID_t parentEntity,
                const Vector2f& parentScale, // the actual scale, not it's layout scale
                const Vector2f& scale,
                const Vector2f& previousItemPosition,
                const Vector2f& previousItemScale,
                int childIndex = 0
            );
            Vector2f calcScale(const Layout& layout, const Vector2f& parentScale);
            float calcElementGap(ExpandElements expandType, const Value& value);
            float toPercentage(float v1, float v2);
        };
    }
}
