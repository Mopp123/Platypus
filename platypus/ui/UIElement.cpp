#include "UIElement.hpp"
#include "LayoutUI.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include <cmath>


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
                    UIElement::s_mouseOverLayers.insert(static_cast<uint32_t>(_pElement->getLayer()));
                    if (UIElement::mouse_over_layer() > _pElement->getLayer())
                    {
                        // *if it already was mouse over, but picked higher layer
                        //  -> mouse exit
                        if (_pElement->_isMouseOver)
                        {
                            if (_pElement->_pMouseExitEvent)
                                _pElement->_pMouseExitEvent->func(x, y);
                        }
                        _pElement->_isMouseOver = false;
                        return;
                    }

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
                    UIElement::s_mouseOverLayers.erase(static_cast<uint32_t>(_pElement->getLayer()));

                    if (UIElement::mouse_over_layer() > _pElement->getLayer())
                        return;

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


        std::set<uint32_t> UIElement::s_mouseOverLayers;
        UIElement::UIElement(
            LayoutUI& ui,
            entityID_t entityID,
            Layout layout,
            const Font* pFont,
            OnClickEvent* pOnClickEvent
        ) :
            _uiRef(ui),
            _pOnClickEvent(pOnClickEvent),
            _entityID(entityID),
            _layout(layout),
            _pFont(pFont)
        {
            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();
            InputManager& inputManager = pApp->getInputManager();
            _pCursorPosEvent = new ElementCursorPosEvent(pScene, this);
            _pMouseButtonEvent = new ElementMouseButtonEvent(pScene, this);
            inputManager.addCursorPosEvent(_pCursorPosEvent);
            inputManager.addMouseButtonEvent(_pMouseButtonEvent);
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

            Application* pApp = Application::get_instance();
            InputManager& inputManager = pApp->getInputManager();
            if (_pMouseButtonEvent)
                inputManager.destroyMouseButtonEvent(_pMouseButtonEvent);
            if (_pCursorPosEvent)
                inputManager.destroyCursorPosEvent(_pCursorPosEvent);

            for (UIElement* pChild : _children)
                delete pChild;

            Scene* pScene = pApp->getSceneManager().accessCurrentScene();
            pScene->destroyEntity(_entityID);
        }

        // NOTE: Via current tree updating system, could get rid of this?
        void UIElement::addChild(
            UIElement* pChild,
            const Vector2f& childPosition,
            const Vector2f& childScale
        )
        {
            pChild->_pParent = this;
            _children.push_back(pChild);
        }

        void UIElement::destroyChildren()
        {
            for (UIElement* pChild : _children)
                delete pChild;

            _children.clear();
        }

        void UIElement::destroyChild(UIElement* pChild)
        {
            int32_t eraseIndex = -1;
            for (size_t i = 0; i < _children.size(); ++i)
            {
                if (_children[i] == pChild)
                {
                    _children[i]->destroyChildren();
                    delete _children[i];
                    eraseIndex = static_cast<size_t>(i);
                    break;
                }
            }
            if (eraseIndex >= 0)
            {
                _children.erase(_children.begin() + static_cast<size_t>(eraseIndex));
            }
            else
            {
                Debug::log(
                    "Child UIElement not found!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        /*
        void UIElement::setScale(const Vector2f& scale)
        {
            GUITransform* pTransform = getTransform();
            pTransform->scale = scale;
        }
        */

        void UIElement::setLayoutScale(Vector2f scale)
        {
            _layout.scale = scale;
            _uiRef.addToUpdatedElements(this);
            _updatePending = true;
        }

        Vector2f UIElement::getGlobalScale() const
        {
            const GUITransform* pTransform = getTransform();
            return pTransform->scale;
        }

        /*
        void UIElement::setGlobalPosition(const Vector2f& position)
        {
            GUITransform* pTransform = getTransform();
            pTransform->position = position;
        }
        */
        void UIElement::setLayoutPosition(const Vector2f& position)
        {
            _layout.position = position;
            _uiRef.addToUpdatedElements(this);
            _updatePending = true;
        }

        Vector2f UIElement::getGlobalPosition() const
        {
            const GUITransform* pTransform = getTransform();
            return pTransform->position;
        }

        GUITransform* UIElement::getTransform()
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUITransform* pTransform = reinterpret_cast<GUITransform*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                )
            );
            if (!pTransform)
            {
                Debug::log(
                    "UIElement's(entityID: " + std::to_string(_entityID) + ") "
                    "GUITransform component was nullptr! "
                    "All UIElements are required to have GUITransform component!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            return pTransform;
        }

        // *Fucking dumb, need the const version of above...
        const GUITransform* UIElement::getTransform() const
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            const GUITransform* pTransform = reinterpret_cast<const GUITransform*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                )
            );
            if (!pTransform)
            {
                Debug::log(
                    "UIElement's(entityID: " + std::to_string(_entityID) + ") "
                    "GUITransform component was nullptr! "
                    "All UIElements are required to have GUITransform component!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            return pTransform;
        }

        GUIRenderable* UIElement::getRenderable()
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pRenderable = reinterpret_cast<GUIRenderable*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pRenderable)
            {
                return pRenderable;
            }
            return nullptr;
        }

        UIElement* UIElement::getRootParent()
        {
            if (!_pParent)
                return this;
            else
                return _pParent->getRootParent();
        }

        void UIElement::setActive(bool arg)
        {
            // *if setting inactive by OnClick func, reset mouseOver
            _isMouseOver = false;
            for (UIElement* pChild : _children)
                pChild->setActive(arg);

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            pScene->setEntityActive(_entityID, arg);
        }

        bool UIElement::isActive()
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            return pScene->isEntityActive(_entityID);
        }

        uint32_t UIElement::getLayer()
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pRenderable = reinterpret_cast<GUIRenderable*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE,
                    false,
                    false
                )
            );
            if (pRenderable)
                return static_cast<int32_t>(pRenderable->layer);

            return 0;
        }

        void UIElement::setLayer(uint32_t layer)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pRenderable = reinterpret_cast<GUIRenderable*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pRenderable)
                pRenderable->layer = layer;
        }

        void UIElement::updateScale()
        {
            if (_children.empty())
            {
                GUITransform* pTransform = getTransform();
                pTransform->scale = _layout.scale;
                return;
            }

            Vector2f scale;
            size_t childIndex = 0;
            for (UIElement* pChild : _children)
            {
                pChild->updateScale();
                uint32_t childEffectOnParent = pChild->_layout.effectOnParentFlags;
                Vector2f childScale = pChild->getGlobalScale();

                if (_layout.expandElements == ExpandElements::DOWN)
                {
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_HORIZONTALLY)
                    {
                        if (childScale.x > scale.x - _layout.padding.x * 2.0f)
                            scale.x = childScale.x + _layout.padding.x * 2.0f;
                    }

                    // NOTE: Shouldn't we take the padding into account here too!?!?
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_VERTICALLY)
                    {
                        scale.y += childScale.y;
                        if (childIndex == 0)
                            scale.y += _layout.padding.y * 2.0f;

                        if (childIndex < _children.size() - 1)
                            scale.y += _layout.elementGap;
                    }
                }
                else if (_layout.expandElements == ExpandElements::RIGHT)
                {
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_VERTICALLY)
                    {
                        if (childScale.y > scale.y - _layout.padding.y * 2.0f)
                            scale.y = childScale.y + _layout.padding.y * 2.0f;
                    }

                    // NOTE: Shouldn't we take the padding into account here too!?!?
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_HORIZONTALLY)
                    {
                        scale.x += childScale.x;
                        if (childIndex == 0)
                            scale.x += _layout.padding.x * 2.0f;

                        if (childIndex < _children.size() - 1)
                            scale.x += _layout.elementGap;
                    }
                }
                ++childIndex;
            }

            GUITransform* pTransform = getTransform();
            pTransform->scale = {
                std::max(_layout.scale.x, scale.x),
                std::max(_layout.scale.y, scale.y)
            };
        }


        static Vector2f position_in_relation_to(
            Vector2f pos,
            HorizontalAlignment horizontalAlignment,
            VerticalAlignment verticalAlignment,
            float left, float right,
            float top, float bottom
        )
        {
            Vector2f result;
            if (horizontalAlignment == HorizontalAlignment::LEFT)
                result.x = left + pos.x;
            if (horizontalAlignment == HorizontalAlignment::RIGHT)
                result.x = right - pos.x;

            if (verticalAlignment == VerticalAlignment::TOP)
                result.y = top + pos.y;
            if (verticalAlignment == VerticalAlignment::BOTTOM)
                result.y = bottom - pos.y;

            return result;
        }

        void UIElement::updatePosition(
            size_t childIndex,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale
        )
        {
            Vector2f padding;
            HorizontalAlignment horizontalAlignment = _layout.horizontalAlignment;
            VerticalAlignment verticalAlignment = _layout.verticalAlignment;
            float elementGap = 0.0f;
            ExpandElements expandElements = ExpandElements::DOWN;
            Vector2f parentPosition;
            Vector2f parentScale;
            Vector2f scale = getGlobalScale();
            Vector2f position;
            if (_pParent)
            {
                const Layout& parentLayout = _pParent->_layout;
                padding = parentLayout.padding;
                horizontalAlignment = parentLayout.horizontalContentAlignment;
                verticalAlignment = parentLayout.verticalContentAlignment;
                elementGap = parentLayout.elementGap;
                expandElements = parentLayout.expandElements;
                GUITransform* pParentTransform = _pParent->getTransform();
                parentPosition = pParentTransform->position;
                parentScale = pParentTransform->scale;
            }
            else
            {
                Application* pApp = Application::get_instance();
                Window& window = pApp->getWindow();
                parentScale.x = static_cast<float>(window.getWidth());
                parentScale.y = static_cast<float>(window.getHeight());
            }

            if (childIndex == 0)
            {
                if (horizontalAlignment == HorizontalAlignment::LEFT)
                    position.x = parentPosition.x + padding.x + _layout.position.x;

                if (horizontalAlignment == HorizontalAlignment::RIGHT)
                    position.x = parentPosition.x + parentScale.x - padding.x - scale.x - _layout.position.x;
                if (horizontalAlignment == HorizontalAlignment::CENTER)
                    position.x = parentPosition.x + parentScale.x * 0.5f - scale.x * 0.5f + _layout.position.x;

                if (verticalAlignment == VerticalAlignment::TOP)
                    position.y = parentPosition.y + padding.y + _layout.position.y;
                if (verticalAlignment == VerticalAlignment::BOTTOM)
                    position.y = parentPosition.y + parentScale.y - padding.y - scale.y - _layout.position.y;
                if (verticalAlignment == VerticalAlignment::CENTER)
                    position.y = parentPosition.y + parentScale.y * 0.5f - scale.y * 0.5f + _layout.position.y;
            }
            else
            {
                if (expandElements == ExpandElements::DOWN)
                {
                    position.y += previousItemPosition.y + previousItemScale.y + elementGap + _layout.position.y;
                    position.x = previousItemPosition.x + _layout.position.x;
                }
                else if (expandElements == ExpandElements::RIGHT)
                {
                    position.x += previousItemPosition.x + previousItemScale.x + elementGap + _layout.position.x;
                    position.y = previousItemPosition.y + _layout.position.y;
                }
            }

            // Round to integer so don't get weird looking lines...
            GUITransform* pTransform = getTransform();
            pTransform->position = { std::round(position.x), std::round(position.y) };
            //setGlobalPosition({ std::round(position.x), std::round(position.y) });

            Vector2f prevItemPosition;
            Vector2f prevItemScale;
            for (size_t i = 0; i < _children.size(); ++i)
            {
                _children[i]->updatePosition(i, prevItemPosition, prevItemScale);
                prevItemPosition = _children[i]->getGlobalPosition();
                prevItemScale = _children[i]->getGlobalScale();
            }
            _updatePending = false;
        }

        void UIElement::updateTree()
        {
            updateScale();
            updatePosition(0, { }, { });
        }

        uint32_t UIElement::mouse_over_layer()
        {
            if (s_mouseOverLayers.empty())
                return 0;

            return *s_mouseOverLayers.rbegin();
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
            Layout useLayout = layout;
            useLayout.padding.x += useLayout.borderThickness;
            useLayout.padding.y += useLayout.borderThickness;
            Vector2f scale = useLayout.scale;


            const Layout* pParentLayout = pParent != nullptr ? &pParent->getLayout() : nullptr;
            entityID_t parentEntityID = pParent != nullptr ? pParent->getEntityID() : NULL_ENTITY_ID;
            Vector2f previousItemPosition = pParent != nullptr ? pParent->_previousItemPosition : Vector2f(0, 0);
            Vector2f previousItemScale = pParent != nullptr ? pParent->_previousItemScale : Vector2f(0, 0);

            // When adding child container, the pParent's child count is the index
            // since we haven't added this new child yet
            int childIndex = pParent != nullptr ? pParent->getChildren().size() : 0;

            Vector2f position;

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
                    useLayout.color
                );
                pRenderable->textureID = textureID;
                pRenderable->borderColor = useLayout.borderColor;
                pRenderable->borderThickness = static_cast<float>(useLayout.borderThickness);
            }

            UIElement* pElement = new UIElement(
                ui,
                entity,
                useLayout,
                pFont,
                pOnClickEvent
            );

            if (pParent)
            {
                pParent->addChild(pElement, position, scale);
                UIElement* pRootParent = pElement->getRootParent();
                pRootParent->updateTree();
            }

            if (!pParent)
                ui.addRootElement(pElement);

            return pElement;
        }
    }
}
