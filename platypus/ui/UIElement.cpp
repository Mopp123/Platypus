#include "UIElement.h"
#include "LayoutUI.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    namespace ui
    {
        void UIElement::ElementCursorPosEvent::func(int x, int y)
        {
            GUITransform* pTransform = (GUITransform*)_pScene->getComponent(
                _pElement->_entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            if (pTransform)
            {
                float fx = (float)x;
                float fy = (float)y;
                if (fx >= pTransform->position.x && fx <= pTransform->position.x + pTransform->scale.x &&
                    fy >= pTransform->position.y && fy <= pTransform->position.y + pTransform->scale.y)
                {
                    if (!_pElement->_isMouseOver)
                    {
                        if (_pElement->_pMouseEnterEvent)
                            _pElement->_pMouseEnterEvent->func(x, y);
                    }

                    if (_pElement->_pMouseOverEvent)
                        _pElement->_pMouseOverEvent->func(x, y);

                    _pElement->_isMouseOver = true;
                }
                else
                {
                    if (_pElement->_isMouseOver)
                    {
                        if (_pElement->_pMouseExitEvent)
                            _pElement->_pMouseExitEvent->func(x, y);
                    }
                    _pElement->_isMouseOver = false;
                }
            }
        }


        void UIElement::ElementMouseButtonEvent::func(MouseButtonName button, InputAction action, int mods)
        {
            if (_pElement->_isMouseOver)
            {
                UIElement::OnClickEvent* pOnClickEvent = _pElement->_pOnClickEvent;
                if (pOnClickEvent)
                    pOnClickEvent->func(button, action);
            }
        }


        UIElement::UIElement(
            entityID_t entityID,
            Layout layout,
            const Font* pFont
        ) :
            _entityID(entityID),
            _layout(layout),
            _pFont(pFont)
        {
            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();
            InputManager& inputManager = pApp->getInputManager();
            inputManager.addCursorPosEvent(
                new ElementCursorPosEvent(
                    pScene,
                    this
                )
            );
            inputManager.addMouseButtonEvent(
                new ElementMouseButtonEvent(this)
            );
        }

        UIElement::~UIElement()
        {
            if (_pMouseEnterEvent)
                delete _pMouseEnterEvent;
            if (_pMouseOverEvent)
                delete _pMouseOverEvent;
            if (_pMouseExitEvent)
                delete _pMouseExitEvent;
            if (_pOnClickEvent)
                delete _pOnClickEvent;
        }

        void UIElement::addChild(
            UIElement* pChild,
            const Vector2f& childPosition,
            const Vector2f& childScale
        )
        {
            _children.push_back(pChild);
            _previousItemPosition = childPosition;
            _previousItemScale = childScale;
        }

        void UIElement::setScale(const Vector2f& scale)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUITransform* pTransform = (GUITransform*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            if (pTransform)
            {
                pTransform->scale = scale;
            }
            _layout.scale = scale;
        }


        UIElement* add_container(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            bool createRenderable,
            ID_t textureID,
            const Font* pFont
        )
        {
            Vector2f scale = layout.scale;

            const Layout* pParentLayout = pParent != nullptr ? &pParent->getLayout() : nullptr;
            entityID_t parentEntityID = pParent != nullptr ? pParent->getEntityID() : NULL_ENTITY_ID;
            Vector2f previousItemPosition = pParent != nullptr ? pParent->_previousItemPosition : Vector2f(0, 0);
            Vector2f previousItemScale = pParent != nullptr ? pParent->_previousItemScale : Vector2f(0, 0);
            int childIndex = pParent != nullptr ? pParent->getChildren().size() : 0;

            Vector2f position = ui.calcPosition(
                layout,
                pParentLayout,
                parentEntityID,
                scale,
                previousItemPosition,
                previousItemScale,
                childIndex
            );

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            entityID_t entity = pScene->createEntity();
            GUITransform* pTransform = create_gui_transform(
                entity,
                position,
                scale
            );

            if (createRenderable)
            {
                GUIRenderable* pRenderable = create_gui_renderable(
                    entity,
                    layout.color
                );
                pRenderable->textureID = textureID;
                pRenderable->borderColor = layout.borderColor;
                pRenderable->borderThickness = static_cast<float>(layout.borderThickness);
            }

            UIElement* pElement = new UIElement(
                entity,
                layout,
                pFont
            );

            if (pParent)
                pParent->addChild(pElement, position, scale);

            ui.addElement(pElement, pParent == nullptr);

            return pElement;
        }
    }
}
