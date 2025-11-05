#include "MasterRenderer.h"
#include "platypus/assets/Texture.h"
#include "platypus/core/Application.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Lights.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "StaticBatch.hpp"
#include "SkinnedBatch.hpp"
#include "TerrainBatch.hpp"


namespace platypus
{
    MasterRenderer::MasterRenderer(Swapchain& swapchain) :
        _swapchainRef(swapchain),
        _descriptorPool(_swapchainRef),
        _batcher(*this, _descriptorPool, 1000, 1000, 9, 52),

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
        _testRenderPass(RenderPassType::SHADOW_PASS, true),
        _testFramebufferTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            false,
            0
        ),
        _shadowmapDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    {
                        { }
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

        allocCommandBuffers(_swapchainRef.getMaxFramesInFlight());
        createCommonShaderResources();

        // TESTING ------------------------------------------------------------
        _testRenderPass.create(
            ImageFormat::R8G8B8A8_SRGB,
            ImageFormat::D32_SFLOAT
        );
        createOffscreenResourcesTEST();
    }

    MasterRenderer::~MasterRenderer()
    {
        destroyOffscreenResourcesTEST();
        _testRenderPass.destroy();
        _shadowmapDescriptorSetLayout.destroy();

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

        // NOTE: ONLY TESTING, DANGEROUS AND INEFFICIENT AS FUCK!
        // TODO: Better way to get shadow caster proj and view matrices!!!!
        void* pShadowPassPushConstants = nullptr;
        size_t shadowPassPushConstantsSize = 0;
        DirectionalLight* pDirectionalLight = (DirectionalLight*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT
        );
        if (pDirectionalLight)
        {
            pShadowPassPushConstants = (void*)pDirectionalLight;
            shadowPassPushConstantsSize = sizeof(Matrix4f) * 2;
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
            ID_t batchID = ID::hash(meshID, materialID);
            Batch* pBatch = _batcher.getBatch(RenderPassType::SCENE_PASS, batchID);
            if (!pBatch)
            {
                create_static_batch(
                    _batcher,
                    _batcher.getMaxStaticBatchLength(),
                    _swapchainRef.getRenderPassPtr(),
                    meshID,
                    materialID
                );

                //create_static_shadow_batch(
                //    _batcher,
                //    _batcher.getMaxStaticBatchLength(),
                //    &_testRenderPass,
                //    meshID,
                //    materialID,
                //    pShadowPassPushConstants,
                //    shadowPassPushConstantsSize
                //);
            }
            add_to_static_batch(
                _batcher,
                batchID,
                pTransform->globalMatrix,
                _currentFrame
            );
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
            ID_t batchID = ID::hash(meshID, materialID);
            const size_t maxSkinnedBatchLength = _batcher.getMaxSkinnedBatchLength();
            const size_t maxJoints = _batcher.getMaxSkinnedMeshJoints();
            Batch* pBatch = _batcher.getBatch(RenderPassType::SCENE_PASS, batchID);
            if (!pBatch)
            {
                create_skinned_batch(
                    _batcher,
                    maxSkinnedBatchLength,
                    maxJoints,
                    _swapchainRef.getRenderPassPtr(),
                    meshID,
                    materialID
                );

                //create_skinned_shadow_batch(
                //    _batcher,
                //    maxSkinnedBatchLength,
                //    maxJoints,
                //    &_testRenderPass,
                //    meshID,
                //    materialID,
                //    pShadowPassPushConstants,
                //    shadowPassPushConstantsSize
                //);
            }
            const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
                entity.id,
                ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
            );
            const Mesh* pSkinnedMesh = (const Mesh*)Application::get_instance()->getAssetManager()->getAsset(
                pSkinnedRenderable->meshID,
                AssetType::ASSET_TYPE_MESH
            );

            add_to_skinned_batch(
                _batcher,
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
            const ID_t meshID = pTerrainRenderable->meshID;
            const ID_t materialID = pTerrainRenderable->materialID;
            ID_t batchID = ID::hash(meshID, materialID);
            Batch* pBatch = _batcher.getBatch(RenderPassType::SCENE_PASS, batchID);
            if (!pBatch)
            {
                create_terrain_batch(
                    _batcher,
                    _batcher.getMaxTerrainBatchLength(),
                    _swapchainRef.getRenderPassPtr(),
                    meshID,
                    materialID,
                    pShadowPassPushConstants,
                    shadowPassPushConstantsSize
                );
            }
            add_to_terrain_batch(
                _batcher,
                batchID,
                pTransform->globalMatrix,
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

    void MasterRenderer::solveVertexBufferLayouts(
        const VertexBufferLayout& meshVertexBufferLayout,
        bool instanced,
        bool skinned,
        bool shadowPipeline,
        std::vector<VertexBufferLayout>& outVertexBufferLayouts
    ) const
    {
        if (!shadowPipeline)
        {
            outVertexBufferLayouts.push_back(meshVertexBufferLayout);
        }
        else
        {
            if (!skinned)
            {
                outVertexBufferLayouts.push_back(
                    {
                        {{ 0, ShaderDataType::Float3 }},
                        VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
                        0,
                        meshVertexBufferLayout.getStride()
                    }
                );
            }
            else
            {
                outVertexBufferLayouts.push_back(
                    VertexBufferLayout::get_common_skinned_shadow_layout(
                        meshVertexBufferLayout.getStride()
                    )
                );
            }
        }

        if (instanced)
        {
            uint32_t meshVBLayoutElements = outVertexBufferLayouts.back().getElements().size();
            VertexBufferLayout instancedVBLayout = {
                {
                    { meshVBLayoutElements, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 1, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 2, ShaderDataType::Float4 },
                    { meshVBLayoutElements + 3, ShaderDataType::Float4 }
                },
                VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
                1
            };
            outVertexBufferLayouts.push_back(instancedVBLayout);
        }
    }

    void MasterRenderer::solveDescriptorSetLayouts(
        const Material* pMaterial,
        bool skinned,
        bool shadowPipeline,
        std::vector<DescriptorSetLayout>& outDescriptorSetLayouts
    ) const
    {
        MaterialType materialType = MaterialType::NONE;
        if (pMaterial)
        {
            materialType = pMaterial->getMaterialType();
            if (skinned && materialType == MaterialType::TERRAIN)
            {
                Debug::log(
                    "@MasterRenderer::solveDescriptorSetLayouts "
                    "Illegal to solve descriptor set layouts for Terrain Materials with skinning!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        if (!shadowPipeline)
        {
            outDescriptorSetLayouts.push_back(_scene3DDataDescriptorSetLayout);
        }
        else
        {
            // TODO: Common shadow descriptor set layout (light view and proj matrices, etc)
            //outDescriptorSetLayouts.push_back(_scene3DDataDescriptorSetLayout);
        }

        if (skinned)
        {
            outDescriptorSetLayouts.push_back(Batcher::get_joint_descriptor_set_layout());
        }
        else if (materialType == MaterialType::TERRAIN)
        {
            outDescriptorSetLayouts.push_back(Batcher::get_terrain_descriptor_set_layout());
        }

        // Checking if shadow pipeline here, since need to add the Material descriptor set layout
        // last if it's used!
        if (!shadowPipeline && pMaterial)
            outDescriptorSetLayouts.push_back(pMaterial->getDescriptorSetLayout());
    }

    void MasterRenderer::createOffscreenResourcesTEST()
    {
        uint32_t swapchainWidth = _swapchainRef.getExtent().width;
        uint32_t swapchainHeight = _swapchainRef.getExtent().height;
        ImageFormat testFramebufferColorFormat = ImageFormat::R8G8B8A8_SRGB;
        _pTestFramebufferColorTexture = new Texture(
            TextureType::COLOR_TEXTURE,
            _testFramebufferTextureSampler,
            testFramebufferColorFormat, // TODO: Query available color format instead of hard coding here!!!!
            swapchainWidth,
            swapchainHeight
        );
        Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pTestFramebufferColorTexture);

        _pTestFramebufferDepthTexture = new Texture(
            TextureType::DEPTH_TEXTURE,
            _testFramebufferTextureSampler,
            ImageFormat::D32_SFLOAT, // TODO: Query available depth format instead of hard coding here!!!!
            swapchainWidth,
            swapchainHeight
        );
        Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pTestFramebufferDepthTexture);

        _pTestFramebuffer = new Framebuffer(
            _testRenderPass,
            { _pTestFramebufferColorTexture },
            _pTestFramebufferDepthTexture,
            swapchainWidth,
            swapchainHeight
        );

        // Update new shadow texture for materials that receive shadows
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
        {
            Material* pMaterial = (Material*)pAsset;
            if (pMaterial->receivesShadows())
                pMaterial->updateShadowmapDescriptorSet(_pTestFramebufferDepthTexture);
        }
    }

    void MasterRenderer::destroyOffscreenResourcesTEST()
    {
        Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pTestFramebufferColorTexture);
        Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pTestFramebufferDepthTexture);
        delete _pTestFramebuffer;

        _pTestFramebufferColorTexture = nullptr;
        _pTestFramebufferDepthTexture = nullptr;
        _pTestFramebuffer = nullptr;
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

    void MasterRenderer::createShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->createShaderResources();
    }

    void MasterRenderer::destroyShaderResources()
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->destroyShaderResources();
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

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        Scene* pScene = sceneManager.accessCurrentScene();

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

        CommandBuffer& currentCommandBuffer = _primaryCommandBuffers[_currentFrame];
        currentCommandBuffer.begin(nullptr);



        // TESTING MULTIPLE PASSES -----------------------------------
        //render::begin_render_pass(
        //    currentCommandBuffer,
        //    _testRenderPass,
        //    _pTestFramebuffer,
        //    _pTestFramebuffer->getDepthAttachment(),
        //    { 1, 0, 1, 1 },
        //    true
        //);
        //std::vector<CommandBuffer> testSecondaries;
        //testSecondaries.push_back(
        //    _pRenderer3D->recordCommandBuffer(
        //        _testRenderPass,
        //        (float)swapchainExtent.width,
        //        (float)swapchainExtent.height,
        //        _batcher.getBatches(RenderPassType::SHADOW_PASS)
        //    )
        //);
        //render::exec_secondary_command_buffers(currentCommandBuffer, testSecondaries);
        //render::end_render_pass(currentCommandBuffer);
        // TESTING END ^^^ -------------------------------------------




        const Framebuffer* pCurrentSwapchainFramebuffer = _swapchainRef.getCurrentFramebuffer();
        render::begin_render_pass(
            currentCommandBuffer,
            _swapchainRef.getRenderPass(),
            pCurrentSwapchainFramebuffer,
            nullptr,
            pScene->environmentProperties.clearColor,
            true
        );

        // NOTE: We create new copies of secondary command buffers here
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;
        secondaryCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _swapchainRef.getRenderPass(),
                (float)swapchainExtent.width,
                (float)swapchainExtent.height,
                _batcher.getBatches(RenderPassType::SCENE_PASS)
            )
        );
        // NOTE: Need to reset batches for next frame's submits
        //      -> Otherwise adding endlessly
        _batcher.resetForNextFrame();

        _pRenderer3D->advanceFrame();

        //secondaryCommandBuffers.push_back(
        //    _pGUIRenderer->recordCommandBuffer(
        //        _swapchainRef.getRenderPass(),
        //        swapchainExtent.width,
        //        swapchainExtent.height,
        //        orthographicProjectionMatrix,
        //        _currentFrame
        //    )
        //);

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
                destroyOffscreenResourcesTEST();
                createOffscreenResourcesTEST();

                // NOTE: After added _receiveShadows into Material, it is required to
                // always recreate their shader resources since the shadowmap texture
                // gets recreated!
                destroyShaderResources();
                createShaderResources();

                // NOTE: Freeing batches which causes each batch to be created again when submitting.
                // For this to work, batcher also needs to destroy all managed pipelines!
                //      BUT it may be okay to NOT destroy and recreate existing pipelines since the created batches
                //      will always have same IDs
                //          -> u might be able to just recreate existing managed pipelines?? -> TEST THIS PLZ!
                //  -> Makes window resizing even slower.
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

                destroyOffscreenResourcesTEST();
                createOffscreenResourcesTEST();

                _batcher.recreateManagedPipelines();
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
