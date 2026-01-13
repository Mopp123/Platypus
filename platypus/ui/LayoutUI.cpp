#include "LayoutUI.hpp"
#include "platypus/ecs/components/Transform.h"
#include "platypus/core/Application.hpp"


namespace platypus
{
    namespace ui
    {
        void LayoutUI::ResizeEvent::func(int w, int h)
        {
            _uiRef._windowWidth = (float)w;
            _uiRef._windowHeight = (float)h;
            for (const UIElement* pRootElement : _uiRef._rootElements)
            {
                _uiRef.update(
                    pRootElement,
                    nullptr
                );
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
            for (UIElement* pElement : _elements)
                delete pElement;
        }

        void LayoutUI::update(
            const UIElement* pElement,
            const UIElement* pParentElement,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            const Layout* pParentLayout = pParentElement != nullptr ? &pParentElement->_layout : nullptr;
            entityID_t parentEntity = pParentElement != nullptr ? pParentElement->_entityID : NULL_ENTITY_ID;
            Vector2f position = calcPosition(
                pElement->_layout,
                pParentLayout,
                parentEntity,
                pElement->_layout.scale,
                previousItemPosition,
                previousItemScale,
                childIndex
            );

            if (pElement->_entityID != NULL_ENTITY_ID)
            {
                GUITransform* pTransform = (GUITransform*)_pScene->getComponent(
                    pElement->_entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                // TODO: Make scaling possible?
                pTransform->position = position;
            }

            Vector2f usePrevItemPos;
            Vector2f usePrevItemScale;
            for (size_t i = 0; i < pElement->_children.size(); ++i)
            {
                const UIElement* pChildElement = pElement->_children[i];
                update(
                    pChildElement,
                    pElement,
                    usePrevItemPos,
                    usePrevItemScale,
                    i
                );

                GUITransform* pChildTransform = (GUITransform*)_pScene->getComponent(
                    pChildElement->_entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                usePrevItemPos = pChildTransform->position;
                usePrevItemScale = pChildTransform->scale;
            }
        }

        Vector2f LayoutUI::calcPosition(
            const Layout& layout,
            const Layout* pParentLayout,
            entityID_t parentEntity,
            const Vector2f& scale,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            Vector2f position;

            Vector2f parentScale((float)_windowWidth, (float)_windowHeight);
            Vector2f parentPosition(0.0f, 0.0f);
            Vector2f padding = pParentLayout != nullptr ? pParentLayout->padding : Vector2f(0.0f, 0.0f);
            float elementGap = pParentLayout != nullptr ? pParentLayout->elementGap : 0.0f;

            HorizontalAlignment horizontalAlignment = pParentLayout != nullptr ? pParentLayout->horizontalContentAlignment : layout.horizontalAlignment;
            VerticalAlignment verticalAlignment = pParentLayout != nullptr ? pParentLayout->verticalContentAlignment : layout.verticalAlignment;
            ExpandElements expandElements = pParentLayout != nullptr ? pParentLayout->expandElements : ExpandElements::DOWN;

            if (pParentLayout)
            {
                GUITransform* pParentTransform = (GUITransform*)_pScene->getComponent(
                    parentEntity,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                parentPosition = pParentTransform->position;
                parentScale = pParentTransform->scale;
            }

            if (horizontalAlignment == HorizontalAlignment::LEFT)
                position.x = parentPosition.x + padding.x + layout.position.x;
            if (horizontalAlignment == HorizontalAlignment::RIGHT)
                position.x = parentPosition.x + parentScale.x - padding.x - scale.x - layout.position.x;
            if (horizontalAlignment == HorizontalAlignment::CENTER)
                position.x = parentPosition.x + parentScale.x * 0.5f - scale.x * 0.5f;

            if (verticalAlignment == VerticalAlignment::TOP)
                position.y = parentPosition.y + padding.y + layout.position.y;
            if (verticalAlignment == VerticalAlignment::BOTTOM)
                position.y = parentPosition.y + parentScale.y - padding.y - scale.y - layout.position.y;
            if (verticalAlignment == VerticalAlignment::CENTER)
                position.y = parentPosition.y + parentScale.y * 0.5f - scale.y * 0.5f;

            if (childIndex != 0)
            {
                if (expandElements == ExpandElements::RIGHT)
                    position.x = previousItemPosition.x + previousItemScale.x + elementGap + layout.position.x;
                else if (expandElements == ExpandElements::DOWN)
                    position.y = previousItemPosition.y + previousItemScale.y + elementGap + layout.position.y;
            }

            return position;
        }

        void LayoutUI::addElement(UIElement* pElement, bool isRoot)
        {
            _elements.push_back(pElement);
            if (isRoot)
                _rootElements.push_back(pElement);
        }

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
