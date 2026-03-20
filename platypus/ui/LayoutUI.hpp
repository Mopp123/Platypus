#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"
#include "UIElement.hpp"
#include <vector>

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
        class Layout
        {
        private:
            friend class LayoutUI;
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

            Vector4f hoverColor = NULL_COLOR;
            Vector4f selectedColor = NULL_COLOR;

            Vector4f borderColor = NULL_COLOR;
            uint32_t borderThickness = 0;
        };

        class LayoutUI
        {
        private:

            // TODO: Remove
            /*
            struct Config
            {
                Vector4f buttonColor = Vector4f(0.4f, 0.4f, 0.4f, 1.0f);
                Vector4f buttonHighlightColor = Vector4f(0.55f, 0.55f, 0.55f, 1.0f);
                Vector4f buttonTextColor = Vector4f(1, 1, 1, 1);
                Vector4f buttonTextHighlightColor = Vector4f(0.1f, 0.1f, 0.1f, 1);
                Vector4f buttonBorderColor = Vector4f(0, 0, 0, 1);
                float buttonBorderThickness = 0.0f;
            };
            static Config s_config;
            */

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

            std::vector<Layout*> _layouts;
            std::vector<UIElement*> _rootElements;
            std::set<UIElement*> _updatedRootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager);
            ~LayoutUI();

            Layout* createLayout();
            void copyLayoutAspects(Layout* pTarget, const Layout* pSource);

            void addRootElement(UIElement* pElement);
            void removeRootElement(UIElement* pElement);
            bool isRootElement(UIElement* pElement) const;

            void addToUpdatedElements(UIElement* pElement);
            // Doesn't do anything if no elements' layouts were changed
            void updateChangedElements();

            Layout* getLayout(int32_t id);

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
