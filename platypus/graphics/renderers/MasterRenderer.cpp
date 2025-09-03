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


namespace platypus
{
    MasterRenderer::MasterRenderer(const Window& window) :
        _swapchain(window),
        _descriptorPool(_swapchain),

        _scene3DDataDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { 0, ShaderDataType::Mat4 },
                        { 1, ShaderDataType::Mat4 },
                        { 2, ShaderDataType::Float4 },
                        { 3, ShaderDataType::Float4 },
                        { 4, ShaderDataType::Float4 },
                        { 5, ShaderDataType::Float4 }
                    }
                }
            }
        )
    {
        _pRenderer3D = std::make_unique<Renderer3D>(*this);

        _pGUIRenderer = std::make_unique<GUIRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE | ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        allocCommandBuffers(_swapchain.getMaxFramesInFlight());
        createCommonShaderResources();

        BatchPool::init();
    }

    MasterRenderer::~MasterRenderer()
    {
        _descriptorPool.freeDescriptorSets(_scene3DDescriptorSets);

        for (Buffer* pBuffer : _scene3DDataUniformBuffers)
            delete pBuffer;

        _scene3DDataUniformBuffers.clear();
        _scene3DDataDescriptorSetLayout.destroy();

        BatchPool::destroy();
    }

    void MasterRenderer::cleanRenderers()
    {
        Device::wait_for_operations();

        BatchPool::free_batches();
        _pGUIRenderer->freeBatches();

        freeShaderResources();
    }

    void MasterRenderer::cleanUp()
    {
        BatchPool::free_batches();
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
        uint64_t requiredMask3 = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM | ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        if (!(entity.componentMask & requiredMask1) &&
            !(entity.componentMask & requiredMask2) &&
            !(entity.componentMask & requiredMask3))
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
            ID_t batchID = BatchPool::get_batch_id(meshID, materialID);
            if (batchID == NULL_ID)
            {
                Debug::log(
                    "@MasterRenderer::submit "
                    "No suitable batch found for StaticMeshRenderable. Creating a new one..."
                );
                // TODO: Error handling if creation fails
                batchID = BatchPool::create_static_batch(meshID, materialID);
            }
            BatchPool::add_to_static_batch(batchID, pTransform->globalMatrix, _currentFrame);
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
            ID_t batchID = BatchPool::get_batch_id(meshID, materialID);
            if (batchID == NULL_ID)
            {
                Debug::log(
                    "@MasterRenderer::submit "
                    "No suitable batch found for SkinnedMeshRenderable. Creating a new one..."
                );
                // TODO: Error handling if creation fails
                batchID = BatchPool::create_skinned_batch(meshID, materialID);
            }

            const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );

            // NOTE: Not sure are jointMatrices provided correctly here?
            BatchPool::add_to_skinned_batch(
                batchID,
                (void*)pAnimation->jointMatrices,
                sizeof(Matrix4f) * pAnimation->jointCount,
                _currentFrame
            );
        }

        if ((entity.componentMask & _pGUIRenderer->getRequiredComponentsMask()) == _pGUIRenderer->getRequiredComponentsMask())
            _pGUIRenderer->submit(pScene, entity.id);
    }

    void MasterRenderer::render(const Window& window)
    {
        SwapchainResult result = _swapchain.acquireImage();
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
                _swapchain,
                cmdBuf,
                _swapchain.getCurrentFrame()
            );

            // present may also tell us to recreate swapchain!
            if (_swapchain.present() == SwapchainResult::RESIZE_REQUIRED || window.resized())
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
        const Extent2D swapchainExtent = _swapchain.getExtent();

        _pGUIRenderer->createPipeline(
            _swapchain.getRenderPass(),
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
        for (int i = 0; i < _swapchain.getMaxFramesInFlight(); ++i)
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
                    &_scene3DDataDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER, pScene3DDataUniformBuffer } }
                )
            );
        }
    }

    void MasterRenderer::createShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->createShaderResources();
    }

    void MasterRenderer::freeShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->freeShaderResources();
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

        currentCommandBuffer.begin(_swapchain.getRenderPass());

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        Scene* pScene = sceneManager.accessCurrentScene();

        render::begin_render_pass(
            currentCommandBuffer,
            _swapchain,
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

        const Extent2D swapchainExtent = _swapchain.getExtent();
        // NOTE:
        //      *Before sending the complete batch to Renderer3D, need to update device side buffers,
        //      because when adding to a batch, it only updates the host side!
        BatchPool::update_device_side_buffers(_currentFrame);
        secondaryCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _swapchain.getRenderPass(),
                (float)swapchainExtent.width,
                (float)swapchainExtent.height,
                BatchPool::get_batches()
            )
        );
        // NOTE: Need to reset batches for next frame's submits
        //      -> Otherwise adding endlessly
        BatchPool::reset_for_next_frame();

        secondaryCommandBuffers.push_back(
            _pGUIRenderer->recordCommandBuffer(
                _swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                orthographicProjectionMatrix,
                _currentFrame
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        size_t maxFramesInFlight = _swapchain.getMaxFramesInFlight();
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
            _swapchain.recreate(window);
            freeShaderResources();
            destroyPipelines();
            freeCommandBuffers();
            allocCommandBuffers(_swapchain.getMaxFramesInFlight());
            createPipelines();
            createShaderResources();
            // NOTE: Makes window resizing even slower.
            //  -> Required tho, because need to get new descriptor sets for batches!
            BatchPool::free_batches();
            window.resetResized();
        }
        else
        {
            pApp->getInputManager().waitEvents();
        }
    }
}
