#include "SceneManager.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    SceneManager::~SceneManager()
    {
        if (_pCurrentScene)
            delete _pCurrentScene;
        if (_pNextScene)
            delete _pNextScene;
    }

    // NOTE: This does a bit more than just updates the current scene
    // -> This should rather be Application's job?!
    void SceneManager::update()
    {
        if (!_pCurrentScene)
        {
            Debug::log(
                "@SceneManager::update "
                "Current scene not assigned",
                Debug::MessageType::PLATYPUS_WARNING
            );
            return;
        }

        _pCurrentScene->update();

        // Update all systems of the scene
        // NOTE: Not actually sure should system updates happen befor or after the scene's update?
        for (System* system : _pCurrentScene->_systems)
            system->update(_pCurrentScene);

        // Submit all "renderable components" for rendering.
        // NOTE: This has to be done here since need quarantee that all necessary components have been
        // properly updated before submission!
        Application* pApp = Application::get_instance();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();
        for (const Entity& entity : _pCurrentScene->_entities)
        {
            if (entity.active)
                pMasterRenderer->submit(_pCurrentScene, entity);
        }
    }

    // triggers scene switching at the end of the frame
    void SceneManager::assignNextScene(Scene* newScene)
    {
        _pNextScene = newScene;
    }

    // Detects and handles scene switching
    void SceneManager::handleSceneSwitching()
    {
        if (_pNextScene != nullptr)
        {
            Debug::log("Switching scene");

            Application* pApp = Application::get_instance();
            Device::wait_for_operations();

            Debug::log("___TEST___cleaning renderers...", PLATYPUS_CURRENT_FUNC_NAME);
            pApp->getMasterRenderer()->cleanRenderers();
            Debug::log("___TEST___success!", PLATYPUS_CURRENT_FUNC_NAME);

            pApp->getInputManager().destroyEvents();
            pApp->getAssetManager()->destroyAssets();

            delete _pCurrentScene;
            _pCurrentScene = _pNextScene;
            _pCurrentScene->init();

            _pNextScene = nullptr;
        }
    }
}
