#include "UIElement.hpp"
#include "UIManager.hpp"
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
                    uint32_t currentHighestLayer = get_cursor_over_layer();
                    add_to_cursor_over_layers(elementAbsoluteLayer, _pElement->getEntityID());

                    // *if it already was mouse over, but picked higher layer
                    //  -> mouse exit
                    if (elementAbsoluteLayer < currentHighestLayer)
                    {
                        if (_pElement->_isCursorOver)
                        {
                            remove_from_cursor_over_layers(elementAbsoluteLayer, _pElement->getEntityID());
                            if (_pElement->_pMouseExitEvent)
                                _pElement->_pMouseExitEvent->func(x, y);
                        }
                        _pElement->_isCursorOver = false;
                    }
                    else
                    {
                        if (!_pElement->_isCursorOver)
                        {
                            if (_pElement->_pMouseEnterEvent)
                                _pElement->_pMouseEnterEvent->func(x, y);
                        }

                        if (_pElement->_pMouseOverEvent)
                            _pElement->_pMouseOverEvent->func(x, y);

                        _pElement->_isCursorOver = true;
                    }
                }
                else
                {
                    if (_pElement->_isCursorOver)
                    {
                        if (_pElement->_pMouseExitEvent)
                            _pElement->_pMouseExitEvent->func(x, y);
                    }
                    _pElement->_isCursorOver = false;
                    remove_from_cursor_over_layers(elementAbsoluteLayer, _pElement->getEntityID());
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

                _pElement->_dragged = true;
            }

            if (action == InputAction::RELEASE)
                _pElement->_dragged = false;
        }


        std::map<uint32_t, std::set<entityID_t>> UIElement::s_cursorOverLayers;
        UIElement::UIElement(
            UIManager& uiManager,
            const Layout* pLayout,
            bool createRenderable,
            ID_t textureID,
            OnClickEvent* pOnClickEvent,
            bool ignoreInput
        ) :
            _managerRef(uiManager),
            _pOnClickEvent(pOnClickEvent),
            _layoutID(pLayout->id)
        {
            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();

            // Currently not allowing any mouse input for Text elements.
            // TODO: There should rather be a way to specify this explicitly.
            //  -> for example we'll eventually want to highlight and copy text, etc with mouse...
            if (!ignoreInput)
            {
                InputManager& inputManager = pApp->getInputManager();
                _pCursorPosEvent = new ElementCursorPosEvent(pScene, this);
                _pMouseButtonEvent = new ElementMouseButtonEvent(pScene, this);
                inputManager.addCursorPosEvent(_pCursorPosEvent);
                inputManager.addMouseButtonEvent(_pMouseButtonEvent);
            }

            if (pLayout->id == -1)
            {
                Debug::log(
                    "Layout's id was invalid(-1). "
                    "Make sure you have added the layout to the UIManager container before using it.",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            if (pLayout->wordWrap == WordWrap::NORMAL && pLayout->scale.x == 0.0f)
            {
                Debug::log(
                    "Layout was using word wrapping but its' scale was 0. "
                    "Can't wrap against width of 0...",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            _entityID = pScene->createEntity();
            GUITransform* pTransform = create_gui_transform(
                _entityID,
                { 0, 0 }, // The actual position is eventually figured out by updatePosition()
                pLayout->scale
            );
            if (createRenderable)
            {
                GUIRenderable* pRenderable = create_gui_renderable(
                    _entityID,
                    pLayout->color
                );
                pRenderable->textureID = textureID;
                pRenderable->borderColor = pLayout->borderColor;
                pRenderable->borderThickness = static_cast<float>(pLayout->borderThickness);
                pRenderable->layer = pLayout->layer;
            }

            // *Need to update the "tree" even if contains only single element
            // so that scale and pos is immediately correct..
            // NOTE: Currently done by UIManager when creating UIElement...
            // not sure if that works tho...
            /*
            if (pParent)
            {
                pParent->addChild(this);
            }
            else
            {
                updateTree();
                _managerRef.addRootElement(this);
            }
            */
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
        void UIElement::addChild(UIElement* pChild)
        {
            // If child was previously root -> make not anymore
            if (_managerRef.isRootElement(pChild))
                _managerRef.removeRootElement(pChild);

            pChild->_pParent = this;

            // Want to make the child inactive only in case this is inactive
            //  -> might want to add child elements that are inactive but this being active
            if (!isActive())
                pChild->setActive(false);

            // NOTE: DANGER! WARNING!
            // All child elements will be put one layer above their parent so it is quaranteed
            // that they are rendered above the parent!
            //  -> This results in more draw calls!
            //  -> This may result in more fuck ups in the future!
            //  *This was to progress forward with the shitty editor thing...
            pChild->setRelativeLayer(1);

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

        void UIElement::setLayoutScale(Vector2f scale)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->scale = scale;
            _managerRef.addToUpdatedElements(this);
            _updatePending = true;
        }

        void UIElement::setLayoutPosition(const Vector2f& position)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->position = position;
            _managerRef.addToUpdatedElements(this);
            _updatePending = true;
        }

        void UIElement::setLayoutColor(const Vector4f& color)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->color = color;
            _updatePending = true;
        }

        void UIElement::setLayoutHoverColor(const Vector4f& color)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->hoverColor = color;
            _updatePending = true;
        }

        void UIElement::setLayoutSelectedColor(const Vector4f& color)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->selectedColor = color;
            _updatePending = true;
        }

        Vector2f UIElement::getGlobalScale() const
        {
            const GUITransform* pTransform = getTransform();
            return pTransform->scale;
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
            if (!arg)
                remove_from_cursor_over_layers(getAbsoluteLayer(), _entityID);

            // *if setting inactive by OnClick func, reset mouseOver
            // NOTE: This might be an issue if setting active and cursor immediately over?
            _isCursorOver = false;
            _dragged = false;
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

        // NOTE: Newly incorporated borderThickness might not work properly
        //  -> not fully tested!
        void UIElement::updateScale()
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            if (_children.empty())
            {
                GUITransform* pTransform = getTransform();
                pTransform->scale = pLayout->scale;
                return;
            }

            Vector2f scale;
            size_t childIndex = 0;
            for (UIElement* pChild : _children)
            {
                // NOTE: Not sure if this fucks something up?
                if (!pChild->isActive())
                    continue;

                pChild->updateScale();
                Layout* pChildLayout = _managerRef.getLayout(pChild->_layoutID);
                uint32_t childEffectOnParent = pChildLayout->effectOnParentFlags;
                // If child has no scaling effect on parent at all,
                // continue AND DON'T INCREMENT THE childIndex! -> otherwise fucks up slightly
                // the next child that has effect on parent!
                if (!(childEffectOnParent & (EffectOnParentFlagBits::STRETCH_HORIZONTALLY | EffectOnParentFlagBits::STRETCH_VERTICALLY)))
                    continue;

                Vector2f childScale = pChild->getGlobalScale();
                if (pLayout->expandElements == ExpandElements::DOWN)
                {
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_HORIZONTALLY)
                    {
                        if (childScale.x > scale.x - pLayout->padding.x * 2.0f - pLayout->borderThickness * 2.0f)
                            scale.x = childScale.x + pLayout->padding.x * 2.0f + pLayout->borderThickness * 2.0f;
                    }

                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_VERTICALLY)
                    {
                        scale.y += childScale.y;
                        if (childIndex == 0)
                            scale.y += pLayout->padding.y * 2.0f + pLayout->borderThickness * 2.0f;

                        if (childIndex < _children.size() - 1)
                            scale.y += pLayout->elementGap;
                    }
                }
                else if (pLayout->expandElements == ExpandElements::RIGHT)
                {
                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_VERTICALLY)
                    {
                        if (childScale.y > scale.y - pLayout->padding.y * 2.0f - pLayout->borderThickness * 2.0f)
                            scale.y = childScale.y + pLayout->padding.y * 2.0f + pLayout->borderThickness * 2.0f;
                    }

                    if (childEffectOnParent & EffectOnParentFlagBits::STRETCH_HORIZONTALLY)
                    {
                        scale.x += childScale.x;
                        if (childIndex == 0)
                            scale.x += pLayout->padding.x * 2.0f + pLayout->borderThickness * 2.0f;

                        if (childIndex < _children.size() - 1)
                            scale.x += pLayout->elementGap;
                    }
                }
                ++childIndex;
            }

            GUITransform* pTransform = getTransform();
            pTransform->scale = {
                std::max(pLayout->scale.x, scale.x),
                std::max(pLayout->scale.y, scale.y)
            };
        }

        // NOTE: Newly incorporated borderThickness might not work properly
        //  -> not fully tested!
        void UIElement::updatePosition(Vector2f& cumulatedScale)
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            Vector2f padding;
            float borderThickness = 0.0f;
            HorizontalAlignment horizontalAlignment = pLayout->horizontalAlignment;
            VerticalAlignment verticalAlignment = pLayout->verticalAlignment;
            float elementGap = 0.0f;
            ExpandElements expandElements = ExpandElements::DOWN;
            Vector2f parentPosition;
            Vector2f parentScale;
            Vector2f scale = getGlobalScale();
            Vector2f position;
            if (_pParent)
            {
                const Layout* pParentLayout = _managerRef.getLayout(_pParent->_layoutID);
                padding = pParentLayout->padding;
                horizontalAlignment = pParentLayout->horizontalContentAlignment;
                verticalAlignment = pParentLayout->verticalContentAlignment;
                elementGap = pParentLayout->elementGap;
                expandElements = pParentLayout->expandElements;
                GUITransform* pParentTransform = _pParent->getTransform();
                parentPosition = pParentTransform->position;
                parentScale = pParentTransform->scale;

                _absoluteLayer = _pParent->getAbsoluteLayer() + pLayout->layer;
            }
            else
            {
                Application* pApp = Application::get_instance();
                Window& window = pApp->getWindow();
                parentScale.x = static_cast<float>(window.getWidth());
                parentScale.y = static_cast<float>(window.getHeight());

                _absoluteLayer = pLayout->layer;
            }
            setRenderLayer(_absoluteLayer);

            // Get the "origin" pos in relation to parent
            if (horizontalAlignment == HorizontalAlignment::LEFT)
                position.x = parentPosition.x + padding.x + borderThickness + pLayout->position.x;

            if (horizontalAlignment == HorizontalAlignment::RIGHT)
                position.x = parentPosition.x + parentScale.x - padding.x - borderThickness - scale.x - pLayout->position.x;
            if (horizontalAlignment == HorizontalAlignment::CENTER)
                position.x = parentPosition.x + parentScale.x * 0.5f - scale.x * 0.5f + pLayout->position.x;

            if (verticalAlignment == VerticalAlignment::TOP)
                position.y = parentPosition.y + padding.y + borderThickness + pLayout->position.y;
            if (verticalAlignment == VerticalAlignment::BOTTOM)
                position.y = parentPosition.y + parentScale.y - padding.y - borderThickness - scale.y - pLayout->position.y;
            if (verticalAlignment == VerticalAlignment::CENTER)
                position.y = parentPosition.y + parentScale.y * 0.5f - scale.y * 0.5f + pLayout->position.y;

            // Add the cumulated elements scale so it goes correctly after the previous element
            // UPDATE TO BELOW: Even if the current elem doesn't affect pos, its own positioning
            // should take the cumulated pos/scale into account!
            // TODO: Remove below commented out section...
            /*
            if (pLayout->effectOnParentFlags & EffectOnParentFlagBits::INCREMENT_POSITION)
            {
                if (expandElements == ExpandElements::DOWN)
                {
                    position.y += (cumulatedScale.y);
                    cumulatedScale.y = cumulatedScale.y + scale.y + elementGap;
                    if (scale.x > cumulatedScale.x)
                        cumulatedScale.x = scale.x + padding.x + borderThickness;
                }
                else if (expandElements == ExpandElements::RIGHT)
                {
                    position.x += (cumulatedScale.x);
                    cumulatedScale.x = cumulatedScale.x + scale.x + elementGap;
                    if (scale.y > cumulatedScale.y)
                        cumulatedScale.y = scale.y + padding.y + borderThickness;
                }
            }
            */
            bool incrementPos = pLayout->effectOnParentFlags & EffectOnParentFlagBits::INCREMENT_POSITION;
            if (expandElements == ExpandElements::DOWN)
            {
                position.y += (cumulatedScale.y);
                if (incrementPos)
                {
                    cumulatedScale.y = cumulatedScale.y + scale.y + elementGap;
                    if (scale.x > cumulatedScale.x)
                        cumulatedScale.x = scale.x + padding.x + borderThickness;
                }
            }
            else if (expandElements == ExpandElements::RIGHT)
            {
                position.x += (cumulatedScale.x);
                if (incrementPos)
                {
                    cumulatedScale.x = cumulatedScale.x + scale.x + elementGap;
                    if (scale.y > cumulatedScale.y)
                        cumulatedScale.y = scale.y + padding.y + borderThickness;
                }
            }

            // Round to integer so don't get weird looking lines...
            GUITransform* pTransform = getTransform();
            pTransform->position = { std::round(position.x), std::round(position.y) };

            // *scale cumulation is always for the immediate children (not for any deeper level!)
            Vector2f childrenCumulatedScale;
            for (size_t i = 0; i < _children.size(); ++i)
                _children[i]->updatePosition(childrenCumulatedScale);

            _updatePending = false;
        }

        // NOTE: BORDER THICKNESS IS NO MORE PART OF PADDING!
        void UIElement::updateTree()
        {
            updateScale();
            Vector2f cumulatedScale;
            updatePosition(cumulatedScale);
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
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            pLayout->layer = relativeLayer;
            _updatePending = true;
        }

        uint32_t UIElement::getRelativeLayer() const
        {
            Layout* pLayout = _managerRef.getLayout(_layoutID);
            return pLayout->layer;
        }

        uint32_t UIElement::getAbsoluteLayer() const
        {
            return _absoluteLayer;
        }

        Layout* UIElement::getLayout() const
        {
            return _managerRef.getLayout(_layoutID);
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


        // TODO: remove below
        /*
        UIElement* add_container(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout* pLayout,
            bool createRenderable,
            ID_t textureID,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClickEvent
        )
        {
            if (pLayout->id == -1)
            {
                Debug::log(
                    "Layout's id was invalid(-1). "
                    "Make sure you have added the layout to the LayoutUI container before using it.",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            if (pLayout->wordWrap == WordWrap::NORMAL && pLayout->scale.x == 0.0f)
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
                pLayout->scale
            );
            if (createRenderable)
            {
                GUIRenderable* pRenderable = create_gui_renderable(
                    entity,
                    pLayout->color
                );
                pRenderable->textureID = textureID;
                pRenderable->borderColor = pLayout->borderColor;
                pRenderable->borderThickness = static_cast<float>(pLayout->borderThickness);
                pRenderable->layer = pLayout->layer;
            }

            UIElement* pElement = new UIElement(
                ui,
                entity,
                pLayout->id,
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
        */
    }
}
