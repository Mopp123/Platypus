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
                uint32_t elementAbsoluteLayer = _pElement->getAbsoluteLayer();
                if (fx >= pTransform->position.x && fx <= pTransform->position.x + pTransform->scale.x &&
                    fy >= pTransform->position.y && fy <= pTransform->position.y + pTransform->scale.y)
                {
                    //Debug::log(
                    //    "___TEST___cursor over element: " + std::to_string(elementAbsoluteLayer) + " "
                    //    "stored cursor over layer: " + std::to_string(UIElement::get_cursor_over_layer()) + " "
                    //    "picked layers: " + std::to_string(UIElement::s_cursorOverLayers.size())
                    //);
                    s_cursorOverLayers[elementAbsoluteLayer].insert(_pElement->getEntityID());
                    //if (UIElement::get_cursor_over_layer() < elementAbsoluteLayer)
                    //{
                    //    // *if it already was mouse over, but picked higher layer
                    //    //  -> mouse exit
                    //    if (_pElement->_isCursorOver)
                    //    {
                    //        remove_from_cursor_over_layers(elementAbsoluteLayer);
                    //        if (_pElement->_pMouseExitEvent)
                    //            _pElement->_pMouseExitEvent->func(x, y);
                    //    }
                    //    _pElement->_isCursorOver = false;
                    //    return;
                    //}

                    bool isHighestPickedLayer = elementAbsoluteLayer == get_cursor_over_layer();
                    if (isHighestPickedLayer)
                    {
                        if (!_pElement->_isCursorOver)
                        {
                            //add_to_cursor_over_layers(elementAbsoluteLayer);
                            if (_pElement->_pMouseEnterEvent)
                                _pElement->_pMouseEnterEvent->func(x, y);
                        }

                        if (_pElement->_pMouseOverEvent)
                            _pElement->_pMouseOverEvent->func(x, y);

                        _pElement->_isCursorOver = true;
                    }
                    else if (get_cursor_over_layer() > elementAbsoluteLayer)
                    {
                        _pElement->_isCursorOver = false;
                    }
                }
                else
                {
                    if (_pElement->_isCursorOver)
                    {
                        //remove_from_cursor_over_layers(elementAbsoluteLayer);
                        if (_pElement->_pMouseExitEvent)
                            _pElement->_pMouseExitEvent->func(x, y);
                    }
                    _pElement->_isCursorOver = false;

                    if (s_cursorOverLayers.find(elementAbsoluteLayer) != s_cursorOverLayers.end())
                    {
                        std::set<entityID_t>& layerEntities = s_cursorOverLayers[elementAbsoluteLayer];
                        if (layerEntities.find(_pElement->getEntityID()) != layerEntities.end())
                        {
                            s_cursorOverLayers[elementAbsoluteLayer].erase(_pElement->getEntityID());
                            if (s_cursorOverLayers[elementAbsoluteLayer].empty())
                            {
                                Debug::log("___TEST___erased cursor over layer: " + std::to_string(elementAbsoluteLayer));
                                s_cursorOverLayers.erase(elementAbsoluteLayer);
                            }
                        }
                    }
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

            if (_pElement->_isCursorOver)
            {
                UIElement::OnClickEvent* pOnClickEvent = _pElement->_pOnClickEvent;
                if (pOnClickEvent)
                    pOnClickEvent->func(button, action);

                Debug::log(
                    "___TEST___clicked elem layer: " + std::to_string(_pElement->getAbsoluteLayer()) + " "
                    "stored cursor over layer: " + std::to_string(get_cursor_over_layer())
                );

                _pElement->_dragged = true;
            }

            if (action == InputAction::RELEASE)
                _pElement->_dragged = false;
        }


        std::map<uint32_t, std::set<entityID_t>> UIElement::s_cursorOverLayers;
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

        // *Also updates the whole tree so the scales and positions are
        // correct immediately.
        // TODO: get rid of the mergeLayers here
        void UIElement::addChild(UIElement* pChild, bool mergeLayers)
        {
            pChild->_pParent = this;
            _children.push_back(pChild);

            UIElement* pRootParent = getRootParent();

            #ifdef PLATYPUS_DEBUG
            if (!pRootParent)
            {
                Debug::log(
                    "No root parent found for UIElement with entityID: " + std::to_string(getEntityID()),
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            pRootParent->updateTree();
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
            // if cursor was over -> remove from those
            if (_isCursorOver)
            {
                Debug::log("___TEST___removed from cursor over layers");
                remove_from_cursor_over_layers(getAbsoluteLayer(), _entityID);
            }

            // *if setting inactive by OnClick func, reset mouseOver
            _isCursorOver = false;
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

        void UIElement::fetchAbsoluteTreeLayers(std::set<uint32_t>& outLayers)
        {
            outLayers.insert(getAbsoluteLayer());
            for (UIElement* pChild : _children)
            {
                pChild->fetchAbsoluteTreeLayers(outLayers);
            }
        }

        void UIElement::fetchAbsoluteTreeLayers(std::map<uint32_t, size_t>& outLayers)
        {
            outLayers[getAbsoluteLayer()] += 1;
            for (UIElement* pChild : _children)
            {
                pChild->fetchAbsoluteTreeLayers(outLayers);
            }
        }

        uint32_t UIElement::getTopTreeLayer()
        {
            std::set<uint32_t> allLayers;
            fetchAbsoluteTreeLayers(allLayers);
            if (!allLayers.empty())
                return *allLayers.rbegin();

            return 0;
        }

        void UIElement::setTreeLayer(uint32_t layer)
        {
            GUIRenderable* pRenderable = getRenderable();
            if (pRenderable)
                pRenderable->layer = layer;

            _layout.layer = layer;

            for (UIElement* pChild : _children)
                pChild->setTreeLayer(layer);
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

        void UIElement::updatePosition(
            size_t childIndex,
            Vector2f& cumulatedScale,
            bool mergeLayerToParent
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

                _absoluteLayer = _pParent->getAbsoluteLayer() + _layout.layer;
            }
            else
            {
                Application* pApp = Application::get_instance();
                Window& window = pApp->getWindow();
                parentScale.x = static_cast<float>(window.getWidth());
                parentScale.y = static_cast<float>(window.getHeight());

                _absoluteLayer = _layout.layer;
            }
            setRenderLayer(_absoluteLayer);

            // Get the "origin" pos in relation to parent
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

            // Add the cumulated elements scale so it goes correctly after the previous element
            if (expandElements == ExpandElements::DOWN)
            {
                position.y += (cumulatedScale.y);
                cumulatedScale.y = cumulatedScale.y + scale.y + elementGap;
                if (scale.x > cumulatedScale.x)
                    cumulatedScale.x = scale.x + padding.x;
            }
            else if (expandElements == ExpandElements::RIGHT)
            {
                position.x += (cumulatedScale.x);
                cumulatedScale.x = cumulatedScale.x + scale.x + elementGap;
                if (scale.y > cumulatedScale.y)
                    cumulatedScale.y = scale.y + padding.y;
            }

            // Round to integer so don't get weird looking lines...
            GUITransform* pTransform = getTransform();
            pTransform->position = { std::round(position.x), std::round(position.y) };

            // *scale cumulation is always for the immediate children (not for any deeper level!)
            Vector2f childrenCumulatedScale;
            for (size_t i = 0; i < _children.size(); ++i)
                _children[i]->updatePosition(i, childrenCumulatedScale);

            _updatePending = false;
        }

        void UIElement::updateTree(bool mergeLayerToParent)
        {
            updateScale();
            Vector2f cumulatedScale;
            updatePosition(0, cumulatedScale, mergeLayerToParent);
        }

        void UIElement::fetchTreeElements(std::vector<UIElement*>& outElements)
        {
            outElements.push_back(this);
            for (UIElement* pChild : _children)
                pChild->fetchTreeElements(outElements);
        }

        bool UIElement::isCursorOverTree() const
        {
            if (_isCursorOver)
                return true;

            for (const UIElement* pChild : _children)
            {
                if (pChild->isCursorOverTree())
                    return true;
            }

            return false;
        }

        void UIElement::setRelativeLayer(uint32_t relativeLayer)
        {
            _layout.layer = relativeLayer;
            _updatePending = true;
        }

        uint32_t UIElement::get_cursor_over_layer()
        {
            if (s_cursorOverLayers.empty())
                return 0;

            return s_cursorOverLayers.rbegin()->first;
        }


        void UIElement::setRenderLayer(uint32_t renderLayer)
        {
            // TODO: Maybe have ptr to scene when creating the element so don't need to get
            // every time again?
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
                pRenderable->layer = renderLayer;
        }

        void UIElement::add_to_cursor_over_layers(uint32_t absoluteLayer, entityID_t entityID)
        {
            // Not sure if it's slower to first search if found, since inserting doesn't
            // insert if already exists...
            s_cursorOverLayers[absoluteLayer].insert(entityID);
        }

        void UIElement::remove_from_cursor_over_layers(uint32_t absoluteLayer, entityID_t entityID)
        {
            std::map<uint32_t, std::set<entityID_t>>::iterator it = s_cursorOverLayers.find(absoluteLayer);
            if (it != s_cursorOverLayers.end())
            {
                std::set<entityID_t>& layerEntities = it->second;
                if (it->second.find(entityID) != layerEntities.end())
                {
                    layerEntities.erase(entityID);

                    if (layerEntities.empty())
                        s_cursorOverLayers.erase(absoluteLayer);
                }
            }
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

            if (useLayout.wordWrap == WordWrap::NORMAL && useLayout.scale.x == 0.0f)
            {
                Debug::log(
                    "Layout was using word wrapping but its' scale was 0. "
                    "Can't wrap against width of 0...",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

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
                pRenderable->layer = useLayout.layer;
            }

            UIElement* pElement = new UIElement(
                ui,
                entity,
                useLayout,
                pFont,
                pOnClickEvent
            );

            // *Need to update the "tree" even if contains only single element
            // so that scale and pos is immediately correct..
            if (pParent)
            {
                pParent->addChild(pElement);
            }
            else
            {
                pElement->updateTree();
                if (!pParent)
                    ui.addRootElement(pElement);
            }

            return pElement;
        }
    }
}
