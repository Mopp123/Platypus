#include "UIManager.hpp"
#include "DefaultLayoutFactory.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include <cstring>


namespace platypus
{
    namespace ui
    {
        void UIManager::ResizeEvent::func(int w, int h)
        {
            _uiRef._windowWidth = (float)w;
            _uiRef._windowHeight = (float)h;
            for (UIElement* pRootElement : _uiRef._rootElements)
                pRootElement->updateTree();
        }

        void UIManager::init(Scene* pScene, InputManager& inputManager, Font* pDefaultFont)
        {
            _pScene = pScene;
            inputManager.addWindowResizeEvent(new ResizeEvent(*this));

            Window& window = Application::get_instance()->getWindow();
            window.getSurfaceExtent(&_windowWidth, &_windowHeight);

            create_default_text_layout(
                *this,
                DEFAULT_EFFECT_ON_PARENT_FLAGS,
                &_pDefaultTextLayout
            );
            create_default_text_layout(
                *this,
                EffectOnParentFlagBits::INCREMENT_POSITION | EffectOnParentFlagBits::STRETCH_VERTICALLY,
                &_pDefaultNonStretchTextLayout
            );
            create_default_button_layout(
                *this,
                &_pDefaultButtonLayout,
                &_pDefaultButtonTextLayout
            );
            create_default_checkbox_layout(
                *this,
                pDefaultFont,
                &_pDefaultCheckboxLayout,
                &_pDefaultCheckboxBoxLayout
            );

            create_default_input_field_layout(
                *this,
                pDefaultFont,
                ExpandElements::DOWN,
                &_pDefaultInputFieldRootLayout,
                &_pDefaultInputFieldLayout
            );
            create_default_input_field_layout(
                *this,
                pDefaultFont,
                ExpandElements::RIGHT,
                &_pDefaultHorizontalInputFieldRootLayout,
                &_pDefaultHorizontalInputFieldLayout
            );
            create_default_input_field_cursor_layout(
                *this,
                pDefaultFont,
                &_pDefaultInputFieldCursorLayout
            );
        }

        UIManager::~UIManager()
        {
            for (UIElement* pElement : _rootElements)
                delete pElement;

            for (Layout* pLayout : _layouts)
                delete pLayout;
        }

        Layout* UIManager::createLayout()
        {
            size_t layoutID = _layouts.size();
            Layout* pLayout = new Layout;
            pLayout->id = layoutID;
            _layouts.push_back(pLayout);
            return pLayout;
        }

        void UIManager::copyLayoutAspects(Layout* pTarget, const Layout* pSource)
        {
            int32_t originalID = pTarget->id;
            memcpy(
                reinterpret_cast<void*>(pTarget),
                reinterpret_cast<const void*>(pSource),
                sizeof(Layout)
            );
            pTarget->id = originalID;
        }

        void UIManager::addRootElement(UIElement* pElement)
        {
            _rootElements.push_back(pElement);
        }

        void UIManager::removeRootElement(UIElement* pElement)
        {
            int32_t eraseIndex = -1;
            for (size_t i = 0; i < _rootElements.size(); ++i)
            {
                if (_rootElements[i] == pElement)
                {
                    eraseIndex = static_cast<int32_t>(i);
                    break;
                }
            }
            if (eraseIndex >= 0)
            {
                _rootElements.erase(_rootElements.begin() + static_cast<size_t>(eraseIndex));
            }
            #ifdef PLATYPUS_DEBUG
            else
            {
                Debug::log(
                    "No root element found",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            if (_updatedRootElements.find(pElement) != _updatedRootElements.end())
                _updatedRootElements.erase(pElement);
        }

        bool UIManager::isRootElement(UIElement* pElement) const
        {
            for (UIElement* pRootElement : _rootElements)
            {
                if (pElement == pRootElement)
                    return true;
            }
            return false;
        }

        void UIManager::addToUpdatedElements(UIElement* pElement)
        {
            UIElement* pElementRootParent = pElement->getRootParent();
            if (_updatedRootElements.find(pElementRootParent) != _updatedRootElements.end())
                return;

            _updatedRootElements.insert(pElementRootParent);
        }

        void UIManager::updateChangedElements()
        {
            for (UIElement* pUpdatedRootElement : _updatedRootElements)
                pUpdatedRootElement->updateTree();

            _updatedRootElements.clear();
        }

        Layout* UIManager::getLayout(int32_t id)
        {
            if (id == -1 || id >= _layouts.size())
            {
                Debug::log(
                    "Invalid layout id " + std::to_string(id) + " "
                    "current _layouts size: " + std::to_string(_layouts.size()),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            return _layouts[static_cast<size_t>(id)];
        }

        UIElement* UIManager::createElement(
            UIElement* pParent,
            const Layout* pLayout,
            bool createRenderable,
            ID_t textureID,
            UIElement::OnClickEvent* pOnClickEvent
        )
        {
            UIElement* pElement = new UIElement(
                *this,
                pParent,
                pLayout,
                createRenderable,
                textureID,
                nullptr, // font
                pOnClickEvent
            );
            // *Need to update the "tree" even if contains only single element
            // so that scale and pos is immediately correct..
            if (!pParent)
            {
                pElement->updateTree();
                addRootElement(pElement);
            }
            return pElement;
        }

        Text* UIManager::createText(
            UIElement* pParent,
            const Layout* pLayout,
            const Font* pFont,
            const std::string& txt
        )
        {
            Text* pText = new Text(
                *this,
                pParent,
                pLayout,
                pFont,
                txt
            );

            if (!pParent)
            {
                pText->updateTree();
                addRootElement(pText);
            }
            return pText;
        }

        Text* UIManager::createText(
            UIElement* pParent,
            const Font* pFont,
            const std::string& txt
        )
        {
            return createText(
                pParent,
                _pDefaultTextLayout,
                pFont,
                txt
            );
        }

        Button* UIManager::createButton(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            Button* pButton = new Button(
                *this,
                pParent,
                pLayout,
                pTextLayout,
                text,
                pFont,
                pOnClick,
                pOnEnter,
                pOnExit
            );

            if (!pParent)
            {
                pButton->updateTree();
                addRootElement(pButton);
            }
            return pButton;
        }

        Button* UIManager::createButton(
            UIElement* pParent,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            return createButton(
                pParent,
                _pDefaultButtonLayout,
                _pDefaultButtonTextLayout,
                text,
                pFont,
                pOnClick,
                pOnEnter,
                pOnExit
            );
        }

        Checkbox* UIManager::createCheckbox(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const Layout* pButtonLayout,
            const Layout* pButtonTextLayout,
            const std::string& text,
            const Font* pFont
        )
        {
            Checkbox* pCheckbox = new Checkbox(
                *this,
                pParent,
                pLayout,
                pTextLayout,
                pButtonLayout,
                pButtonTextLayout,
                text,
                pFont
            );

            if (!pParent)
            {
                pCheckbox->updateTree();
                addRootElement(pCheckbox);
            }
            return pCheckbox;
        }

        Checkbox* UIManager::createCheckbox(
            UIElement* pParent,
            const std::string& text,
            const Font* pFont
        )
        {
            return createCheckbox(
                pParent,
                _pDefaultCheckboxLayout,
                _pDefaultTextLayout,
                _pDefaultCheckboxBoxLayout,
                _pDefaultButtonTextLayout,
                text,
                pFont
            );
        }

        InputField* UIManager::createInputField(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const Layout* pFieldLayout,
            const Layout* pFieldTextLayout,
            const Layout* pCursorIndicatorLayout,
            const std::string& infoText,
            const Font* pFont,
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        )
        {
            InputField* pInputField = new InputField(
                *this,
                pParent,
                pLayout,
                pTextLayout,
                pFieldLayout,
                pFieldTextLayout,
                pCursorIndicatorLayout,
                infoText,
                pFont,
                pOnInputCharFunc,
                pOnInputCharUserData
            );

            if (!pParent)
            {
                pInputField->updateTree();
                addRootElement(pInputField);
            }
            return pInputField;
        }

        InputField* UIManager::createInputField(
            UIElement* pParent,
            const std::string& infoText,
            const Font* pFont,
            ExpandElements fieldDirection, // is the field to the left or below the info txt
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        )
        {
            Layout* pRootLayout = fieldDirection == ExpandElements::DOWN ? _pDefaultInputFieldRootLayout : _pDefaultHorizontalInputFieldRootLayout;
            Layout* pFieldLayout = fieldDirection == ExpandElements::DOWN ? _pDefaultInputFieldLayout : _pDefaultHorizontalInputFieldLayout;

            return createInputField(
                pParent,
                pRootLayout,
                _pDefaultTextLayout,
                pFieldLayout,
                _pDefaultNonStretchTextLayout,
                _pDefaultInputFieldCursorLayout,
                infoText,
                pFont,
                pOnInputCharFunc,
                pOnInputCharUserData
            );
        }

        float UIManager::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
