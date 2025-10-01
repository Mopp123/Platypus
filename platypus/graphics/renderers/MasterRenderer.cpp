#include "MasterRenderer.h"
#include "platypus/core/Application.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Lights.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/assets/TerrainMesh.hpp"


namespace platypus
{
    MasterRenderer::MasterRenderer(Swapchain& swapchain) :
        _swapchainRef(swapchain),
        _descriptorPool(_swapchainRef),
        _batcher(*this, _descriptorPool, 1000, 1000, 9, 50),

        _scene3DDataDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { ShaderDataType::Mat4 },
                        { ShaderDataType::Mat4 },
                        { ShaderDataType::Float4 },
                        { ShaderDataType::Float4 },
                        { ShaderDataType::Float4 },
                        { ShaderDataType::Float4 }
                    }
                }
            }
        ),
        _testFramebufferTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            false,
            0
        )
    {
        _pRenderer3D = std::make_unique<Renderer3D>(*this);

        _pGUIRenderer = std::make_unique<GUIRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE | ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        allocCommandBuffers(_swapchainRef.getMaxFramesInFlight());
        createCommonShaderResources();

        // TESTING
        uint32_t swapchainWidth = _swapchainRef.getExtent().width;
        uint32_t swapchainHeight = _swapchainRef.getExtent().height;
        _pTestFramebufferColorTexture = new Texture(
            TextureUsage::FRAMEBUFFER_COLOR,
            _testFramebufferTextureSampler,
            ImageFormat::R8G8B8A8_UNORM, // TODO: Query available color format instead of hard coding here!!!!
            swapchainWidth,
            swapchainHeight
        );
        Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pTestFramebufferColorTexture);

        _pTestFramebufferDepthTexture = new Texture(
            TextureUsage::FRAMEBUFFER_DEPTH,
            _testFramebufferTextureSampler,
            ImageFormat::D32_SFLOAT, // TODO: Query available depth format instead of hard coding here!!!!
            swapchainWidth,
            swapchainHeight
        );
        _testRenderPass.create(
            ImageFormat::R8G8B8A8_UNORM,
            ImageFormat::D32_SFLOAT,
            true
        );
        _pTestFramebuffer = new Framebuffer(
            _testRenderPass,
            { _pTestFramebufferColorTexture, _pTestFramebufferDepthTexture },
            swapchainWidth,
            swapchainHeight
        );
    }

    MasterRenderer::~MasterRenderer()
    {
        delete _pTestFramebuffer;
        _testRenderPass.destroy();
        delete _pTestFramebufferDepthTexture;
        //delete _pTestFramebufferColorTexture;

        destroyCommonShaderResources();
        _scene3DDataDescriptorSetLayout.destroy();
    }

    void MasterRenderer::cleanRenderers()
    {
        Device::wait_for_operations();

        _batcher.freeBatches();
        _pGUIRenderer->freeBatches();
    }

    void MasterRenderer::cleanUp()
    {
        _batcher.freeBatches();
        cleanRenderers();
        destroyPipelines();
        // NOTE: Why need to free command buffers here?
        //  -> cleanUp() is called on scene change, but new scene doesn't need new command buffers!
        freeCommandBuffers();
    }

    void MasterRenderer::submit(const Scene* pScene, const Entity& entity)
    {
        uint64_t requiredMask1 = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE;
        uint64_t requiredMask2 = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE;
        uint64_t requiredMask3 = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE;
        uint64_t requiredMask4 = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM | ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        if (!(entity.componentMask & requiredMask1) &&
            !(entity.componentMask & requiredMask2) &&
            !(entity.componentMask & requiredMask3) &&
            !(entity.componentMask & requiredMask4))
        {
            return;
        }

        // NOTE: Below should rather be done by the "batcher" since these are kind of batching related operations?!
        Transform* pTransform = (Transform*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_TRANSFORM,
            false,
            false
        );

        StaticMeshRenderable* pStaticRenderable = (StaticMeshRenderable*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE,
            false,
            false
        );
        if (pStaticRenderable)
        {
            const ID_t meshID = pStaticRenderable->meshID;
            const ID_t materialID = pStaticRenderable->materialID;
            ID_t batchID = _batcher.getBatchID(meshID, materialID);
            if (batchID == NULL_ID)
            {
                Debug::log(
                    "@MasterRenderer::submit "
                    "No suitable batch found for StaticMeshRenderable. Creating a new one..."
                );
                // TODO: Error handling if creation fails
                batchID = _batcher.createStaticBatch(meshID, materialID);
            }
            _batcher.addToStaticBatch(batchID, pTransform->globalMatrix, _currentFrame);
        }

        SkinnedMeshRenderable* pSkinnedRenderable = (SkinnedMeshRenderable*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_SKINNED_MESH_RENDERABLE,
            false,
            false
        );
        if (pSkinnedRenderable)
        {
            const ID_t meshID = pSkinnedRenderable->meshID;
            const ID_t materialID = pSkinnedRenderable->materialID;
            ID_t batchID = _batcher.getBatchID(meshID, materialID);
            if (batchID == NULL_ID)
            {
                Debug::log(
                    "@MasterRenderer::submit "
                    "No suitable batch found for SkinnedMeshRenderable. Creating a new one..."
                );
                // TODO: Error handling if creation fails
                batchID = _batcher.createSkinnedBatch(meshID, materialID);
            }

            const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );
            const Mesh* pSkinnedMesh = (const Mesh*)Application::get_instance()->getAssetManager()->getAsset(
                pSkinnedRenderable->meshID,
                AssetType::ASSET_TYPE_MESH
            );

            _batcher.addToSkinnedBatch(
                batchID,
                (void*)pAnimation->jointMatrices,
                sizeof(Matrix4f) * pSkinnedMesh->getJointCount(),
                _currentFrame
            );
        }

        TerrainMeshRenderable* pTerrainRenderable = (TerrainMeshRenderable*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_TERRAIN_MESH_RENDERABLE,
            false,
            false
        );
        if (pTerrainRenderable)
        {
            const ID_t terrainMeshID = pTerrainRenderable->terrainMeshID;
            const ID_t materialID = pTerrainRenderable->materialID;
            ID_t batchID = _batcher.getBatchID(terrainMeshID, materialID);
            if (batchID == NULL_ID)
            {
                Debug::log(
                    "@MasterRenderer::submit "
                    "No suitable batch found for TerrainMeshRenderable. Creating a new one..."
                );
                // TODO: Error handling if creation fails
                batchID = _batcher.createTerrainBatch(terrainMeshID, materialID);
            }

            const TerrainMesh* pTerrainMesh = (const TerrainMesh*)Application::get_instance()->getAssetManager()->getAsset(
                pTerrainRenderable->terrainMeshID,
                AssetType::ASSET_TYPE_TERRAIN_MESH
            );
            _batcher.addToTerrainBatch(
                batchID,
                pTransform->globalMatrix,
                pTerrainMesh->getTileSize(),
                pTerrainMesh->getVerticesPerRow(),
                _currentFrame
            );
        }

        if ((entity.componentMask & _pGUIRenderer->getRequiredComponentsMask()) == _pGUIRenderer->getRequiredComponentsMask())
            _pGUIRenderer->submit(pScene, entity.id);
    }

    void MasterRenderer::render(const Window& window)
    {
        SwapchainResult result = _swapchainRef.acquireImage();
        if (result == SwapchainResult::ERROR)
        {
            Debug::log(
                "@MasterRenderer::render "
                "Failed to acquire swapchain image!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        else if (result == SwapchainResult::RESIZE_REQUIRED)
        {
            handleWindowResize();
        }
        else
        {
            const CommandBuffer& cmdBuf = recordCommandBuffer();
            Device::submit_primary_command_buffer(
                _swapchainRef,
                cmdBuf,
                _swapchainRef.getCurrentFrame()
            );

            // present may also tell us to recreate swapchain!
            if (_swapchainRef.present() == SwapchainResult::RESIZE_REQUIRED || window.resized())
                handleWindowResize();
        }
    }

    void MasterRenderer::allocCommandBuffers(uint32_t count)
    {
        _primaryCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            count,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        );

        // TODO: Make all renderers alloc same way!
        _pRenderer3D->allocCommandBuffers();
        _pGUIRenderer->allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        _pGUIRenderer->freeCommandBuffers();
        _pRenderer3D->freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();

        _primaryCommandBuffers.clear();
    }

    void MasterRenderer::createPipelines()
    {
        const Extent2D swapchainExtent = _swapchainRef.getExtent();

        _pGUIRenderer->createPipeline(
            _swapchainRef.getRenderPass(),
            swapchainExtent.width,
            swapchainExtent.height
        );

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->recreateExistingPipeline();
    }

    void MasterRenderer::destroyPipelines()
    {
        _pGUIRenderer->destroyPipeline();

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->destroyPipeline();
    }

    void MasterRenderer::createCommonShaderResources()
    {
        // Create common uniform buffers and descriptor sets
        Scene3DData scene3DData;
        for (int i = 0; i < _swapchainRef.getMaxFramesInFlight(); ++i)
        {
            Buffer* pScene3DDataUniformBuffer = new Buffer(
                &scene3DData,
                sizeof(scene3DData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            _scene3DDataUniformBuffers.push_back(pScene3DDataUniformBuffer);

            _scene3DDescriptorSets.push_back(
                _descriptorPool.createDescriptorSet(
                    _scene3DDataDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pScene3DDataUniformBuffer } }
                )
            );
        }
    }

    void MasterRenderer::destroyCommonShaderResources()
    {
        _descriptorPool.freeDescriptorSets(_scene3DDescriptorSets);
        _scene3DDescriptorSets.clear();

        for (Buffer* pBuffer : _scene3DDataUniformBuffers)
            delete pBuffer;

        _scene3DDataUniformBuffers.clear();
    }

    const CommandBuffer& MasterRenderer::recordCommandBuffer()
    {
        if (_currentFrame >= _primaryCommandBuffers.size())
        {
            Debug::log(
                "@MasterRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_primaryCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _primaryCommandBuffers[_currentFrame];

        currentCommandBuffer.begin(_swapchainRef.getRenderPass());

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        Scene* pScene = sceneManager.accessCurrentScene();

        // TESTING MULTIPLE PASSEs
        render::begin_render_pass(
            currentCommandBuffer,
            _testRenderPass,
            *_pTestFramebuffer,
            { 1, 0, 1, 1 },
            true
        );
        render::end_render_pass(currentCommandBuffer);

        render::begin_render_pass(
            currentCommandBuffer,
            _swapchainRef.getRenderPass(),
            *_swapchainRef.getFramebuffers()[_swapchainRef.getCurrentImageIndex()],
            pScene->environmentProperties.clearColor,
            true
        );

        // NOTE: We create new copies of secondary command buffers here
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;

        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f orthographicProjectionMatrix = Matrix4f(1.0f);
        Matrix4f viewMatrix = Matrix4f(1.0f);

        Camera* pCamera = (Camera*)pScene->getComponent(ComponentType::COMPONENT_TYPE_CAMERA);
        Transform* pCameraTransform = (Transform*)pScene->getComponent(ComponentType::COMPONENT_TYPE_TRANSFORM);
        Vector4f cameraPosition;
        if (pCamera)
        {
            perspectiveProjectionMatrix = pCamera->perspectiveProjectionMatrix;
            orthographicProjectionMatrix = pCamera->orthographicProjectionMatrix;
        }
        _scene3DData.perspectiveProjectionMatrix = perspectiveProjectionMatrix;

        if (pCameraTransform)
        {
            const Matrix4f cameraTransformationMatrix = pCameraTransform->globalMatrix;
            cameraPosition.x = cameraTransformationMatrix[0 + 3 * 4];
            cameraPosition.y = cameraTransformationMatrix[1 + 3 * 4];
            cameraPosition.z = cameraTransformationMatrix[2 + 3 * 4];
            viewMatrix = cameraTransformationMatrix.inverse();
        }
        _scene3DData.cameraPosition = cameraPosition;
        _scene3DData.viewMatrix = viewMatrix;

        const DirectionalLight* pDirectionalLight = (const DirectionalLight*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT,
            false
        );
        if (!pDirectionalLight)
        {
            // TODO: Normalize
            _scene3DData.lightDirection = { 0.75f, -1.5f, -1.0f, 0.0f };
            _scene3DData.lightColor = { 0, 0, 0, 1 };
        }
        else
        {
            _scene3DData.lightDirection = {
                pDirectionalLight->direction.x,
                pDirectionalLight->direction.y,
                pDirectionalLight->direction.z,
                0.0f
            };
            _scene3DData.lightColor = {
                pDirectionalLight->color.r,
                pDirectionalLight->color.g,
                pDirectionalLight->color.b,
                1.0f
            };
        }

        const Vector3f sceneAmbientLight = pScene->environmentProperties.ambientColor;
        _scene3DData.ambientLightColor = {
            sceneAmbientLight.r,
            sceneAmbientLight.g,
            sceneAmbientLight.b,
            1.0f
        };

        _scene3DDataUniformBuffers[_currentFrame]->updateDeviceAndHost(
            &_scene3DData,
            sizeof(Scene3DData),
            0
        );

        const Extent2D swapchainExtent = _swapchainRef.getExtent();

        // NOTE:
        //      *Before sending the complete batch to Renderer3D, need to update device side buffers,
        //      because when adding to a batch, it only updates the host side!
        _batcher.updateDeviceSideBuffers(_currentFrame);
        secondaryCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _swapchainRef.getRenderPass(),
                (float)swapchainExtent.width,
                (float)swapchainExtent.height,
                _batcher.getBatches()
            )
        );
        // NOTE: Need to reset batches for next frame's submits
        //      -> Otherwise adding endlessly
        _batcher.resetForNextFrame();

        secondaryCommandBuffers.push_back(
            _pGUIRenderer->recordCommandBuffer(
                _swapchainRef.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                orthographicProjectionMatrix,
                _currentFrame
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        size_t maxFramesInFlight = _swapchainRef.getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    void MasterRenderer::handleWindowResize()
    {
        Application* pApp = Application::get_instance();
        Device::wait_for_operations();

        Window& window = pApp->getWindow();
        if (!window.isMinimized())
        {
            Device::handle_window_resize();
            _swapchainRef.recreate(window);
            if (_swapchainRef.imageCountChanged())
            {
                Debug::log(
                    "@MasterRenderer::handleWindowResize "
                    "Swapchain's image count changed! "
                    "Recreating shader resources, command buffers, pipelines and batches...",
                    Debug::MessageType::PLATYPUS_WARNING
                );
                destroyCommonShaderResources();
                destroyPipelines();
                freeCommandBuffers();
                allocCommandBuffers(_swapchainRef.getMaxFramesInFlight());
                createPipelines();
                createCommonShaderResources();
                // NOTE: Makes window resizing even slower.
                //  -> Required tho, because need to get new descriptor sets for batches!
                _batcher.freeBatches();
            }
            else
            {
                Debug::log(
                    "@MasterRenderer::handleWindowResize "
                    "Swapchain's image count didn't change. "
                    "Recreating pipelines...",
                    Debug::MessageType::PLATYPUS_WARNING
                );
                destroyPipelines();
                createPipelines();
            }

            _swapchainRef.resetChangedImageCount();
            window.resetResized();
        }
        else
        {
            pApp->getInputManager().waitEvents();
        }
    }
}
