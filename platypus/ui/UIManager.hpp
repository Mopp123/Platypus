#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/core/Scene.hpp"
#include "Layout.hpp"
#include "UIElement.hpp"
#include "Text.hpp"
#include "Button.hpp"
#include "Checkbox.hpp"
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

            // TODO: Unfuck below! This shit is getting out of hand... once again...
            Layout* _pDefaultTextLayout = nullptr;
            Layout* _pDefaultNonStretchTextLayout = nullptr;

            Layout* _pDefaultButtonLayout = nullptr;
            Layout* _pDefaultButtonTextLayout = nullptr;
            Layout* _pDefaultCheckboxLayout = nullptr;
            Layout* _pDefaultCheckboxBoxLayout = nullptr;

            Layout* _pDefaultInputFieldRootLayout = nullptr;
            Layout* _pDefaultInputFieldLayout = nullptr;
            Layout* _pDefaultHorizontalInputFieldRootLayout = nullptr;
            Layout* _pDefaultHorizontalInputFieldLayout = nullptr;

            Layout* _pDefaultInputFieldCursorLayout = nullptr;

            // NOTE: Why the aren't all of these sets?
            std::vector<Layout*> _layouts;
            std::vector<UIElement*> _rootElements;
            std::set<UIElement*> _updatedRootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager, Font* pDefaultFont);
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
                UUID_t textureID = NULL_UUID,
                UIElement::OnClickEvent* pOnClickEvent = nullptr
            );

            Text* createText(
                UIElement* pParent,
                const Layout* pLayout,
                const std::string& txt,
                const Font* pFont
            );

            Text* createText(
                UIElement* pParent,
                const std::string& txt,
                const Font* pFont
            );

            Button* createButton(
                UIElement* pParent,
                const Layout* pLayout,
                const Layout* pTextLayout,
                const std::string& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick,
                UIElement::MouseEnterEvent* pOnEnter = nullptr,
                UIElement::MouseExitEvent* pOnExit = nullptr
            );

            Button* createButton(
                UIElement* pParent,
                const std::string& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick,
                UIElement::MouseEnterEvent* pOnEnter = nullptr,
                UIElement::MouseExitEvent* pOnExit = nullptr
            );

            Checkbox* createCheckbox(
                UIElement* pParent,
                const Layout* pLayout,
                const Layout* pTextLayout,
                const Layout* pButtonLayout,
                const Layout* pButtonTextLayout,
                const std::string& text,
                const Font* pFont
            );

            Checkbox* createCheckbox(
                UIElement* pParent,
                const std::string& text,
                const Font* pFont
            );

            InputField* createInputField(
                UIElement* pParent,
                const Layout* pLayout,
                const Layout* pTextLayout,
                const Layout* pFieldLayout,
                const Layout* pFieldTextLayout,
                const Layout* pCursorIndicatorLayout,
                const std::string& infoText,
                const Font* pFont,
                void(*pOnFinishInput)(const std::string&, void*) = nullptr,
                void* pOnFinishInputUserData = nullptr,
                void(*pOnInputCharFunc)(const std::string&, void*) = nullptr,
                void* pOnInputCharUserData = nullptr
            );

            InputField* createInputField(
                UIElement* pParent,
                const std::string& infoText,
                const Font* pFont,
                ExpandElements fieldDirection, // is the field to the left or below the info txt
                void(*pOnFinishInput)(const std::string&, void*) = nullptr,
                void* pOnFinishInputUserData = nullptr,
                void(*pOnInputCharFunc)(const std::string&, void*) = nullptr,
                void* pOnInputCharUserData = nullptr
            );

            inline const Layout* getDefaultTextLayout() const { return _pDefaultTextLayout; }
            inline const Layout* getDefaultNonStretchTextLayout() const { return _pDefaultNonStretchTextLayout; }
            inline const Layout* getDefaultButtonLayout() const { return _pDefaultButtonLayout; }
            inline Layout* getDefaultButtonLayout() { return _pDefaultButtonLayout; }
            inline const Layout* getDefaultButtonTextLayout() const { return _pDefaultButtonTextLayout; }
            inline const Layout* getDefaultInputFieldLayout() const { return _pDefaultInputFieldLayout; }
            inline const Layout* getDefaultInputFieldRootLayout() const { return _pDefaultInputFieldRootLayout; }
            inline const Layout* getDefaultInputFieldCursorLayout() const { return _pDefaultInputFieldCursorLayout; }

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
