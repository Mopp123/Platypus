#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/core/Scene.h"
#include "UIElement.h"
#include <vector>
#include <string>

/*
    Issues with prev UI systems:
        *That fucked up constraint thing
        *Unable to specify "containers'" layout for it's elements / child components


    NEXT UP:
        *Make it possible for "container" to scale to fit all it's elements automatically

    TODO:
        *Create the full "layout composition", immediate mode style
            -> have some "pool" of components to use for rendering
                -> render the layout composition

        *System where I can just say, give me some container, specify how it should look,
        specify way it's added elements will be layed out, how things should scale

        *Container can have Elements and other Containers whose layout depends on their parent container
*/

namespace platypus
{
    namespace ui
    {
        class LayoutUI
        {
        public:
            struct Config
            {
                Vector4f buttonColor = Vector4f(0.4f, 0.4f, 0.4f, 1.0f);
                Vector4f buttonHighlightColor = Vector4f(0.55f, 0.55f, 0.55f, 1.0f);
                Vector4f buttonTextColor = Vector4f(1, 1, 1, 1);
                Vector4f buttonTextHighlightColor = Vector4f(0.1f, 0.1f, 0.1f, 1);
            };

            static Config s_config;

        private:
            class ResizeEvent : public WindowResizeEvent
            {
            public:
                LayoutUI& _uiRef;
                ResizeEvent(LayoutUI& uiRef) : _uiRef(uiRef) {}
                virtual void func(int w, int h);
            };

            Scene* _pScene = nullptr;

            int _windowWidth = 0;
            int _windowHeight = 0;

            std::vector<UIElement*> _elements;
            std::vector<UIElement*> _rootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager);
            ~LayoutUI();

            void update(
                const UIElement* pElement,
                const UIElement* pParentElement,
                const Vector2f& previousItemPosition = Vector2f(0, 0),
                const Vector2f& previousItemScale = Vector2f(0, 0),
                int childIndex = 0
            );

            Vector2f calcPosition(
                const Layout& layout,
                const Layout* pParentLayout,
                entityID_t parentEntity,
                const Vector2f& scale,
                const Vector2f& previousItemPosition,
                const Vector2f& previousItemScale,
                int childIndex = 0
            );

            void addElement(UIElement* pElement, bool isRoot);

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
