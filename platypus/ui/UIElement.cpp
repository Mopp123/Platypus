#include "UIElement.hpp"
#include "LayoutUI.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    namespace ui
    {
        void UIElement::ElementCursorPosEvent::func(int x, int y)
        {
            if (!_pScene->isEntityActive(_pElement->_entityID))
                return;

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

            if (_pElement->_pDragEvent && _pElement->_dragged)
                _pElement->_pDragEvent->func(x, y);
        }


        void UIElement::ElementMouseButtonEvent::func(
            MouseButtonName button,
            InputAction action,
            int mods
        )
        {
            if (!_pScene->isEntityActive(_pElement->_entityID))
                return;

            if (_pElement->_isMouseOver)
            {
                UIElement::OnClickEvent* pOnClickEvent = _pElement->_pOnClickEvent;
                if (pOnClickEvent)
                    pOnClickEvent->func(button, action);

                _pElement->_dragged = true;
            }

            if (action == InputAction::RELEASE)
                _pElement->_dragged = false;
        }


        UIElement::UIElement(
            entityID_t entityID,
            Layout layout,
            const Font* pFont,
            OnClickEvent* pOnClickEvent
        ) :
            _pOnClickEvent(pOnClickEvent),
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
                new ElementMouseButtonEvent(pScene, this)
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
            if (_pDragEvent)
                delete _pDragEvent;
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
            updateFromChild(pChild, childPosition, childScale);
        }

        void UIElement::updateFromChild(
            UIElement* pChild,
            const Vector2f& childPosition,
            const Vector2f& childScale
        )
        {
            _previousItemPosition = childPosition;
            _previousItemScale = childScale;

            Vector2f newScale = _layout.scale;
            Vector2f globalPosition = getGlobalPosition();
            if (_layout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_HORIZONTALLY)
            {
                float childRight = childPosition.x + childScale.x;
                float ownRight = globalPosition.x + _layout.scale.x - _layout.padding.x;
                if (childRight > ownRight)
                {
                    float stretchX = childRight - ownRight;
                    newScale.x = _layout.scale.x + stretchX;
                }
            }
            if (_layout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_VERTICALLY)
            {
                float childBottom = childPosition.y + childScale.y;
                float ownBottom = globalPosition.y + _layout.scale.y - _layout.padding.y;
                if (childBottom > ownBottom)
                {
                    float stretchY = childBottom - ownBottom;
                    newScale.y = _layout.scale.y + stretchY;
                }
            }
            if (newScale != _layout.scale)
                setScale(newScale);
        }

        void UIElement::setScale(const Vector2f& scale)
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
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

        Vector2f UIElement::getGlobalScale() const
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUITransform* pTransform = (GUITransform*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            if (pTransform)
                return pTransform->scale;
            else
                return _layout.scale;
        }

        void UIElement::setGlobalPosition(const Vector2f& position)
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUITransform* pTransform = (GUITransform*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            // NOTE: Not actually sure what to do if there's no Transform
            // since the layout's position is a relative position and not global
            if (pTransform)
                pTransform->position = position;
            else
                _layout.position = position;
        }

        Vector2f UIElement::getGlobalPosition() const
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUITransform* pTransform = (GUITransform*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
            );
            if (pTransform)
            {
                return pTransform->position;
            }
            return { };
        }

        GUIRenderable* UIElement::getRenderable()
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pRenderable = (GUIRenderable*)pScene->getComponent(
                _entityID,
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            if (pRenderable)
            {
                return pRenderable;
            }
            return nullptr;
        }

        void UIElement::setActive(bool arg)
        {
            for (UIElement* pChild : _children)
                pChild->setActive(arg);

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            pScene->setEntityActive(_entityID, arg);
        }


        UIElement* add_container(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            bool createRenderable,
            ID_t textureID,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClickEvent
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
                pFont,
                pOnClickEvent
            );

            if (pParent)
                pParent->addChild(pElement, position, scale);

            ui.addElement(pElement, pParent == nullptr);

            return pElement;
        }

        void update_containers(
            LayoutUI& ui,
            UIElement* pParent,
            UIElement* pElement,
            const Layout& layout
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
            GUITransform* pTransform = reinterpret_cast<GUITransform*>(
                    pScene->getComponent(
                    pElement->getEntityID(),
                    ComponentType::COMPONENT_TYPE_TRANSFORM
                )
            );
            pTransform->position = position;
            pTransform->scale = scale;

            if (pParent)
                pParent->updateFromChild(pElement, position, scale);

            // TODO:
            // 1. Update new roots for the LayoutUI here!
            // 2. Do the LayoutUI's update here... or something like that?
        }
    }
}
