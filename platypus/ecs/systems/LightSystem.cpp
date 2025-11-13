#include "LightSystem.hpp"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/utils/Maths.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"

#include <cmath>


namespace platypus
{
    LightSystem::LightSystem()
    {
        _requiredComponentMask = ComponentType::COMPONENT_TYPE_LIGHT;
    }

    LightSystem::~LightSystem()
    {}

    void LightSystem::update(Scene* pScene)
    {
        for (const Entity& entity : pScene->getEntities())
        {
            if ((entity.componentMask & _requiredComponentMask) != _requiredComponentMask)
                continue;

            Light* pLightComponent = (Light*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_LIGHT
            );

            if (pLightComponent->type == LightType::DIRECTIONAL_LIGHT)
            {
                updateDirectionalLight(pScene, pLightComponent);
            }
            else
            {
                Debug::log(
                    "@LightSystem::update "
                    "Light type was " + light_type_to_string(pLightComponent->type) + " "
                    "currently supporting only " + light_type_to_string(LightType::DIRECTIONAL_LIGHT),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                continue;
            }
        }
    }

    static void get_view_frustum_bounds(
        float& outMinX, float& outMaxX,
        float& outMinY, float& outMaxY,
        float& outMinZ, float& outMaxZ,
        const Matrix4f& transformationMatrix,
        Vector4f points[8]
    )
    {
        bool first = true;
        for (int i = 0; i < 8; ++i)
        {
            Vector4f& p = points[i];
            p = transformationMatrix * p;
            if (first)
            {
                outMinX = p.x;
                outMinY = p.y;
                outMinZ = p.z;

                outMaxX = p.x;
                outMaxY = p.y;
                outMaxZ = p.z;

                first = false;
                continue;
            }

            outMinX = std::min(p.x, outMinX);
            outMinY = std::min(p.y, outMinY);
            outMinZ = std::max(p.z, outMinZ);

            outMaxX = std::max(p.x, outMaxX);
            outMaxY = std::max(p.y, outMaxY);
            outMaxZ = std::min(p.z, outMaxZ);
        }
    }

    void LightSystem::updateDirectionalLight(Scene* pScene, Light* pDirectionalLight)
    {
        entityID_t cameraEntity = pScene->getActiveCameraEntity();
        if (cameraEntity == NULL_ENTITY_ID)
        {
            Debug::log(
                "@LightSystem::updateDirectionalLight "
                "Scene's active camera was NULL_ENTITY_ID",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Camera* pCameraComponent = (Camera*)pScene->getComponent(
            cameraEntity,
            ComponentType::COMPONENT_TYPE_CAMERA
        );
        Transform* pCameraTransformComponent = (Transform*)pScene->getComponent(
            cameraEntity,
            ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        const float maxShadowDistance = pDirectionalLight->maxShadowDistance;
        // calc ortho proj matrix to take biggest possible shape on the camera's view frustum
        Vector4f points[4 + 4]; // 4 + 4 : nearplane + far plane vertices

        const float aspectRatio = pCameraComponent->aspectRatio;
        const float fov = pCameraComponent->fov;
        const float zNear = pCameraComponent->zNear;
        const float zFar = pCameraComponent->zFar; // Overridden by maxShadowDistance?

        float nearPlaneWidth = tan(fov) * zNear;
        float nearPlaneHeight = nearPlaneWidth / aspectRatio;

        float farPlaneWidth = tan(fov) * maxShadowDistance;
        float farPlaneHeight = farPlaneWidth / aspectRatio;

        // near plane:

        // top left
        points[0] = Vector4f(-nearPlaneWidth, nearPlaneHeight, -zNear, 1.0f);
        // topRight
        points[1] = Vector4f(nearPlaneWidth, nearPlaneHeight, -zNear, 1.0f);
        // bottom left
        points[2] = Vector4f(-nearPlaneWidth, -nearPlaneHeight, -zNear, 1.0f);
        // bottom right
        points[3] = Vector4f(nearPlaneWidth, -nearPlaneHeight, -zNear, 1.0f);

        // far plane:

        // top left
        points[4] = Vector4f(-farPlaneWidth, farPlaneHeight, -maxShadowDistance, 1.0f);
        // topRight
        points[5] = Vector4f(farPlaneWidth, farPlaneHeight, -maxShadowDistance, 1.0f);
        // bottom left
        points[6] = Vector4f(-farPlaneWidth, -farPlaneHeight, -maxShadowDistance, 1.0f);
        // bottom right
        points[7] = Vector4f(farPlaneWidth, -farPlaneHeight, -maxShadowDistance, 1.0f);


        // Determine actual view frustum bounds

        // transformation of the camera (just inverse of the viewMat)
        const Matrix4f& cameraTransformationMatrix = pCameraTransformComponent->globalMatrix;

        float pMinX = INFINITY;
        float pMinY = INFINITY;
        float pMinZ = -INFINITY;

        float pMaxX = -INFINITY;
        float pMaxY = -INFINITY;
        float pMaxZ = INFINITY;

        get_view_frustum_bounds(
            pMinX, pMaxX,
            pMinY, pMaxY,
            pMinZ, pMaxZ,
            cameraTransformationMatrix,
            points
        );

        // Get the shadow area's center point for the shadow caster's view matrix
        Vector3f centerPos(
            (pMinX + pMaxX) * 0.5f,
            (pMinY + pMaxY) * 0.5f,
            (pMinZ + pMaxZ) * 0.5f
        );

        // now we can get the dimensions of the "shadow area's orthographic projection matrix"
        float width = (pMaxX - pMinX);
        float height = (pMaxY - pMinY);
        float length = (pMaxZ - pMinZ);

        // NOTE: Old comment below, currently the shadows seem fine without multiplying the height by 2...
        // * I have no idea why height has to be multiplied by 2, but if it wasnt multiplied by 2 we may
        // sometimes get shadows looking way too wrong..
        pDirectionalLight->shadowProjectionMatrix = create_orthographic_projection_matrix(
            -width,
            width,
            height, // * 2,
            -height, //  * 2,
            -length,
            length
        );

        // Create rotation matrix from light direction
        const Vector3f lightDirection = pDirectionalLight->direction;
        const Vector3f globalUp(0, 1, 0);

        Vector3f xaxis = globalUp.cross(lightDirection);
        xaxis = xaxis.normalize();

        Vector3f yaxis = lightDirection.cross(xaxis);
        yaxis.normalize();

        Matrix4f rotationMatrix(1.0f);

        rotationMatrix[0 + 0 * 4] = xaxis.x;
        rotationMatrix[1 + 0 * 4] = yaxis.x;
        rotationMatrix[2 + 0 * 4] = lightDirection.x;

        rotationMatrix[0 + 1 * 4] = xaxis.y;
        rotationMatrix[1 + 1 * 4] = yaxis.y;
        rotationMatrix[2 + 1 * 4] = lightDirection.y;

        rotationMatrix[0 + 2 * 4] = xaxis.z;
        rotationMatrix[1 + 2 * 4] = yaxis.z;
        rotationMatrix[2 + 2 * 4] = lightDirection.z;

        pDirectionalLight->shadowViewMatrix = create_view_matrix(centerPos, rotationMatrix);
    }
}
