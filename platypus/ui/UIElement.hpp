#pragma once

#include "platypus/utils/Maths.hpp"

#include "platypus/core/Scene.hpp"
#include "platypus/core/InputEvent.hpp"

#include "platypus/ecs/Entity.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/assets/Font.hpp"
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

        enum class WordWrap
        {
            NONE,
            NORMAL
        };

        enum StretchFitContentFlagBits
        {
            STRETCH_FIT_CONTENT_HORIZONTALLY = 0x1,
            STRETCH_FIT_CONTENT_VERTICALLY = 0x1 << 1
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
            float elementGap = 0.0f;
            ValueType elementGapType = ValueType::PIXEL;
            uint32_t layer = 0;
            //std::vector<Layout> children = { };

            WordWrap wordWrap = WordWrap::NONE;

            Vector4f borderColor = Vector4f(0, 0, 0, 0);
            uint32_t borderThickness = 0;

            uint32_t stretchFitContentFlags = 0;

            /*
            Layout(const Layout& other) :
                position(other.position),
                scale(other.scale),
                color(other.color),
                padding(other.padding),
                horizontalAlignment(other.horizontalAlignment),
                verticalAlignment(other.verticalContentAlignment),
                horizontalContentAlignment(other.horizontalContentAlignment),
                verticalContentAlignment(other.verticalContentAlignment),
                expandElements(other.expandElements),
                elementGap(other.elementGap),
                elementGapType(other.elementGapType),
                layer(other.layer),
                children(other.children),
                wordWrap(other.wordWrap),
                borderColor(other.borderColor),
                borderThickness(other.borderThickness),
                stretchFitContentFlags(other.stretchFitContentFlags)
            {}
            */
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

            class DragEvent
            {
            public:
                virtual ~DragEvent() {}
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
            DragEvent* _pDragEvent = nullptr;
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
                Scene* _pScene = nullptr;
                UIElement* _pElement = nullptr;
                ElementMouseButtonEvent(
                    Scene* pScene,
                    UIElement* pElement
                ) :
                    _pScene(pScene),
                    _pElement(pElement)
                {}
                ~ElementMouseButtonEvent() {}
                virtual void func(MouseButtonName button, InputAction action, int mods);
            };

            entityID_t _entityID = NULL_ENTITY_ID;
            Layout _layout;
            const Font* _pFont = nullptr;
            std::vector<UIElement*> _children;

            bool _isMouseOver = false;
            bool _dragged = false;

        public:
            UIElement(
                entityID_t entityID,
                Layout layout,
                const Font* pFont,
                OnClickEvent* pOnClickEvent
            );
            ~UIElement();

            void addChild(
                UIElement* pChild,
                const Vector2f& childPosition,
                const Vector2f& childScale
            );
            // Used to update current element according to child.
            // Used to set _previousItemPosition, _previousItemScale
            // and possibly stretching current element to make the child
            // fit in.
            void updateFromChild(
                UIElement* pChild,
                const Vector2f& childPosition,
                const Vector2f& childScale
            );

            void setScale(const Vector2f& scale);
            Vector2f getGlobalScale() const;
            void setGlobalPosition(const Vector2f& position);
            Vector2f getGlobalPosition() const;
            GUIRenderable* getRenderable();
            void setActive(bool arg);

            inline const entityID_t getEntityID() const { return _entityID; }
            inline const Layout& getLayout() const { return _layout; }
            inline const Font* getFont() const { return _pFont; }
            inline const std::vector<UIElement*>& getChildren() const { return _children; }
            inline bool isMouseOver() const { return _isMouseOver; }
        };


        UIElement* add_container(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            bool createRenderable,
            ID_t textureID = NULL_ID,
            const Font* pFont = nullptr,
            UIElement::OnClickEvent* pOnClickEvent = nullptr
        );

        void update_containers(
            LayoutUI& ui,
            UIElement* pParent,
            UIElement* pElement,
            const Layout& layout
        );
    }
}
