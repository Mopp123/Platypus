#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/core/Scene.hpp"
#include "UIElement.hpp"
#include <vector>

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
                Vector4f buttonBorderColor = Vector4f(0, 0, 0, 1);
                float buttonBorderThickness = 0.0f;
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

            std::vector<UIElement*> _rootElements;

        public:
            void init(Scene* pScene, InputManager& inputManager);
            ~LayoutUI();

            void addRootElement(UIElement* pElement);

        private:
            float toPercentage(float v1, float v2);
        };
    }
}
