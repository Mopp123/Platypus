#include "LightSystem.hpp"
#include "platypus/ecs/components/Component.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    LightSystem::LightSystem()
    {
        _requiredComponentMask = ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION;
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
                updateDirectionalLight(pLightComponent);
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

    void LightSystem::updateDirectionalLight(Light* pDirectionalLight)
    {
        /*
        Camera* camera = Camera::get_current_camera();

        // calc ortho proj matrix to take biggest possible shape on the camera's veiw frustum
        mml::Vector4 points[4 + 4]; // 4 + 4 : nearplane + far plane vertices

        const float aspectRatio = camera->getAspectRatio();
        const float fov = camera->getFov();
        const float zNear = camera->getZNear();
        const float zFar = camera->getZFar();

        float nearPlaneWidth = std::tan(fov * 0.5f) * zNear;
        float nearPlaneHeight = nearPlaneWidth / aspectRatio;

        float farPlaneWidth = std::tan(fov * 0.5f) * _maxShadowDistance;
        float farPlaneHeight = farPlaneWidth / aspectRatio;

        // near plane:

        // top left
        points[0] = mml::Vector4(-nearPlaneWidth, nearPlaneHeight, -zNear, 1.0f);
        // topRight
        points[1] = mml::Vector4(nearPlaneWidth, nearPlaneHeight, -zNear, 1.0f);
        // bottom left
        points[2] = mml::Vector4(-nearPlaneWidth, -nearPlaneHeight, -zNear, 1.0f);
        // bottom right
        points[3] = mml::Vector4(nearPlaneWidth, -nearPlaneHeight, -zNear, 1.0f);

        // far plane:

        // top left
        points[4] = mml::Vector4(-farPlaneWidth, farPlaneHeight, -_maxShadowDistance, 1.0f);
        // topRight
        points[5] = mml::Vector4(farPlaneWidth, farPlaneHeight, -_maxShadowDistance, 1.0f);
        // bottom left
        points[6] = mml::Vector4(-farPlaneWidth, -farPlaneHeight, -_maxShadowDistance, 1.0f);
        // bottom right
        points[7] = mml::Vector4(farPlaneWidth, -farPlaneHeight, -_maxShadowDistance, 1.0f);


        // Determine actual view frustum bounds

        // transformation of the camera (just inverse of the viewMat)
        mml::Matrix4 transformationMatrix = camera->getViewMatrix();
        transformationMatrix.inverse();

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
            transformationMatrix,
            points
        );

        // Get the shadow area's center point for the shadow caster's view matrix
        mml::Vector3 centerPos(
            (pMinX + pMaxX) * 0.5f,
            (pMinY + pMaxY) * 0.5f,
            (pMinZ + pMaxZ) * 0.5f
        );

        // now we can get the dimensions of the "shadow area's orthographic projection matrix"
        float width = (pMaxX - pMinX);
        float height = (pMaxY - pMinY);
        float length = (pMaxZ - pMinZ);

        // * I have no idea why height has to be multiplied by 2, but if it wasnt multiplied by 2 we may sometimes get
        // shadows looking way too wrong..
        mml::create_orthographic_projection_matrix(_projectionMatrix, -width, width, height * 2, -height * 2, -length, length);

        // Then finally our shadow caster's view matrix
        mml::create_view_matrix(_viewMatrix, centerPos, _transform->getRotation());
        */
    }
}
