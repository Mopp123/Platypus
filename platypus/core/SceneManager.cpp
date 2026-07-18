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

        // NOTE: New possible entity errors are generated after this which
        // can be queried by user before this point!
        _pCurrentScene->clearErrors();

        // Submit all "renderable components" for rendering.
        // NOTE: This has to be done here since need quarantee that all necessary components have been
        // properly updated before submission!
        for (const Entity& entity : _pCurrentScene->_entities)
        {
            if (entity.id != NULL_ENTITY_ID && entity.active)
                pMasterRenderer->submit(_pCurrentScene, entity);
        }


        // CONTINUE HERE!
        //PLATYPUS_ASSERT(false);
        // TODO:
        //  Quick error fixes (same way as u did earlier in EditorUI::updateAssetLists):
        //      *If Renderable3D's mesh is unavailable -> use error mesh
        //      *If Renderable3D's material is unavailable -> use error material
        //      *If Renderable3D's material's texture becomes unavailable -> use error texture
        //      *if Renderable3D's mesh and material are incompatible -> use error mesh and material
        std::unordered_map<UUID_t, std::vector<EntityError>>::const_iterator errorIt;
        const std::unordered_map<UUID_t, std::vector<EntityError>>& entityErrors = _pCurrentScene->getErrors();

        const std::unordered_map<EntityErrorType, void(*)(Scene*, EntityError)>& fixMapping = _pCurrentScene->_entityErrorFixMapping;
        for (errorIt = entityErrors.begin(); errorIt != entityErrors.end(); ++errorIt)
        {
            const std::vector<EntityError>& errors = errorIt->second;
            for (const EntityError& error : errors)
            {
                std::unordered_map<EntityErrorType, void(*)(Scene*, EntityError)>::const_iterator fixIt = fixMapping.find(error.type);
                if (fixIt == fixMapping.end())
                {
                    Debug::log(
                        "No fix function found for EntityErrorType: " + entity_error_type_to_string(error.type),
                        PLATYPUS_CURRENT_FUNC_NAME,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
                void(*pFixFunc)(Scene*, EntityError) = fixIt->second;
                (*pFixFunc)(_pCurrentScene, error);
            }
        }

        // NOTE: Need some way to specify if u really want to do this
        // or if u want to do this for some batches but not for some
        // others!
        //  -> BECAUSE:
        //      1. It's slow to destroy and allocate new batches!
        //      2. u might want for some batches to be ready which
        //      aren't used atm(empty)!
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

            pApp->getMasterRenderer()->cleanRenderers();

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
