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
            // Used to update current element according to child.
            // Used to set _previousItemPosition, _previousItemScale
            // and possibly stretching current element to make the child
            // fit in.
            void updateFromChild(
                UIElement* pChild,
                const Vector2f& childPosition,
                const Vector2f& childScale
            );

            static Vector2f calc_position(
                const Layout& layout,
                const Layout* pParentLayout,
                entityID_t parentEntity,
                const Vector2f& scale,
                const Vector2f& previousItemPosition,
                const Vector2f& previousItemScale,
                bool isFirstChild
            );

            void updatePosition(
                const UIElement* pParent,
                int32_t childIndex = 0
            );

            void getChildBounds(
                float& minX, float& maxX,
                float& minY, float& maxY
            );
            void updateStretching(bool& repositionRequired);

            // Updates the whole UIElement tree.
            // Does rescaling if stretching is enabled.
            //  -> in such case repositions all elements also accordingly.
            // NOTE: Pretty inefficient and awful, but will do for now
            void updateTree(
                const UIElement* pParent,
                int32_t childIndex = 0
            );


            void setScale(const Vector2f& scale);
            Vector2f getGlobalScale() const;
            void setGlobalPosition(const Vector2f& position);
            Vector2f getGlobalPosition() const;
            GUITransform* getTransform();
            const GUITransform* getTransform() const;
            GUIRenderable* getRenderable();
            inline UIElement* getParent() const { return _pParent; }
            UIElement* getRootParent();
            void setActive(bool arg);
            bool isActive();

            uint32_t getLayer();
            void setLayer(uint32_t layer);

            static uint32_t mouse_over_layer();

            inline const entityID_t getEntityID() const { return _entityID; }
            inline const Layout& getLayout() const { return _layout; }
            inline const Font* getFont() const { return _pFont; }
            inline const std::vector<UIElement*>& getChildren() const { return _children; }
            inline bool isMouseOver() const { return _isMouseOver; }
            inline void setSelected(bool arg) { _selected = arg; }
            inline bool isSelected() const { return _selected; }
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

        /*
        void update_containers(
            LayoutUI& ui,
            UIElement* pParent,
            UIElement* pElement,
            const Layout& layout
        );
        */
    }
}
