#include "LoadingScreen.hpp"

using namespace platypus;


LoadingScreen::LoadingScreen()
{
}

LoadingScreen::~LoadingScreen()
{
}

static std::unordered_map<std::string, std::string>::const_iterator s_loadIt;
void LoadingScreen::init()
{
    initBase();

    AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
    Font* pFont = pAssetManager->loadFont("assets/fonts/Ubuntu-R.ttf", 16);

    InputManager& inputManager = Application::get_instance()->getInputManager();
    _ui.init(this, inputManager);

    ui::Layout boxLayout {
        { 0, 0 }, // pos
        { 400, 200 }, // scale
        { 0.2f, 0.2f, 0.2f, 0.8f } // color
    };
    boxLayout.borderColor = { 1, 1, 1, 1 };
    boxLayout.borderThickness = 2;
    boxLayout.horizontalAlignment = ui::HorizontalAlignment::CENTER;
    boxLayout.verticalAlignment = ui::VerticalAlignment::CENTER;
    boxLayout.padding = { 5, 5 };
    boxLayout.elementGap = 2;
    _pBoxElement = ui::add_container(_ui, nullptr, boxLayout, true);

    Vector4f textColor(1, 1, 1, 1);
    _pStatusTextElement = ui::add_text_element(
        _ui,
        _pBoxElement,
        "Loading...",
        textColor,
        pFont
    );

    Vector4f progressBarColor(1, 1, 1, 1);
    _maxVisualWidth = boxLayout.scale.x - boxLayout.padding.x * 2;
    ui::Layout progressBarLayout {
        { 0, 0 }, // pos
        { 0, _progressBarHeight }, // scale
        progressBarColor
    };
    _pProgressBarElement = ui::add_container(_ui, _pBoxElement, progressBarLayout, true);

    _texturePaths = {
        { "DiffuseTest", "assets/textures/DiffuseTest.png" },
        { "DistortionMap", "assets/textures/DistortionMap.png" },
        { "Floor", "assets/textures/Floor.png" },
        { "Grass", "assets/textures/Grass.png" },
        { "Water", "assets/textures/Water.png" }
    };
    _maxProgress = _texturePaths.size();

    s_loadIt = _texturePaths.begin();
}

static bool s_finished = false;
static float s_cooldown = 0.0f;
static float s_maxCooldown = 1.0f;
static uint32_t s_progress = 0;
void LoadingScreen::update()
{
    updateBase();

    Application* pApp = Application::get_instance();
    AssetManager* pAssetManager = pApp->getAssetManager();
    if (!s_finished && s_cooldown == 0.0f)
    {
        ui::set_text(_pStatusTextElement, _pBoxElement, "Loading: " + s_loadIt->first);
        _textures[s_loadIt->first] = pAssetManager->loadTexture(
            s_loadIt->second,
            ImageFormat::R8G8B8A8_SRGB,
            {
                TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
                TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT,
                true,
                0
            }
        );
        ++s_progress;
        setProgress(s_progress);
        ++s_loadIt;
        if (s_loadIt == _texturePaths.end())
        {
            s_finished = true;
        }
    }

    if (s_cooldown <= s_maxCooldown)
        s_cooldown += Timing::get_delta_time();
    else
        s_cooldown = 0.0f;
}

void LoadingScreen::setProgress(uint32_t progress)
{
    float s = _maxVisualWidth / static_cast<float>(_maxProgress);
    float visualProgress = s * static_cast<float>(progress);
    _pProgressBarElement->setScale({ visualProgress, _progressBarHeight });
}
