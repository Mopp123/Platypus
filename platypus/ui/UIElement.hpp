#pragma once

#include "platypus/utils/Maths.hpp"

#include "platypus/core/Scene.hpp"
#include "platypus/core/InputEvent.hpp"

#include "platypus/ecs/Entity.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/assets/Font.hpp"
#include <vector>


#define NULL_COLOR Vector4f(0, 0, 0, 0)


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

        enum EffectOnParentFlagBits
        {
            NONE = 0,
            STRETCH_HORIZONTALLY = 0x1,
            STRETCH_VERTICALLY = 0x1 << 1,
            INCREMENT_POSITION = 0x1 << 2
        };

        inline constexpr uint32_t DEFAULT_EFFECT_ON_PARENT_FLAGS = (EffectOnParentFlagBits::STRETCH_HORIZONTALLY | EffectOnParentFlagBits::STRETCH_VERTICALLY | EffectOnParentFlagBits::INCREMENT_POSITION);

        struct Layout
        {
            Vector2f position;
            Vector2f scale;
            Vector4f color = NULL_COLOR;
            Vector2f padding;

            uint32_t effectOnParentFlags = DEFAULT_EFFECT_ON_PARENT_FLAGS;

            // NOTE: Parent's content alignment override child's own alignment
            HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = VerticalAlignment::TOP;

            HorizontalAlignment horizontalContentAlignment = HorizontalAlignment::LEFT;
            VerticalAlignment verticalContentAlignment = VerticalAlignment::TOP;

            ExpandElements expandElements = ExpandElements::DOWN;
            float elementGap = 0.0f;
            ValueType elementGapType = ValueType::PIXEL;
            uint32_t layer = 0;

            WordWrap wordWrap = WordWrap::NONE;

            Vector4f borderColor = Vector4f(0, 0, 0, 0);
            uint32_t borderThickness = 0;
        };

        class LayoutUI;
        class UIElement
        {
        private:
            LayoutUI& _uiRef;

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

            ElementCursorPosEvent* _pCursorPosEvent = nullptr;
            ElementMouseButtonEvent* _pMouseButtonEvent = nullptr;

            entityID_t _entityID = NULL_ENTITY_ID;
            Layout _layout;
            const Font* _pFont = nullptr;
            UIElement* _pParent = nullptr;
            std::vector<UIElement*> _children;

            bool _isMouseOver = false;
            bool _dragged = false;
            bool _selected = false; // NOTE: this is atm only used by InputField

            // *reset back to false at updatePosition(...)
            // (since pos update has to be done after scale update)
            bool _updatePending = false;

        public:
            static std::set<uint32_t> s_mouseOverLayers;

            UIElement(
                LayoutUI& ui,
                entityID_t entityID,
                Layout layout,
                const Font* pFont,
                OnClickEvent* pOnClickEvent
            );
            ~UIElement();

            // NOTE: Via current tree updating system, could get rid of this?
            void addChild(
                UIElement* pChild,
                const Vector2f& childPosition,
                const Vector2f& childScale
            );
            // Destroys child tree recursively
            // NOTE: Very shit and inefficient, just testing atm!
            void destroyChildren();
            void destroyChild(UIElement* pChild);

            // TODO: Disable modifying scale and position directly
            //  -> should be rather changed via element's layout!
            //void setScale(const Vector2f& scale);
            void setLayoutScale(Vector2f scale);
            Vector2f getGlobalScale() const;
            void setLayoutPosition(const Vector2f& position);
            Vector2f getGlobalPosition() const;
            const GUITransform* getTransform() const;
            GUIRenderable* getRenderable();
            inline UIElement* getParent() const { return _pParent; }
            UIElement* getRootParent();
            void setActive(bool arg);
            bool isActive();

            uint32_t getLayer();
            void setLayer(uint32_t layer);

            // Updates global scales for the whole element tree recursively
            void updateScale();
            // Updates global positions for the whole element tree recursively
            // NOTE: Has to be called AFTER updating all element tree scales using above!
            void updatePosition(
                size_t childIndex,
                const Vector2f& previousItemPosition,
                const Vector2f& previousItemScale
            );
            // Updates scales and positions for the whole tree
            void updateTree();

            static uint32_t mouse_over_layer();

            inline const entityID_t getEntityID() const { return _entityID; }
            inline const Layout& getLayout() const { return _layout; }
            inline const Font* getFont() const { return _pFont; }
            inline const std::vector<UIElement*>& getChildren() const { return _children; }
            inline bool isMouseOver() const { return _isMouseOver; }
            inline void setSelected(bool arg) { _selected = arg; }
            inline bool isSelected() const { return _selected; }
            inline bool isUpdatePending() const { return _updatePending; }

        private:
            GUITransform* getTransform();
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
    }
}
