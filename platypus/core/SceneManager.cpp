#include "SceneManager.hpp"
#include "Application.hpp"
#include "Debug.hpp"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    SceneManager::~SceneManager()
    {
        cleanUp();
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


        Application* pApp = Application::get_instance();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        // Reset Batcher for next round of submits!
        // NOTE: This was earlier done in MasterRenderer
        // before _pRenderer3D->advanceFrame();!
        pMasterRenderer->getBatcher().resetForNextFrame();

        // Submit all "renderable components" for rendering.
        // NOTE: This has to be done here since need quarantee that all necessary components have been
        // properly updated before submission!
        for (const Entity& entity : _pCurrentScene->_entities)
        {
            if (entity.id != NULL_ENTITY_ID && entity.active)
                pMasterRenderer->submit(_pCurrentScene, entity);
        }

        // NOTE: Need some way to specify if u really want to do this
        // or if u want to do this for some batches but not for some
        // others!
        //  -> BECAUSE:
        //      1. It's slow to destroy and allocate new batches!
        //      2. u might want for some batches to be ready which
        //      aren't used atm!
        //
        // *For debug and editor purposes this might be fine to be
        // done always..?
        #ifdef PLATYPUS_DEBUG
        pMasterRenderer->getBatcher().pruneEmptyBatches();
        #endif
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

            // NOTE: Important that the scene gets destroyed here, since
            // it might destroy some resources explicitly by itself and
            // the input and asset managers then cleans the rest!
            delete _pCurrentScene;

            pApp->getInputManager().destroyEvents();
            pApp->getAssetManager()->destroyAssets();

            _pCurrentScene = _pNextScene;
            _pCurrentScene->init();

            _pNextScene = nullptr;
        }
    }

    void SceneManager::cleanUp()
    {
        if (_pCurrentScene)
            delete _pCurrentScene;
        if (_pNextScene)
            delete _pNextScene;

        _pCurrentScene = nullptr;
        _pNextScene = nullptr;
    }
}
