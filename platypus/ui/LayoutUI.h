#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/InputEvent.h"
#include "platypus/ecs/Entity.h"
#include <vector>


#define NULL_COLOR Vector4f(0, 0, 0, 0)
/*
    Issues with prev UI systems:
        *That fucked up constraint thing
        *Unable to specify "containers'" layout for it's elements / child components


    NEXT UP:
        *Make it possible for "container" to scale to fit all it's elements automatically

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

        enum class HorizontalConstraint
        {
            LEFT,
            CENTER,
            RIGHT
        };

        enum class VerticalConstraint
        {
            TOP,
            CENTER,
            BOTTOM
        };

        struct Layout
        {
            Vector2f position;
            Vector2f scale;
            Vector4f color = NULL_COLOR;
            Vector2f padding;

            bool fullscreen = false;

            HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = VerticalAlignment::TOP;
            ExpandElements expandElements = ExpandElements::DOWN;
            float elementGap;
            ValueType elementGapType = ValueType::PIXEL;
            uint32_t layer = 0;
            std::vector<Layout> children;
        };

        struct UIElement
        {
            entityID_t entityID = NULL_ENTITY_ID;
            Layout layout;
            std::vector<size_t> childIndices;
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
            std::vector<size_t> _rootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager);

            size_t create(
                const Layout& layout,
                const Layout* pParentLayout,
                entityID_t parentEntity,
                const Vector2f& previousItemPosition = Vector2f(0, 0),
                const Vector2f& previousItemScale = Vector2f(0, 0),
                int childIndex = 0
            );

            void update(
                const UIElement& element,
                const UIElement* pParentElement,
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
                const Vector2f& scale,
                const Vector2f& previousItemPosition,
                const Vector2f& previousItemScale,
                int childIndex = 0
            );
            float toPercentage(float v1, float v2);
        };
    }
}
