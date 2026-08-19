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
                _defaultInputFieldWidth,
                &_defaultInputFieldRootLayouts[DefaultInputFieldType::MEDIUM_VERTICAL],
                &_defaultInputFieldLayouts[DefaultInputFieldType::MEDIUM_VERTICAL]
            );
            create_default_input_field_layout(
                *this,
                pDefaultFont,
                ExpandElements::RIGHT,
                _defaultInputFieldWidth,
                &_defaultInputFieldRootLayouts[DefaultInputFieldType::MEDIUM_HORIZONTAL],
                &_defaultInputFieldLayouts[DefaultInputFieldType::MEDIUM_HORIZONTAL]
            );

            create_default_input_field_layout(
                *this,
                pDefaultFont,
                ExpandElements::DOWN,
                _defaultSmallInputFieldWidth,
                &_defaultInputFieldRootLayouts[DefaultInputFieldType::SMALL_VERTICAL],
                &_defaultInputFieldLayouts[DefaultInputFieldType::SMALL_VERTICAL]
            );
            create_default_input_field_layout(
                *this,
                pDefaultFont,
                ExpandElements::RIGHT,
                _defaultSmallInputFieldWidth,
                &_defaultInputFieldRootLayouts[DefaultInputFieldType::SMALL_HORIZONTAL],
                &_defaultInputFieldLayouts[DefaultInputFieldType::SMALL_HORIZONTAL]
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
            UUID_t textureID,
            void(*pOnClick)(MouseButtonName, InputAction, void*),
            void* pOnClickUserData
        )
        {
            UIElement* pElement = new UIElement(
                *this,
                pParent,
                pLayout,
                createRenderable,
                textureID,
                nullptr, // font
                pOnClick,
                pOnClickUserData
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
            const std::string& txt,
            const Font* pFont
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
            const std::string& txt,
            const Font* pFont
        )
        {
            return createText(
                pParent,
                _pDefaultTextLayout,
                txt,
                pFont
            );
        }

        Button* UIManager::createButton(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const std::string& text,
            const Font* pFont,
            void(*pOnClick)(MouseButtonName, InputAction, void*),
            void* pOnClickUserData,
            void(*pOnMouseEnter)(int mx, int my, void* pUserData),
            void* pOnMouseEnterUserData,
            void(*pOnMouseExit)(int mx, int my, void* pUserData),
            void* pOnMouseExitUserData
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
                pOnClickUserData,
                pOnMouseEnter,
                pOnMouseEnterUserData,
                pOnMouseExit,
                pOnMouseExitUserData
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
            void(*pOnClick)(MouseButtonName, InputAction, void*),
            void* pOnClickUserData,
            void(*pOnMouseEnter)(int mx, int my, void* pUserData),
            void* pOnMouseEnterUserData,
            void(*pOnMouseExit)(int mx, int my, void* pUserData),
            void* pOnMouseExitUserData
        )
        {
            return createButton(
                pParent,
                _pDefaultButtonLayout,
                _pDefaultButtonTextLayout,
                text,
                pFont,
                pOnClick,
                pOnClickUserData,
                pOnMouseEnter,
                pOnMouseEnterUserData,
                pOnMouseExit,
                pOnMouseExitUserData
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
            void(*pOnFinishInput)(const std::string&, void*),
            void* pOnFinishInputUserData,
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
                pOnFinishInput,
                pOnFinishInputUserData,
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

        InputField* UIManager::createDefaultInputField(
            UIElement* pParent,
            const std::string& infoText,
            const Font* pFont,
            DefaultInputFieldType type,
            void(*pOnFinishInput)(const std::string&, void*),
            void* pOnFinishInputUserData,
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        )
        {
            const Layout* pRootLayout = getDefaultInputFieldRootLayout(type);
            const Layout* pFieldLayout = getDefaultInputFieldLayout(type);

            return createInputField(
                pParent,
                pRootLayout,
                _pDefaultTextLayout,
                pFieldLayout,
                _pDefaultNonStretchTextLayout,
                _pDefaultInputFieldCursorLayout,
                infoText,
                pFont,
                pOnFinishInput,
                pOnFinishInputUserData,
                pOnInputCharFunc,
                pOnInputCharUserData
            );
        }

        const Layout* UIManager::getDefaultInputFieldRootLayout(DefaultInputFieldType type) const
        {
            std::unordered_map<DefaultInputFieldType, Layout*>::const_iterator it = _defaultInputFieldRootLayouts.find(type);
            if (it == _defaultInputFieldRootLayouts.end())
            {
                Debug::log(
                    "Invalid DefaulInputFieldType: " + std::to_string(static_cast<uint32_t>(type)),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return nullptr;
            }
            return it->second;
        }

        const Layout* UIManager::getDefaultInputFieldLayout(DefaultInputFieldType type) const
        {
            std::unordered_map<DefaultInputFieldType, Layout*>::const_iterator it = _defaultInputFieldLayouts.find(type);
            if (it == _defaultInputFieldLayouts.end())
            {
                Debug::log(
                    "Invalid DefaulInputFieldType: " + std::to_string(static_cast<uint32_t>(type)),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return nullptr;
            }
            return it->second;
        }

        float UIManager::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
