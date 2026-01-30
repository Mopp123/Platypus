#include "LoadingScreen.hpp"

using namespace platypus;


LoadingScreen::LoadingScreen()
{
}

LoadingScreen::~LoadingScreen()
{
}

void LoadingScreen::init()
{
    initBase();

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
    Font* pFont = pAssetManager->loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    InputManager& inputManager = Application::get_instance()->getInputManager();
    _ui.init(this, inputManager);

    ui::Layout layout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.2f, 0.2f, 0.2f, 0.8f } // color
    };
    layout.borderColor = { 1, 1, 1, 1 };
    layout.borderThickness = 2;
    layout.horizontalAlignment = ui::HorizontalAlignment::CENTER;
    layout.verticalAlignment = ui::VerticalAlignment::CENTER;

    ui::UIElement* pBoxElement = ui::add_container(_ui, nullptr, layout, true);
    Vector4f textColor(1, 1, 1, 1);
    ui::UIElement* pTextElement = ui::add_text_element(
        _ui,
        pBoxElement,
        "Loading...",
        textColor,
        pFont
    );
}

void LoadingScreen::update()
{
    updateBase();
}
