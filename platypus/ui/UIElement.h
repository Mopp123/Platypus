#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/ecs/Entity.h"
#include "platypus/assets/Font.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/InputEvent.h"
#include <vector>


#define NULL_COLOR Vector4f(0, 0, 0, 0)


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

            // NOTE: Parent's content alignment override child's own alignment
            HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = VerticalAlignment::TOP;

            HorizontalAlignment horizontalContentAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalContentAlignment = VerticalAlignment::TOP;

            ExpandElements expandElements = ExpandElements::DOWN;
            float elementGap;
            ValueType elementGapType = ValueType::PIXEL;
            uint32_t layer = 0;
            std::vector<Layout> children;
        };

        class LayoutUI;
        class UIElement
        {
        public:
            class MouseEnterEvent
            {
            public:
                virtual ~MouseEnterEvent() {}
                virtual void func(int mx, int my) = 0;
            };

            class MouseOverEvent
            {
            public:
                virtual ~MouseOverEvent() {}
                virtual void func(int mx, int my) = 0;
            };

            class MouseExitEvent
            {
            public:
                virtual ~MouseExitEvent() {}
                virtual void func(int mx, int my) = 0;
            };

            class OnClickEvent
            {
            public:
                virtual ~OnClickEvent() {}
                virtual void func(MouseButtonName button, InputAction action) = 0;
            };

            Vector2f _previousItemPosition;
            Vector2f _previousItemScale;

            MouseEnterEvent* _pMouseEnterEvent = nullptr;
            MouseOverEvent* _pMouseOverEvent = nullptr;
            MouseExitEvent* _pMouseExitEvent = nullptr;
            OnClickEvent* _pOnClickEvent = nullptr;

        private:
            friend class LayoutUI;

            class ElementCursorPosEvent : public CursorPosEvent
            {
            public:
                Scene* _pScene = nullptr;
                UIElement* _pElement = nullptr;

                ElementCursorPosEvent(
                    Scene* pScene,
                    UIElement* pElement
                ) :
                    _pScene(pScene),
                    _pElement(pElement)
                {}
                ~ElementCursorPosEvent() {}
                virtual void func(int x, int y);
            };

            class ElementMouseButtonEvent : public MouseButtonEvent
            {
            public:
                UIElement* _pElement = nullptr;
                ElementMouseButtonEvent(UIElement* pElement) : _pElement(pElement) {}
                ~ElementMouseButtonEvent() {}
                virtual void func(MouseButtonName button, InputAction action, int mods);
            };

            entityID_t _entityID = NULL_ENTITY_ID;
            Layout _layout;
            std::vector<UIElement*> _children;

            bool _isMouseOver = false;

        public:
            UIElement(
                entityID_t entityID,
                Layout layout
            );
            ~UIElement();

            void addChild(
                UIElement* pChild,
                const Vector2f& childPosition,
                const Vector2f& childScale
            );

            inline const entityID_t getEntityID() const { return _entityID; }
            inline const Layout& getLayout() const { return _layout; }
            inline const std::vector<UIElement*>& getChildren() const { return _children; }
        };


        UIElement* add_container(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            bool createRenderable,
            ID_t textureID = NULL_ID
        );
    }
}
