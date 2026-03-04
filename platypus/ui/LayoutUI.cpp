#include "LayoutUI.hpp"
#include "platypus/ecs/components/Transform.hpp"
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
            {
                // TODO: Get rid of the old UIElement::updateTree(...)
                //  -> some fixes might be required for the new way tho...
                pRootElement->updateScale_TEST();
                pRootElement->updatePosition_TEST(0, {}, {});
                //pRootElement->updateTree(nullptr);
            }
        }

        LayoutUI::Config LayoutUI::s_config;
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
        }

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
