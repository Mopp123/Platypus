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
                pRootElement->updatePosition(nullptr);
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
            Debug::log("___TEST___Destroying LayoutUI");
            for (UIElement* pElement : _rootElements)
                delete pElement;
        }

        void LayoutUI::addRootElement(UIElement* pElement)
        {
            _rootElements.push_back(pElement);
        }

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
