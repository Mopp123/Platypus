#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "platypus/core/InputManager.h"
#include "platypus/core/InputEvent.h"
#include "platypus/ecs/Entity.h"
#include "platypus/assets/Font.h"
#include <vector>
#include <string>


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

            Vector2f _previousItemPosition;
            Vector2f _previousItemScale;

            MouseEnterEvent* _pMouseEnterEvent = nullptr;
            MouseOverEvent* _pMouseOverEvent = nullptr;
            MouseExitEvent* _pMouseExitEvent = nullptr;
            OnClickEvent* _pOnClickEvent = nullptr;

            bool _isMouseOver = false;

        public:
            UIElement(
                entityID_t entityID,
                Layout layout,
                MouseEnterEvent* pMouseEnterEvent,
                MouseOverEvent* pMouseOverEvent,
                MouseExitEvent* pMouseExitEvent,
                OnClickEvent* pOnClickEvent
            );
            ~UIElement();

            const entityID_t getEntityID() const { return _entityID; }
            const Layout& getLayout() const { return _layout; }
        };

        class LayoutUI
        {
        public:
            struct Config
            {
                Vector4f buttonColor = Vector4f(0.4f, 0.4f, 0.4f, 1.0f);
                Vector4f buttonHighlightColor = Vector4f(0.55f, 0.55f, 0.55f, 1.0f);
                Vector4f buttonTextColor = Vector4f(1, 1, 1, 1);
                Vector4f buttonTextHighlightColor = Vector4f(0.1f, 0.1f, 0.1f, 1);
            };

            static Config s_config;

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

            std::vector<UIElement*> _elements;
            std::vector<UIElement*> _rootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager);
            ~LayoutUI();

            void update(
                const UIElement* pElement,
                const UIElement* pParentElement,
                const Vector2f& previousItemPosition = Vector2f(0, 0),
                const Vector2f& previousItemScale = Vector2f(0, 0),
                int childIndex = 0
            );

            UIElement* addContainer(
                UIElement* pParent,
                const Layout& layout,
                UIElement::MouseEnterEvent* pMouseEnterEvent = nullptr,
                UIElement::MouseOverEvent* pMouseOverEvent = nullptr,
                UIElement::MouseExitEvent* pMouseExitEvent = nullptr,
                UIElement::OnClickEvent* pOnClick = nullptr
            );

            void createImage(UIElement* pElement, ID_t textureID);
            UIElement* addTextElement(
                UIElement* pParent,
                const std::wstring& text,
                const Vector4f& color,
                const Font* pFont,
                UIElement::MouseEnterEvent* pMouseEnterEvent = nullptr,
                UIElement::MouseOverEvent* pMouseOverEvent = nullptr,
                UIElement::MouseExitEvent* pMouseExitEvent = nullptr,
                UIElement::OnClickEvent* pOnClick = nullptr
            );

            UIElement* addButtonElement(
                UIElement* pParent,
                const std::wstring& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick = nullptr
            );

            static Vector2f get_text_scale(const std::wstring& text, const Font* pFont);

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
