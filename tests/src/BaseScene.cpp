#include "BaseScene.hpp"
#include "platypus/ecs/components/Renderable.h"

using namespace platypus;


void BaseScene::SceneWindowResizeEvent::func(int w, int h)
{
    Camera* pCameraComponent = (Camera*)_pScene->getComponent(
        _cameraEntity,
        ComponentType::COMPONENT_TYPE_CAMERA
    );

    float useHeight = h > 0 ? (float)h : 1.0f;
    pCameraComponent->aspectRatio = (float)w / useHeight;
    pCameraComponent->perspectiveProjectionMatrix = create_perspective_projection_matrix(
        pCameraComponent->aspectRatio,
        pCameraComponent->fov,
        pCameraComponent->zNear,
        pCameraComponent->zFar
    );

    pCameraComponent->orthographicProjectionMatrix = create_orthographic_projection_matrix(
        0,
        w,
        0,
        useHeight,
        0,
        0.1f
    );
}


BaseScene::BaseScene()
{}

BaseScene::~BaseScene()
{}

void BaseScene::initBase()
{
    environmentProperties.clearColor = { 0.8f, 0.7f, 0.7f, 1.0f };
    //environmentProperties.ambientColor = { 0.4f, 0.35f, 0.35f };
    environmentProperties.ambientColor = { 0.1f, 0.1f, 0.1f };

    _cameraEntity = createEntity();
    create_transform(
        _cameraEntity,
        { 0.0f, 0.0f, 0.0f },
        { {0, 1, 0}, 0.0f },
        { 1, 1, 1 }
    );
    int windowSurfaceWidth = 0;
    int windowSurfaceHeight = 0;
    Application::get_instance()->getWindow().getSurfaceExtent(
        &windowSurfaceWidth, &windowSurfaceHeight
    );
    float aspectRatio = 1.7f;
    if (windowSurfaceHeight > 0)
        aspectRatio = (float)windowSurfaceWidth / (float)windowSurfaceHeight;

    Debug::log("___TEST___window scale: " + std::to_string(windowSurfaceWidth) + ", " + std::to_string(windowSurfaceHeight));

    Matrix4f orthoProjMat = create_orthographic_projection_matrix(
        0,
        windowSurfaceWidth,
        0,
        windowSurfaceHeight,
        0,
        0.1f
    );

    const float fov = 1.3f * 0.75f;
    const float zNear = 0.1f;
    const float zFar = 100.0f;
    Camera* pCamera = create_camera(
        _cameraEntity,
        aspectRatio,
        fov,
        zNear,
        zFar,
        orthoProjMat
    );
    setActiveCameraEntity(_cameraEntity);

    /*
    _cameraController.init();
    _cameraController.set(
        PLATY_MATH_PI * 0.25f, // pitch
        0.0f,    // yaw
        0.0025f, // rot speed
        40.0f,   // zoom
        80.0f,   // max zoom
        1.25f    // zoom speed
    );
    _cameraController.setOffsetPos({ 0, 4, 0});
    */

    _lightEntity = createEntity();
    Vector3f lightDir( 0.4f, -0.6f, -0.6f);
    lightDir = lightDir.normalize();
    Light* pLight = create_directional_light(
        _lightEntity,
        lightDir,
        { 1, 1, 1 }
    );

    Application::get_instance()->getInputManager().addWindowResizeEvent(
        new SceneWindowResizeEvent(this, _cameraEntity)
    );
}

void BaseScene::updateBase()
{}


Material* BaseScene::createMeshMaterial(
    AssetManager* pAssetManager,
    std::string textureFilepath,
    bool repeatTexture
)
{
    TextureSamplerAddressMode addressMode = TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (repeatTexture)
        addressMode = TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;

    TextureSampler textureSampler(
        TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
        addressMode,
        true,
        0
    );
    Texture* pTexture = pAssetManager->loadTexture(
        textureFilepath,
        ImageFormat::R8G8B8A8_SRGB,
        textureSampler
    );
    Material* pMaterial = pAssetManager->createMaterial(
        MaterialType::MESH,
        NULL_ID,
        { pTexture->getID() },
        { pAssetManager->getWhiteTexture()->getID() },
        { }
    );
    return pMaterial;
}

entityID_t BaseScene::createStaticMeshEntity(
    const Vector3f& position,
    const Quaternion& rotation,
    const Vector3f& scale,
    ID_t meshAssetID,
    ID_t materialAssetID
)
{
    entityID_t entity = createEntity();
    Transform* pTransform = create_transform(
        entity,
        position,
        rotation,
        scale
    );
    StaticMeshRenderable* pRenderable = create_static_mesh_renderable(
        entity,
        meshAssetID,
        materialAssetID
    );
    return entity;
}


entityID_t BaseScene::createSkinnedMeshEntity(
    const platypus::Vector3f& position,
    const platypus::Quaternion& rotation,
    const platypus::Vector3f& scale,
    platypus::Mesh* pMesh,
    platypus::SkeletalAnimationData* pAnimationAsset,
    ID_t materialAssetID,
    std::vector<entityID_t>& outJointEntities
)
{
    // Create entity containing the skeleton
    //  -> otherwise root joint doesn't get animated at all
    entityID_t entity = createEntity();
    create_transform(
        entity,
        position,
        rotation,
        scale
    );

    outJointEntities = create_skeleton(
        pMesh->getBindPose().joints,
        pMesh->getBindPose().jointChildMapping
    );
    entityID_t rootJointEntity = outJointEntities[0];
    create_skinned_mesh_renderable(
        rootJointEntity,
        pMesh->getID(),
        materialAssetID
    );
    create_skeletal_animation(
        rootJointEntity,
        pAnimationAsset->getID()
    );

    add_child(entity, rootJointEntity);

    return entity;
}
