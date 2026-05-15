#pragma once

#include "platypus/utils/Maths.hpp"

#include "platypus/core/Scene.hpp"
#include "platypus/core/InputEvent.hpp"

#include "platypus/assets/Font.hpp"

#include "platypus/ecs/Entity.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include <map>


namespace platypus
{
    namespace ui
    {
        class UIManager;
        class Layout;
        class UIElement
        {
        protected:
            UIManager& _managerRef;

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

        protected:
            friend class UIManager;

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
            int32_t _layoutID = -1;
            const Font* _pFont = nullptr;

            bool _overrideScale = false;
            Vector2f _overrideScaleValue;

            // NOTE: Parent is currently set by UIManager when creating the UIElement if necessary
            UIElement* _pParent = nullptr;
            std::vector<UIElement*> _children;

            void(*_pOnAddChild)(void*) = nullptr;
            void* _pOnAddChildUserData = nullptr;

            bool _isCursorOver = false;
            bool _dragged = false;
            bool _selected = false; // NOTE: this is atm only used by InputField
            bool _groupRoot = false;

            // *reset back to false at updatePosition(...)
            // (since pos update has to be done after scale update)
            bool _updatePending = false;

            // *Gets updated @updatePosition(...)
            uint32_t _absoluteLayer = 0;

            UIElement(
                UIManager& uiManager,
                UIElement* pParent,
                const Layout* pLayout,
                bool createRenderable,
                UUID_t textureID,
                const Font* pFont,
                OnClickEvent* pOnClickEvent,
                bool ignoreInput = false
            );

        public:
            static std::map<uint32_t, std::set<entityID_t>> s_cursorOverLayers;
            virtual ~UIElement();

            inline void setGroupRoot_TEST(bool arg) { _groupRoot = arg; }

            void configureOnAddChild(void(*pFunc)(void*), void* pUserData);
            // *Also updates the whole tree so the scales and positions are
            // correct immediately.
            void addChild(UIElement* pChild);
            void removeChild(UIElement* pChild);

            // Destroys child tree recursively
            // NOTE: Very shit and inefficient, just testing atm!
            void destroyChildren();
            void destroyChild(UIElement* pChild);

            void setLayoutScale(Vector2f scale);
            void setLayoutPosition(const Vector2f& position);
            void setLayoutColor(const Vector4f& color);
            void setLayoutHoverColor(const Vector4f& color);
            void setLayoutSelectedColor(const Vector4f& color);
            Vector2f getGlobalScale() const;
            Vector2f getGlobalPosition() const;
            const GUITransform* getTransform() const;
            GUIRenderable* getRenderable();
            inline UIElement* getParent() const { return _pParent; }
            UIElement* getRootParent();
            virtual void setActive(bool arg);
            bool isActive();

            // returns set of all used layers in the tree
            void fetchAbsoluteTreeLayers(std::set<uint32_t>& outLayers);
            void fetchAbsoluteTreeLayers(std::map<uint32_t, size_t>& outLayers);
            uint32_t getTopTreeLayer();

            // Updates global scales for the whole element tree recursively
            void updateScale();
            // Updates width and/or height according to parent if layout demands it
            // NOTE: This was a quick fix for one issue, this could be fucked...
            void updateInheritedScale(Vector2f parentScale);

            // Updates global positions for the whole element tree recursively
            // NOTE: Has to be called AFTER updating all element tree scales using above!
            void updatePosition(Vector2f& cumulatedScale);
            // Updates scales and positions for the whole tree
            void updateTree();

            void triggerFullTreeUpdate();

            void fetchTreeElements(std::vector<UIElement*>& outElements);
            // Returns true if cursor is over any element in the tree starting from
            // the called element (obviously:D)
            bool isCursorOverTree() const;

            // Sets the tree selected and updates correct colors according to layout
            void setSelected(bool arg);

            void setRelativeLayer(uint32_t relativeLayer);
            uint32_t getRelativeLayer() const;
            uint32_t getAbsoluteLayer() const;

            Layout* getLayout() const;

            static uint32_t get_cursor_over_layer();

            inline const entityID_t getEntityID() const { return _entityID; }
            inline void overrideScale(const Vector2f& scale) { _overrideScale = true; _overrideScaleValue = scale; }
            inline const std::vector<UIElement*>& getChildren() const { return _children; }
            inline bool isCursorOver() const { return _isCursorOver; }
            inline bool isSelected() const { return _selected; }
            inline bool isUpdatePending() const { return _updatePending; }

            static void add_to_cursor_over_layers(uint32_t absoluteLayer, entityID_t entityID);
            static void remove_from_cursor_over_layers(uint32_t absoluteLayer, entityID_t entityID);

        protected:
            void setRenderLayer(uint32_t renderLayer);
            GUITransform* getTransform();
        };
    }
}
