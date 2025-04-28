#include "LayoutUI.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    namespace ui
    {

        void LayoutUI::ResizeEvent::func(int w, int h)
        {
            for (size_t rootElementIndex : _uiRef._rootElements)
            {
                UIElement& rootElement = _uiRef._elements[rootElementIndex];
                if (rootElement.layout.fullscreen)
                {
                    Vector2f newScale((float)w, (float)h);
                    rootElement.layout.scale = newScale;
                    ((GUITransform*)_uiRef._pScene->getComponent(
                        rootElement.entityID,
                        ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                    ))->scale = newScale;
                }

                _uiRef.update(
                    rootElement,
                    nullptr,
                    NULL_ENTITY_ID
                );
            }
        }

        void LayoutUI::init(Scene* pScene, InputManager& inputManager)
        {
            _pScene = pScene;
            inputManager.addWindowResizeEvent(new ResizeEvent(*this));

            Window& window = Application::get_instance()->getWindow();
            window.getSurfaceExtent(&_windowWidth, &_windowHeight);
        }

        size_t LayoutUI::create(
            const Layout& layout,
            const Layout* pParentLayout,
            entityID_t parentEntity,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            Vector2f scale = layout.scale;
            if (layout.fullscreen)
                scale = { _windowWidth, _windowHeight };

            Vector2f position = calcPosition(
                layout,
                pParentLayout,
                parentEntity,
                scale,
                previousItemPosition,
                previousItemScale,
                childIndex
            );

            // Just temporarely create rect to display
            entityID_t entity = _pScene->createEntity();
            GUITransform* pTransform = create_gui_transform(
                entity,
                position,
                scale
            );

            GUIRenderable* pBoxRenderable = create_gui_renderable(
                entity,
                layout.color
            );

            _elements.push_back({ entity, layout });
            size_t elementIndex = _elements.size() - 1;
            if (!pParentLayout)
                _rootElements.push_back(elementIndex);

            Vector2f usePrevItemPos;
            Vector2f usePrevItemScale;
            for (size_t i = 0; i < layout.children.size(); ++i)
            {
                size_t childElementIndex = create(
                    layout.children[i],
                    &layout,
                    entity,
                    usePrevItemPos,
                    usePrevItemScale,
                    i
                );

                GUITransform* pChildTransform = (GUITransform*)_pScene->getComponent(
                    _elements[childElementIndex].entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                usePrevItemPos = pChildTransform->position;
                usePrevItemScale = pChildTransform->scale;
                _elements[elementIndex].childIndices.push_back(childElementIndex);
            }
            return elementIndex;
        }


        void LayoutUI::update(
            const UIElement& element,
            const UIElement* pParentElement,
            entityID_t parentEntity,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            const Layout* pParentLayout = pParentElement != nullptr ? &pParentElement->layout : nullptr;
            Vector2f position = calcPosition(
                element.layout,
                pParentLayout,
                parentEntity,
                element.layout.scale,
                previousItemPosition,
                previousItemScale,
                childIndex
            );

            if (element.entityID != NULL_ENTITY_ID)
            {
                GUITransform* pTransform = (GUITransform*)_pScene->getComponent(
                    element.entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                // TODO: Make scaling possible?
                pTransform->position = position;
            }

            Vector2f usePrevItemPos;
            Vector2f usePrevItemScale;
            for (size_t i = 0; i < element.childIndices.size(); ++i)
            {
                const UIElement& childElement = _elements[element.childIndices[i]];
                update(
                    childElement,
                    &element,
                    element.entityID,
                    usePrevItemPos,
                    usePrevItemScale,
                    i
                );

                GUITransform* pChildTransform = (GUITransform*)_pScene->getComponent(
                    childElement.entityID,
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

            HorizontalAlignment horizontalAlignment = pParentLayout != nullptr ? pParentLayout->horizontalAlignment : HorizontalAlignment::LEFT;
            VerticalAlignment verticalAlignment = pParentLayout != nullptr ? pParentLayout->verticalAlignment : VerticalAlignment::TOP;
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

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
