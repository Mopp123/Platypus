#include "LayoutUI.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    namespace ui
    {
        void LayoutUI::ResizeEvent::func(int w, int h)
        {
            _uiRef._windowWidth = (float)w;
            _uiRef._windowHeight = (float)h;
            for (UIElement* pRootElement : _uiRef._rootElements)
                pRootElement->updateTree();
        }

        void LayoutUI::init(Scene* pScene, InputManager& inputManager)
        {
            _pScene = pScene;
            inputManager.addWindowResizeEvent(new ResizeEvent(*this));

            Window& window = Application::get_instance()->getWindow();
            window.getSurfaceExtent(&_windowWidth, &_windowHeight);
        }

        LayoutUI::~LayoutUI()
        {
            for (UIElement* pElement : _rootElements)
                delete pElement;
        }

        Layout* LayoutUI::createLayout()
        {
            _layouts.push_back({});
            return _layouts[_layouts.size() - 1];
        }

        void LayoutUI::addRootElement(UIElement* pElement)
        {
            _rootElements.push_back(pElement);
        }

        void LayoutUI::removeRootElement(UIElement* pElement)
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

        bool LayoutUI::isRootElement(UIElement* pElement) const
        {
            for (UIElement* pRootElement : _rootElements)
            {
                if (pElement == pRootElement)
                    return true;
            }
            return false;
        }

        void LayoutUI::addToUpdatedElements(UIElement* pElement)
        {
            UIElement* pElementRootParent = pElement->getRootParent();
            if (_updatedRootElements.find(pElementRootParent) != _updatedRootElements.end())
                return;

            _updatedRootElements.insert(pElementRootParent);
        }

        void LayoutUI::updateChangedElements()
        {
            for (UIElement* pUpdatedRootElement : _updatedRootElements)
                pUpdatedRootElement->updateTree();

            _updatedRootElements.clear();
        }

        void LayoutUI::addLayout(Layout& layout)
        {
            if (layout.id != -1)
            {
                Debug::log(
                    "layout's id " + std::to_string(layout.id) + " was already set. "
                    "You might have already added this layout to the LayoutUI container.",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            layout.id = static_cast<int32_t>(_layouts.size());
            _layouts.push_back(layout);
        }

        Layout& LayoutUI::getLayout(int32_t id)
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

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
