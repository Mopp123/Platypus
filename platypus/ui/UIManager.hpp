#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/core/Scene.hpp"
#include "Layout.hpp"
#include "UIElement.hpp"
#include "Text.hpp"
#include "Button.hpp"
#include "InputField.hpp"
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
        class UIManager
        {
        private:
            class ResizeEvent : public WindowResizeEvent
            {
            public:
                UIManager& _uiRef;
                ResizeEvent(UIManager& uiRef) : _uiRef(uiRef) {}
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
            ~UIManager();

            Layout* createLayout();
            void copyLayoutAspects(Layout* pTarget, const Layout* pSource);

            void addRootElement(UIElement* pElement);
            void removeRootElement(UIElement* pElement);
            bool isRootElement(UIElement* pElement) const;

            void addToUpdatedElements(UIElement* pElement);
            // Doesn't do anything if no elements' layouts were changed
            void updateChangedElements();

            Layout* getLayout(int32_t id);

            UIElement* createElement(
                UIElement* pParent,
                const Layout* pLayout,
                bool createRenderable,
                ID_t textureID = NULL_ID,
                UIElement::OnClickEvent* pOnClickEvent = nullptr
            );

            Text* createText(
                UIElement* pParent,
                const Font* pFont,
                const Layout::Colors& colors,
                const std::string& txt,
                uint32_t effectOnParentFlags = DEFAULT_EFFECT_ON_PARENT_FLAGS
            );

            Button* createButton(
                UIElement* pParent,
                const Layout* pLayout,
                const Layout::Colors& textColors,
                uint32_t textEffectOnParentFlags,
                const std::string& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick,
                UIElement::MouseEnterEvent* pOnEnter = nullptr,
                UIElement::MouseExitEvent* pOnExit = nullptr
            );

            InputField* createInputField(
                UIElement* pParent,
                const Layout* pLayout,
                TextOverflow overflow,
                const Layout::Colors& textColors,
                const std::string& infoText,
                const Font* pFont,
                void(*pUserOnClickFunc)(void*) = nullptr,
                void* pOnClickUserData = nullptr,
                void(*pOnInputCharFunc)(const std::string&, void*) = nullptr,
                void* pOnInputCharUserData = nullptr
            );

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
