#include "UIElement.hpp"
#include "LayoutUI.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include <cmath>
#include <limits>


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

        void UIElement::addChild(
            UIElement* pChild,
            const Vector2f& childPosition,
            const Vector2f& childScale
        )
        {
            _children.push_back(pChild);
            updateFromChild(pChild, childPosition, childScale);
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

        Vector2f UIElement::calc_position(
            const Layout& layout,
            const Layout* pParentLayout,
            entityID_t parentEntity,
            const Vector2f& scale,
            const Vector2f& previousItemPosition,
            const Vector2f& previousItemScale,
            bool isFirstChild
        )
        {
            Vector2f position;

            Application* pApp = Application::get_instance();
            Window& window = pApp->getWindow();
            Vector2f parentScale(
                static_cast<float>(window.getWidth()),
                static_cast<float>(window.getHeight())
            );
            Vector2f parentPosition(0.0f, 0.0f);
            Vector2f padding = pParentLayout != nullptr ? pParentLayout->padding : Vector2f(0.0f, 0.0f);
            float elementGap = pParentLayout != nullptr ? pParentLayout->elementGap : 0.0f;

            HorizontalAlignment horizontalAlignment = pParentLayout != nullptr ? pParentLayout->horizontalContentAlignment : layout.horizontalAlignment;
            VerticalAlignment verticalAlignment = pParentLayout != nullptr ? pParentLayout->verticalContentAlignment : layout.verticalAlignment;
            ExpandElements expandElements = pParentLayout != nullptr ? pParentLayout->expandElements : ExpandElements::DOWN;

            if (pParentLayout)
            {
                Scene* pScene = pApp->getSceneManager().accessCurrentScene();
                GUITransform* pParentTransform = reinterpret_cast<GUITransform*>(
                    pScene->getComponent(
                        parentEntity,
                        ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
                    )
                );
                parentPosition = pParentTransform->position;
                parentScale = pParentTransform->scale;
            }

            if (horizontalAlignment == HorizontalAlignment::LEFT)
                position.x = parentPosition.x + padding.x + layout.position.x;
            if (horizontalAlignment == HorizontalAlignment::RIGHT)
                position.x = parentPosition.x + parentScale.x - padding.x - scale.x - layout.position.x;
            if (horizontalAlignment == HorizontalAlignment::CENTER)
                position.x = parentPosition.x + parentScale.x * 0.5f - scale.x * 0.5f + layout.position.x;

            if (verticalAlignment == VerticalAlignment::TOP)
                position.y = parentPosition.y + padding.y + layout.position.y;
            if (verticalAlignment == VerticalAlignment::BOTTOM)
                position.y = parentPosition.y + parentScale.y - padding.y - scale.y - layout.position.y;
            if (verticalAlignment == VerticalAlignment::CENTER)
                position.y = parentPosition.y + parentScale.y * 0.5f - scale.y * 0.5f + layout.position.y;

            // Advance position if not the first child
            if (!isFirstChild)
            {
                if (expandElements == ExpandElements::RIGHT)
                    position.x = previousItemPosition.x + previousItemScale.x + elementGap + layout.position.x;
                else if (expandElements == ExpandElements::DOWN)
                    position.y = previousItemPosition.y + previousItemScale.y + elementGap + layout.position.y;
            }

            // Round to integer so don't get weird looking lines...
            return { std::round(position.x), std::round(position.y) };
        }

        void UIElement::updatePosition(
            const UIElement* pParent,
            int32_t childIndex
        )
        {
            const Layout* pParentLayout = pParent != nullptr ? &pParent->getLayout() : nullptr;
            entityID_t parentEntityID = pParent != nullptr ? pParent->getEntityID() : NULL_ENTITY_ID;

            Vector2f prevItemPos;
            Vector2f prevItemScale;
            int32_t prevIndex = childIndex - 1;
            if (prevIndex >= 0)
            {
                prevItemPos = pParent->_children[prevIndex]->getGlobalPosition();
                prevItemScale = pParent->_children[prevIndex]->getGlobalScale();
            }
            else if (pParent)
            {
                prevItemPos = pParent->getGlobalPosition();
                prevItemScale = pParentLayout->padding;
            }

            Vector2f position = calc_position(
                _layout,
                pParentLayout,
                parentEntityID,
                getGlobalScale(),
                prevItemPos,
                prevItemScale,
                childIndex == 0 //pParent != nullptr ? pParent->getChildren().size() : 0
            );
            setGlobalPosition(position);

            for (size_t i = 0; i < _children.size(); ++i)
                _children[i]->updatePosition(this, i);
        }

        void UIElement::getChildBounds(
            float& minX, float& maxX,
            float& minY, float& maxY
        )
        {
            for (UIElement* pChild : _children)
            {
                Vector2f childPos = pChild->getGlobalPosition();
                Vector2f childScale = pChild->getGlobalScale();

                float childLeft = childPos.x;
                float childRight = childPos.x + childScale.x;

                float childTop = childPos.y;
                float childBottom = childPos.y + childScale.y;

                minX = std::min(minX, childLeft);
                minY = std::min(minY, childTop);
                maxX = std::max(maxX, childRight);
                maxY = std::max(maxY, childBottom);

                pChild->getChildBounds(minX, maxX, minY, maxY);
            }
        }

        static float s_testMaxY = 0;
        void UIElement::updateStretching(bool& repositionRequired)
        {
            if (_layout.stretchFitContentFlags > 0)
            {
                GUITransform* pTransform = getTransform();
                Vector2f& posRef = pTransform->position;
                Vector2f& scaleRef = pTransform->scale;
                const Vector2f prevPos = posRef;
                const Vector2f prevScale = scaleRef;

                float minX = posRef.x;
                float minY = posRef.y;
                float maxX = posRef.x + scaleRef.x; // std::numeric_limits<float>::min();
                float maxY = posRef.y + scaleRef.y; // std::numeric_limits<float>::min();
                getChildBounds(minX, maxX, minY, maxY);

                s_testMaxY = std::max(s_testMaxY, maxY);

                // if minX or minY goes negative shit gets fucked
                minX = std::max(minX, 0.0f);
                minY = std::max(minY, 0.0f);
                if (_layout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_HORIZONTALLY)
                {
                    if (minX < posRef.x)
                    {
                        // NOTE: You'd get here by assigning negative values to some child element's
                        // layout's position!
                        //
                        // Why this is an issue:
                        //  *We'd need to change the element's position here. Then we'd need to
                        //  prevent the updatePosition(...) to change this position on the "next
                        //  round's" position updates. In addition to this we'd need to change the
                        //  child element's layout's position to be correctly in relation to the
                        //  parent's new position(that was updated here) so it's non negative,
                        //  because the child's new position is calculated using its' layout's
                        //  position. Otherwise the child and parent chases each others endlessly and
                        //  other fuckery...
                        Debug::log(
                            "Child element's position on X-axis(" + std::to_string(minX) + ") "
                            "was less than the parent element's(" + std::to_string(posRef.x) + "). "
                            "This isn't allowed for horizontally stretching elements!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        //posRef.x = minX;
                    }

                    if ((minX + (maxX - minX)) > (posRef.x + scaleRef.x))
                        scaleRef.x = maxX - minX + _layout.padding.x;
                }
                if (_layout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_VERTICALLY)
                {
                    if (minY < posRef.y)
                    {
                        // *See notes in above, horizontal case's same issue.
                        Debug::log(
                            "Child element's position on Y-axis(" + std::to_string(minY) + ") "
                            "was less than the parent element's(" + std::to_string(posRef.y) + "). "
                            "This isn't allowed for vertically stretching elements!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        posRef.y = minY;
                    }

                    if ((minY + (maxY - minY)) > (posRef.y + scaleRef.y))
                        scaleRef.y = maxY - minY + _layout.padding.y;
                }

                // If stretching occurs, need to reposition the whole tree
                if (prevPos != posRef || prevScale != scaleRef)
                    repositionRequired = true;
            }

            for (UIElement* pChild : _children)
                pChild->updateStretching(repositionRequired);
        }

        // Updates the whole UIElement tree.
        // Does rescaling if stretching is enabled.
        //  -> in such case repositions all elements also accordingly.
        // NOTE: Pretty inefficient and awful, but will do for now
        void UIElement::updateTree(
            const UIElement* pParent,
            int32_t childIndex
        )
        {
            while (true)
            {
                s_testMaxY = 0.0f;
                // Need to update all positions before potential stretchings
                // since the element positions affects the stretching
                updatePosition(pParent, childIndex);
                bool repositionRequired = false;
                updateStretching(repositionRequired);
                if (!repositionRequired)
                    break;
            }

        }

        void UIElement::setScale(const Vector2f& scale)
        {
            GUITransform* pTransform = getTransform();
            pTransform->scale = scale;
        }

        Vector2f UIElement::getGlobalScale() const
        {
            const GUITransform* pTransform = getTransform();
            return pTransform->scale;
        }

        void UIElement::setGlobalPosition(const Vector2f& position)
        {
            GUITransform* pTransform = getTransform();
            pTransform->position = position;
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

        void UIElement::setActive(bool arg)
        {
            // *if setting inactive by OnClick func, reset mouseOver
            _isMouseOver = false;
            for (UIElement* pChild : _children)
                pChild->setActive(arg);

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            pScene->setEntityActive(_entityID, arg);
        }

        uint32_t UIElement::getLayer()
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pRenderable = reinterpret_cast<GUIRenderable*>(
                pScene->getComponent(
                    _entityID,
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
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
            Vector2f scale = layout.scale;

            const Layout* pParentLayout = pParent != nullptr ? &pParent->getLayout() : nullptr;
            entityID_t parentEntityID = pParent != nullptr ? pParent->getEntityID() : NULL_ENTITY_ID;
            Vector2f previousItemPosition = pParent != nullptr ? pParent->_previousItemPosition : Vector2f(0, 0);
            Vector2f previousItemScale = pParent != nullptr ? pParent->_previousItemScale : Vector2f(0, 0);

            // When adding child container, the pParent's child count is the index
            // since we haven't added this new child yet
            int childIndex = pParent != nullptr ? pParent->getChildren().size() : 0;

            Vector2f position = UIElement::calc_position(
                layout,
                pParentLayout,
                parentEntityID,
                scale,
                previousItemPosition,
                previousItemScale,
                childIndex == 0
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
                ui,
                entity,
                layout,
                pFont,
                pOnClickEvent
            );

            if (pParent)
                pParent->addChild(pElement, position, scale);

            if (!pParent)
                ui.addRootElement(pElement);

            return pElement;
        }

        /*
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
        */
    }
}
