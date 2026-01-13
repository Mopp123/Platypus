#include "Camera.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    Camera* create_camera(
        entityID_t target,
        float aspectRatio,
        float fov,
        float zNear,
        float zFar,
        const Matrix4f& orthographicProjectionMatrix // Used for 2D rendering stuff
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_camera"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CAMERA;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_camera "
                "Failed to allocate Camera component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        Camera* pCamera = (Camera*)pComponent;
        pCamera->perspectiveProjectionMatrix = create_perspective_projection_matrix(
            aspectRatio,
            fov,
            zNear,
            zFar
        );
        pCamera->orthographicProjectionMatrix = orthographicProjectionMatrix;
        pCamera->aspectRatio = aspectRatio;
        pCamera->fov = fov;
        pCamera->zNear = zNear;
        pCamera->zFar = zFar;
        return pCamera;
    }
}
