#include "UIManager.hpp"
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

        void UIManager::init(Scene* pScene, InputManager& inputManager)
        {
            _pScene = pScene;
            inputManager.addWindowResizeEvent(new ResizeEvent(*this));

            Window& window = Application::get_instance()->getWindow();
            window.getSurfaceExtent(&_windowWidth, &_windowHeight);
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
                pLayout,
                createRenderable,
                textureID,
                pOnClickEvent
            );
            // *Need to update the "tree" even if contains only single element
            // so that scale and pos is immediately correct..
            if (pParent)
            {
                pParent->addChild(pElement);
            }
            else
            {
                pElement->updateTree();
                addRootElement(pElement);
            }
            return pElement;
        }

        Text* UIManager::createText(
            UIElement* pParent,
            const Font* pFont,
            const Layout::Colors& colors,
            const std::string& txt,
            uint32_t effectOnParentFlags
        )
        {
            Text* pText = new Text(
                *this,
                pParent,
                pFont,
                colors,
                txt,
                effectOnParentFlags
            );

            if (pParent)
            {
                pParent->addChild(pText);
            }
            else
            {
                pText->updateTree();
                addRootElement(pText);
            }
            return pText;
        }

        Button* UIManager::createButton(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout::Colors& textColors,
            uint32_t textEffectOnParentFlags,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            Button* pButton = new Button(
                *this,
                pLayout,
                textColors,
                textEffectOnParentFlags,
                text,
                pFont,
                pOnClick,
                pOnEnter,
                pOnExit
            );

            if (pParent)
            {
                pParent->addChild(pButton);
            }
            else
            {
                pButton->updateTree();
                addRootElement(pButton);
            }
            return pButton;
        }

        Checkbox* UIManager::createCheckbox(
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pButtonLayout,
            const Layout::Colors& textColors,
            const std::string& text,
            const Font* pFont
        )
        {
            Checkbox* pCheckbox = new Checkbox(
                *this,
                pLayout,
                pButtonLayout,
                textColors,
                text,
                pFont
            );

            if (pParent)
            {
                pParent->addChild(pCheckbox);
            }
            else
            {
                pCheckbox->updateTree();
                addRootElement(pCheckbox);
            }
            return pCheckbox;
        }

        InputField* UIManager::createInputField(
            UIElement* pParent,
            const Layout* pRootLayout,
            const Layout* pFieldLayout,
            const Layout::Colors& textColors,
            const std::string& infoText,
            const Font* pFont,
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        )
        {
            InputField* pInputField = new InputField(
                *this,
                pRootLayout,
                pFieldLayout,
                textColors,
                infoText,
                pFont,
                pOnInputCharFunc,
                pOnInputCharUserData
            );

            if (pParent)
            {
                pParent->addChild(pInputField);
            }
            else
            {
                pInputField->updateTree();
                addRootElement(pInputField);
            }
            return pInputField;
        }

        float UIManager::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
