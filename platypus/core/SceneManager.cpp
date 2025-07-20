#include "SceneManager.h"
#include "Application.h"
#include "Debug.h"


namespace platypus
{
    /*
    static void submit_renderable(
        Scene* pScene,
        ComponentPool& renderablePool,
        ComponentPool& transformPool,
        const std::vector<Entity>& entities,
        Renderer* pRenderer,
        uint32_t requiredComponentMask,
        ComponentType requiredAdditionalComponent
    )
    {
        void* pCustomData = nullptr;
        size_t customDataSize = 0;
        for (const Entity& e : entities)
        {
            if ((e.componentMask & requiredComponentMask) == requiredComponentMask)
            {
                Component* pRenderable = (Component*)renderablePool[e.id];
                Transform* pTransform = (Transform*)transformPool[e.id];
                if (!pRenderable->isActive())
                    continue;

                if (requiredAdditionalComponent != ComponentType::PK_EMPTY)
                {
                    Component* pAdditionalComponent = pScene->getComponent(
                        e.id,
                        requiredAdditionalComponent
                    );
                    pCustomData = (void*)pAdditionalComponent;
                    customDataSize = sizeof(Component*); // ..ptr size should be same for any ptr
                }

                pRenderer->submit(
                    pRenderable,
                    pTransform->getTransformationMatrix(),
                    pCustomData,
                    customDataSize
                );
            }
        }
    }
    */

    SceneManager::~SceneManager()
    {
        if (_pCurrentScene)
            delete _pCurrentScene;
        if (_pNextScene)
            delete _pNextScene;
    }

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
            pMasterRenderer->submit(_pCurrentScene, entity);

        /*
        ComponentPool& transformPool = _pCurrentScene->componentPools[ComponentType::PK_TRANSFORM];
        std::map<ComponentType, Renderer*>& renderers = masterRenderer.accessRenderers();
        std::map<ComponentType, Renderer*>::iterator rIt;
        const std::vector<Entity>& entities = _pCurrentScene->entities;
        for (rIt = renderers.begin(); rIt != renderers.end(); ++rIt)
        {
            const ComponentType& renderableType = rIt->first;
            // TODO: make this more clever..
            ComponentType requiredAdditionalComponent = renderableType == ComponentType::PK_RENDERABLE_SKINNED ? ComponentType::PK_ANIMATION_DATA : ComponentType::PK_EMPTY;
            submit_renderable(
                _pCurrentScene,
                _pCurrentScene->componentPools[renderableType],
                transformPool,
                entities,
                rIt->second,
                ComponentType::PK_TRANSFORM | renderableType,
                requiredAdditionalComponent
            );
        }
        */
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
            Context::waitForOperations();
            pApp->getMasterRenderer()->cleanRenderers();
            pApp->getInputManager().destroyEvents();
            pApp->getAssetManager()->destroyAssets();

            delete _pCurrentScene;
            _pCurrentScene = _pNextScene;
            _pCurrentScene->init();

            _pNextScene = nullptr;
        }
    }
}
