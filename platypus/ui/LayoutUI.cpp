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
            for (UIElement& elem : _uiRef._elements)
            {
                GUITransform* pTransform = (GUITransform*)_uiRef._pScene->getComponent(
                    elem.entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );

                Value& positionX = elem.layout.position[0];
                Value& positionY = elem.layout.position[1];
                Value& width = elem.layout.width;
                Value& height = elem.layout.height;
                if (positionX.type == VType::PR)
                    pTransform->position.x = _uiRef.toPercentage((float)w, positionX.value);
                if (positionY.type == VType::PR)
                    pTransform->position.y = _uiRef.toPercentage((float)h, positionY.value);
                if (width.type == VType::PR)
                    pTransform->scale.x = _uiRef.toPercentage((float)w, width.value);
                if (height.type == VType::PR)
                    pTransform->scale.y = _uiRef.toPercentage((float)h, height.value);
            }
        }

        void LayoutUI::init(Scene* pScene, InputManager& inputManager)
        {
            _pScene = pScene;
            inputManager.addWindowResizeEvent(new ResizeEvent(*this));

            Window& window = Application::get_instance()->getWindow();
            window.getSurfaceExtent(&_windowWidth, &_windowHeight);
        }

        UIElement LayoutUI::create(
            const Layout& layout,
            const Layout* pParentLayout,
            entityID_t parentEntity,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            Vector2f parentScale;
            if (pParentLayout && parentEntity != NULL_ENTITY_ID)
            {
                GUITransform* pParentTransform = (GUITransform*)_pScene->getComponent(
                    parentEntity,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                parentScale = pParentTransform->scale;
            }
            else
            {
                if (layout.width.type == VType::PR)
                    parentScale.x = _windowWidth;
                if (layout.height.type == VType::PR)
                    parentScale.y = _windowHeight;
            }

            Vector2f scale = calcScale(layout, parentScale);
            Vector2f position = calcPosition(
                layout,
                pParentLayout,
                parentEntity,
                parentScale,
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

            Vector2f usePrevItemPos;
            Vector2f usePrevItemScale;
            for (size_t i = 0; i < layout.children.size(); ++i)
            {
                UIElement childElement = create(
                    layout.children[i],
                    &layout,
                    entity,
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

            return { entity, layout };
        }


        Vector2f LayoutUI::calcPosition(
            const Layout& layout,
            const Layout* pParentLayout,
            entityID_t parentEntity,
            const Vector2f& parentScale,
            const Vector2f& scale,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            int childIndex
        )
        {
            float positionX = layout.position[0].value;
            float positionY = layout.position[1].value;
            float parentPositionX = 0.0f;
            float parentPositionY = 0.0f;

            float convertedX = positionX;
            float convertedY = positionY;
            if (layout.position[0].type == VType::PR)
                convertedX = toPercentage(parentScale.x, layout.position[0].value);
            if (layout.position[1].type == VType::PR)
                convertedY = toPercentage(parentScale.y, layout.position[1].value);

            if (pParentLayout)
            {
                GUITransform* pParentTransform = (GUITransform*)_pScene->getComponent(
                    parentEntity,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                );
                parentPositionX = pParentTransform->position.x;
                parentPositionY = pParentTransform->position.y;
                float parentWidth = pParentTransform->scale.x;
                float parentHeight = pParentTransform->scale.y;

                const Value& paddingX = pParentLayout->paddingX;
                const Value& paddingY = pParentLayout->paddingY;
                float paddingValX = paddingX.value;
                float paddingValY = paddingY.value;

                if (paddingX.type == VType::PR)
                    paddingValX = toPercentage(parentScale.x, paddingValX);
                if (paddingY.type == VType::PR)
                    paddingValY = toPercentage(parentScale.y, paddingValY);

                if (pParentLayout->horizontalAlignment == HorizontalAlignment::LEFT)
                    positionX = parentPositionX + paddingValX + convertedX;
                if (pParentLayout->horizontalAlignment == HorizontalAlignment::RIGHT)
                    positionX = parentPositionX + parentScale.x - paddingValX - scale.x - convertedX;
                if (pParentLayout->horizontalAlignment == HorizontalAlignment::CENTER)
                    positionX = parentPositionX + parentWidth * 0.5f - scale.x * 0.5f;

                if (pParentLayout->verticalAlignment == VerticalAlignment::TOP)
                    positionY = parentPositionY + paddingValY + convertedY;
                if (pParentLayout->verticalAlignment == VerticalAlignment::BOTTOM)
                    positionY = parentPositionY + parentScale.y - paddingValY - scale.y - convertedY;
                if (pParentLayout->verticalAlignment == VerticalAlignment::CENTER)
                    positionY = parentPositionY + parentHeight * 0.5f - scale.y * 0.5f;

                if (childIndex != 0)
                {
                    float elemGap = calcElementGap(pParentLayout->expandElements, pParentLayout->elementGap);

                    if (pParentLayout->expandElements == ExpandElements::RIGHT)
                    {
                        positionX = previousItemPosition.x + previousItemScale.x + elemGap + convertedX;
                    }
                    else if (pParentLayout->expandElements == ExpandElements::LEFT)
                    {
                        positionX = previousItemPosition.x - elemGap - scale.x + convertedX;
                    }
                    else if (pParentLayout->expandElements == ExpandElements::DOWN)
                    {
                        positionY = previousItemPosition.y + previousItemScale.y + elemGap + convertedY;
                    }
                    else if (pParentLayout->expandElements == ExpandElements::UP)
                    {
                        //positionY -= previousItemScale.y * childIndex - elemGap;
                        positionY = previousItemPosition.y - elemGap - scale.y + convertedY;
                    }
                }
            }

            return { positionX, positionY };
        }

        Vector2f LayoutUI::calcScale(const Layout& layout, const Vector2f& parentScale)
        {
            float width = layout.width.value;
            float height = layout.height.value;
            if (layout.width.type == VType::PR)
                width = toPercentage((float)parentScale.x, layout.width.value);
            if (layout.height.type == VType::PR)
                height = toPercentage((float)parentScale.y, layout.height.value);

            return { width, height };
        }

        float LayoutUI::calcElementGap(ExpandElements expandType, const Value& value)
        {
            float elementGapVal = value.value;
            if (value.type == VType::PR)
            {
                float percentageFrom = _windowWidth;
                if (expandType == ExpandElements::DOWN || expandType == ExpandElements::UP)
                    percentageFrom = _windowHeight;

                elementGapVal = toPercentage(percentageFrom, elementGapVal);
            }
            return elementGapVal;
        }

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
