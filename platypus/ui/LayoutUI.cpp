#include "LayoutUI.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Application.h"


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

        UIElement& LayoutUI::createElement(const Layout& layout)
        {
            entityID_t entity = _pScene->createEntity();

            float positionX = layout.position[0].value;
            float positionY = layout.position[1].value;
            float width = layout.width.value;
            float height = layout.height.value;
            if (layout.position[0].type == VType::PR)
                positionX = toPercentage((float)_windowWidth, layout.position[0].value);
            if (layout.position[1].type == VType::PR)
                positionY = toPercentage((float)_windowHeight, layout.position[1].value);
            if (layout.width.type == VType::PR)
                width = toPercentage((float)_windowWidth, layout.width.value);
            if (layout.height.type == VType::PR)
                height = toPercentage((float)_windowHeight, layout.height.value);


            create_gui_transform(
                entity,
                { positionX, positionY },
                { width, height }
            );
            if (layout.color != NULL_COLOR)
            {
                GUIRenderable* pBoxRenderable = create_gui_renderable(
                    entity,
                    layout.color
                );
            // layer?
            }
            _elements.push_back({entity, layout});
            return _elements.back();
        }

        UIElement& LayoutUI::addChild(UIElement& parent, const Layout& layout)
        {
            entityID_t entity = _pScene->createEntity();

            float positionX = layout.position[0].value;
            float positionY = layout.position[1].value;
            GUITransform* pParentTransform = (GUITransform*)_pScene->getComponent(
                parent.entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            Value paddingX = parent.layout.paddingX;
            Value paddingY = parent.layout.paddingY;

            if (layout.position[0].type == VType::PR)
            {
                positionX = toPercentage(pParentTransform->scale.x, layout.position[0].value);
            }
            if (layout.position[1].type == VType::PR)
                positionY = toPercentage(pParentTransform->scale.y, layout.position[1].value);

            positionX += pParentTransform->position.x + paddingX.value;
            positionY += pParentTransform->position.y + paddingY.value;

            float width = layout.width.value;
            float height = layout.height.value;
            if (layout.width.type == VType::PR)
                width = toPercentage((float)_windowWidth, layout.width.value);
            if (layout.height.type == VType::PR)
                height = toPercentage((float)_windowHeight, layout.height.value);


            create_gui_transform(
                entity,
                { positionX, positionY },
                { width, height }
            );
            if (layout.color != NULL_COLOR)
            {
                GUIRenderable* pBoxRenderable = create_gui_renderable(
                    entity,
                    layout.color
                );
            // layer?
            }
            _elements.push_back({entity, layout});
            return _elements.back();
        }

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }
    }
}
