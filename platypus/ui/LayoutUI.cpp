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
            GUIRenderable* pRenderable = (GUIRenderable*)_pScene->getComponent(
                _pElement->_entityID,
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            if (pTransform && pRenderable)
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
            MouseEnterEvent* pMouseEnterEvent,
            MouseOverEvent* pMouseOverEvent,
            MouseExitEvent* pMouseExitEvent,
            OnClickEvent* pOnClickEvent
        ) :
            _entityID(entityID),
            _layout(layout),
            _pMouseEnterEvent(pMouseEnterEvent),
            _pMouseOverEvent(pMouseOverEvent),
            _pMouseExitEvent(pMouseExitEvent),
            _pOnClickEvent(pOnClickEvent)
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

        LayoutUI::Config s_config;
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

        float LayoutUI::toPercentage(float v1, float v2)
        {
            return (float)((int)(v1 / 100.0f * v2));
        }


        UIElement* LayoutUI::addContainer(
            UIElement* pParent,
            const Layout& layout,
            UIElement::MouseEnterEvent* pMouseEnterEvent,
            UIElement::MouseOverEvent* pMouseOverEvent,
            UIElement::MouseExitEvent* pMouseExitEvent,
            UIElement::OnClickEvent* pOnClickEvent
        )
        {
            Vector2f scale = layout.scale;

            const Layout* pParentLayout = pParent != nullptr ? &pParent->getLayout() : nullptr;
            entityID_t parentEntityID = pParent != nullptr ? pParent->getEntityID() : NULL_ENTITY_ID;
            Vector2f previousItemPosition = pParent != nullptr ? pParent->_previousItemPosition : Vector2f(0, 0);
            Vector2f previousItemScale = pParent != nullptr ? pParent->_previousItemScale : Vector2f(0, 0);
            int childIndex = pParent != nullptr ? pParent->_children.size() : 0;

            Vector2f position = calcPosition(
                layout,
                pParentLayout,
                parentEntityID,
                scale,
                previousItemPosition,
                previousItemScale,
                childIndex
            );

            entityID_t entity = _pScene->createEntity();
            GUITransform* pTransform = create_gui_transform(
                entity,
                position,
                scale
            );

            UIElement* pElement = new UIElement(
                entity,
                layout,
                pMouseEnterEvent,
                pMouseOverEvent,
                pMouseExitEvent,
                pOnClickEvent
            );
            _elements.push_back(pElement);

            if (pParent)
            {
                pParent->_children.push_back(pElement);
                pParent->_previousItemPosition = position;
                pParent->_previousItemScale = scale;
            }
            else
            {
                _rootElements.push_back(pElement);
            }

            return pElement;
        }

        void LayoutUI::createImage(UIElement* pElement, ID_t textureID)
        {
            create_gui_renderable(
                pElement->_entityID,
                pElement->_layout.color
            );
        }

        UIElement* LayoutUI::addTextElement(
            UIElement* pParent,
            const std::wstring& text,
            const Vector4f& color,
            const Font* pFont,
            UIElement::MouseEnterEvent* pMouseEnterEvent,
            UIElement::MouseOverEvent* pMouseOverEvent,
            UIElement::MouseExitEvent* pMouseExitEvent,
            UIElement::OnClickEvent* pOnClickEvent
        )
        {
            Layout layout = pParent->_layout;
            layout.scale = get_text_scale(text, pFont);

            UIElement* pElement = addContainer(
                pParent,
                layout,
                pMouseEnterEvent,
                pMouseOverEvent,
                pMouseExitEvent,
                pOnClickEvent
            );
            GUIRenderable* pTextRenderable = create_gui_renderable(
                pElement->_entityID,
                color
            );
            pTextRenderable->textureID = pFont->getTextureID();
            pTextRenderable->fontID = pFont->getID();
            pTextRenderable->text = text;

            return pElement;
        }

        Vector2f LayoutUI::get_text_scale(const std::wstring& text, const Font* pFont)
        {
            Vector2f scale(0, (float)pFont->getMaxCharHeight());
            for (wchar_t c : text)
            {
                const FontGlyphData * const glyph = pFont->getGlyph(c);
                if (glyph)
                {
                    scale.x += ((float)(glyph->advance >> 6));
                }
            }
            return scale;
        }

        UIElement* LayoutUI::addButtonElement(
            UIElement* pParent,
            const std::wstring& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick
        )
        {
            // TODO: do it...
            return nullptr;
        }
    }
}
