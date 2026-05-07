#include "Camera.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    Camera* create_camera(
        entityID_t target,
        float aspectRatio,
        float fov,
        float zNear,
        float zFar,
        const Matrix4f& orthographicProjectionMatrix, // Used for 2D rendering stuff
        Scene* pScene
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_camera"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CAMERA;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        pUseScene->addToComponentMask(target, componentType);
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

    std::vector<char> serialize(const Camera* pCamera)
    {
        const size_t componentTypeSize = sizeof(ComponentType);

        std::vector<char> serializedData(serialized_camera_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_CAMERA;
        memcpy(
            serializedData.data(),
            &componentType,
            componentTypeSize
        );
        size_t pos = componentTypeSize;

        memcpy(
            serializedData.data() + pos,
            &(pCamera->perspectiveProjectionMatrix),
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            serializedData.data() + pos,
            &(pCamera->orthographicProjectionMatrix),
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            serializedData.data() + pos,
            &(pCamera->aspectRatio),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pCamera->fov),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pCamera->zNear),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pCamera->zFar),
            sizeof(float)
        );

        return serializedData;
    }
}
