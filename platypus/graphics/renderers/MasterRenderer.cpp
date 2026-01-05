#include "MasterRenderer.h"
#include "platypus/assets/Texture.h"
#include "platypus/core/Application.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Lights.h"
#include "platypus/ecs/components/Component.h"
#include "platypus/ecs/components/SkeletalAnimation.h"
#include "platypus/core/Timing.h"
#include "platypus/core/Debug.h"

namespace platypus
{
    MasterRenderer::MasterRenderer(Swapchain& swapchain, ImageFormat shadowmapDepthFormat) :
        _swapchainRef(swapchain),
        _descriptorPool(_swapchainRef),
        _batcher(*this, _descriptorPool, 1000, 1000, 500, 50),

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
                        { ShaderDataType::Float4 },
                        { ShaderDataType::Float4 }, // NOTE: For some reason this worked, even I had forgotten to put the shadow properties into this
                        { ShaderDataType::Float }
                    }
                }
            }
        ),
        _shadowPass(
            RenderPassType::SHADOW_PASS,
            true,
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_DEPTH_DISCRETE,
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_DEPTH
        ),
        _opaquePass(
            RenderPassType::OPAQUE_PASS,
            true,
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE |
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_DEPTH_DISCRETE,
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_COLOR |
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_DEPTH
        ),
        _transparentPass(
            RenderPassType::TRANSPARENT_PASS,
            true,
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_CONTINUE |
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_DEPTH_CONTINUE,
            0
        ),
        _offscreenTextureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            false,
            0
        ),
        _shadowPassInstance(
            _shadowPass,
            _shadowmapWidth,
            _shadowmapWidth,
            _offscreenTextureSampler,
            false
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
        _pPostProcessingRenderer = std::make_unique<PostProcessingRenderer>(
            _descriptorPool,
            _swapchainRef.getRenderPassPtr()
        );

        _pGUIRenderer = std::make_unique<GUIRenderer>(
            *this,
            _descriptorPool,
            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE | ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        allocCommandBuffers(_swapchainRef.getMaxFramesInFlight());
        createCommonShaderResources();

        _shadowPass.create(
            ImageFormat::NONE,
            shadowmapDepthFormat
        );
        // TODO: Query which color and depth formats actually available!
        _offscreenColorFormat = ImageFormat::R8G8B8A8_SRGB;
        _offscreenDepthFormat = ImageFormat::D32_SFLOAT;
        _opaquePass.create(
            _offscreenColorFormat,
            _offscreenDepthFormat
        );
        _transparentPass.create(
            _offscreenColorFormat,
            _offscreenDepthFormat
        );
        createOffscreenPassResources();

        _pPostProcessingRenderer->createFramebuffers();
        _pPostProcessingRenderer->createShaderResources(_pColorAttachment);

        // NOTE: This could fuck things up if not being very careful which
        // pipelines gets created here since this gets called in Application's
        // constructor!
        createPipelines();
    }

    MasterRenderer::~MasterRenderer()
    {
        destroyOffscreenPassResources();
        _shadowPass.destroy();
        _opaquePass.destroy();
        _transparentPass.destroy();
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
        uint64_t requiredMask1 = ComponentType::COMPONENT_TYPE_TRANSFORM | ComponentType::COMPONENT_TYPE_RENDERABLE3D;
        uint64_t requiredMask2 = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM | ComponentType::COMPONENT_TYPE_GUI_RENDERABLE;
        if (!(entity.componentMask & requiredMask1) &&
            !(entity.componentMask & requiredMask2))
        {
            return;
        }

        Light* pDirectionalLight = (Light*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_LIGHT
        );

        // NOTE: Below should rather be done by the "batcher" since these are kind of batching related things?
        Transform* pTransform = (Transform*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_TRANSFORM,
            false,
            false
        );

        Renderable3D* pRenderable3D = (Renderable3D*)pScene->getComponent(
            entity.id,
            ComponentType::COMPONENT_TYPE_RENDERABLE3D,
            false,
            false
        );
        if (pRenderable3D)
        {
            AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
            const ID_t meshID = pRenderable3D->meshID;
            Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
            const MeshType meshType = pMesh->getType();
            const ID_t materialID = pRenderable3D->materialID;
            const Material * const pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

            ID_t batchID = ID::hash(meshID, materialID);

            if (pMaterial->isTransparent())
            {
                // Create transparent batch (if required)
                if (!_batcher.getBatch(RenderPassType::TRANSPARENT_PASS, batchID))
                    _batcher.createBatch(meshID, materialID, pDirectionalLight, &_transparentPass);
            }
            else
            {
                // Create opaque batch (if required)
                if (!_batcher.getBatch(RenderPassType::OPAQUE_PASS, batchID))
                    _batcher.createBatch(meshID, materialID, pDirectionalLight, &_opaquePass);
            }

            // Create shadow batch (if required)
            if (pMaterial->castsShadows() && !_batcher.getBatch(RenderPassType::SHADOW_PASS, batchID))
                _batcher.createBatch(meshID, materialID, pDirectionalLight, &(_shadowPassInstance.getRenderPass()));

            if (meshType == MeshType::MESH_TYPE_STATIC || meshType == MeshType::MESH_TYPE_STATIC_INSTANCED)
            {
                _batcher.addToBatch(
                    batchID,
                    (void*)&(pTransform->globalMatrix),
                    sizeof(Matrix4f),
                    { sizeof(Matrix4f) },
                    _currentFrame
                );
            }
            else if (meshType == MeshType::MESH_TYPE_SKINNED)
            {
                const SkeletalAnimation* pAnimation = (const SkeletalAnimation*)pScene->getComponent(
                    entity.id,
                    ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION
                );
                _batcher.addToBatch(
                    batchID,
                    (void*)pAnimation->jointMatrices,
                    sizeof(Matrix4f) * pMesh->getJointCount(),
                    { sizeof(Matrix4f) * pMesh->getJointCount() },
                    _currentFrame
                );
            }
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
        bool instanced,
        bool skinned,
        bool shadowPipeline,
        std::vector<DescriptorSetLayout>& outDescriptorSetLayouts
    ) const
    {
        bool usingBlendmap = false;
        if (pMaterial)
        {
            usingBlendmap = pMaterial->hasBlendmap();
            if (skinned && usingBlendmap)
            {
                Debug::log(
                    "@MasterRenderer::solveDescriptorSetLayouts "
                    "Currently not supporting Materials with blendmaps for skinned meshes!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            if (instanced && usingBlendmap)
            {
                Debug::log(
                    "@MasterRenderer::solveDescriptorSetLayouts "
                    "Currently not supporting Materials with blendmaps for instanced meshes!",
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
        else if (!instanced)
        {
            outDescriptorSetLayouts.push_back(Batcher::get_static_descriptor_set_layout());
        }

        // Checking if shadow pipeline here, since need to add the Material descriptor set layout
        // last if it's used!
        if (!shadowPipeline && pMaterial)
            outDescriptorSetLayouts.push_back(pMaterial->getDescriptorSetLayout());
    }

    void MasterRenderer::createOffscreenPassResources()
    {
        const Extent2D swapchainExtent = _swapchainRef.getExtent();
        _pColorAttachment = new Texture(
            TextureType::COLOR_TEXTURE,
            _offscreenTextureSampler,
            _offscreenColorFormat,
            swapchainExtent.width,
            swapchainExtent.height
        );
        Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pColorAttachment);

        _pDepthAttachment = new Texture(
            TextureType::DEPTH_TEXTURE,
            _offscreenTextureSampler,
            _offscreenDepthFormat,
            swapchainExtent.width,
            swapchainExtent.height
        );
        Application::get_instance()->getAssetManager()->addExternalPersistentAsset(_pDepthAttachment);

        Debug::log("___TEST___FRAMEBUFFER create opaque");
        _pOpaqueFramebuffer = new Framebuffer(
            _opaquePass,
            { _pColorAttachment },
            _pDepthAttachment,
            swapchainExtent.width,
            swapchainExtent.height
        );
        Debug::log("___TEST___FRAMEBUFFER create transparent");
        _pTransparentFramebuffer = new Framebuffer(
            _transparentPass,
            { _pColorAttachment },
            _pDepthAttachment,
            swapchainExtent.width,
            swapchainExtent.height
        );

        Debug::log("___TEST___FRAMEBUFFER create shadowpass instance");
        _shadowPassInstance.create();

        // NOTE: This is so wrong way of dealing with Materials that are relying on this kind of
        // external stuff...
        //
        // Update new shadow texture for materials that receive shadows
        // Update new "scene depth texture" for materials that are transparent
        // TODO: Some way to know if Material uses framebuffer attachment as texture?
        Texture* pDepthAttachment = _shadowPassInstance.getFramebuffer(0)->getDepthAttachment();
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
        {
            Material* pMaterial = (Material*)pAsset;
            if (pMaterial->receivesShadows())
                pMaterial->updateShadowmapDescriptorSet(pDepthAttachment);

            if (pMaterial->isTransparent())
                pMaterial->updateSceneDepthDescriptorSet(_pOpaqueFramebuffer->getDepthAttachment());
        }
    }

    void MasterRenderer::destroyOffscreenPassResources()
    {
        delete _pOpaqueFramebuffer;
        delete _pTransparentFramebuffer;
        _pOpaqueFramebuffer = nullptr;
        _pTransparentFramebuffer = nullptr;

        Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pColorAttachment);
        Application::get_instance()->getAssetManager()->destroyExternalPersistentAsset(_pDepthAttachment);
        _pColorAttachment = nullptr;
        _pDepthAttachment = nullptr;

        _shadowPassInstance.destroy();
    }

    void MasterRenderer::allocCommandBuffers(uint32_t count)
    {
        _primaryCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            count,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        );

        // TODO: Make all renderers alloc same way!
        _pRenderer3D->allocCommandBuffers();
        _pPostProcessingRenderer->allocCommandBuffers();
        _pGUIRenderer->allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        _pGUIRenderer->freeCommandBuffers();
        _pPostProcessingRenderer->freeCommandBuffers();
        _pRenderer3D->freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();

        _primaryCommandBuffers.clear();
    }

    void MasterRenderer::createPipelines()
    {
        const Extent2D swapchainExtent = _swapchainRef.getExtent();

        Debug::log("___TEST___MasterRenderer::createPipelines creating post processing pipeline");
        _pPostProcessingRenderer->createPipelines(_swapchainRef.getRenderPass());

        Debug::log("___TEST___MasterRenderer::createPipelines creating GUIRenderer pipeline");
        _pGUIRenderer->createPipeline(
            _swapchainRef.getRenderPass(),
            swapchainExtent.width,
            swapchainExtent.height
        );

        Debug::log("___TEST___MasterRenderer::createPipelines creating Material pipelines");

        // NOTE: Materials' pipelines gets initially created by Batcher.
        // If swapchain recreation occurs this makes the Materials to also recreate
        // their pipelines according to the current situation
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->recreateExistingPipeline();
    }

    void MasterRenderer::destroyPipelines()
    {
        _pPostProcessingRenderer->destroyPipelines();
        _pGUIRenderer->destroyPipeline();

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->destroyPipeline();
    }

    void MasterRenderer::createShaderResources()
    {
        Debug::log("___TEST___MasterRenderer::createShaderResources");
        // NOTE: ATM JUST TESTING HERE!
        _pPostProcessingRenderer->createShaderResources(_pColorAttachment);

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        for (Asset* pAsset : pAssetManager->getAssets(AssetType::ASSET_TYPE_MATERIAL))
            ((Material*)pAsset)->createShaderResources();
    }

    void MasterRenderer::destroyShaderResources()
    {
        Debug::log("___TEST___MasterRenderer::destroyShaderResources");
        _pPostProcessingRenderer->destroyShaderResources();

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

        const Vector3f sceneAmbientLight = pScene->environmentProperties.ambientColor;
        _scene3DData.ambientLightColor = {
            sceneAmbientLight.r,
            sceneAmbientLight.g,
            sceneAmbientLight.b,
            1.0f
        };

        // NOTE: Consider all light data of all scene lights inside a single
        // descriptor set?
        const Light* pDirectionalLight = (const Light*)pScene->getComponent(
            ComponentType::COMPONENT_TYPE_LIGHT,
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
        _scene3DData.shadowProperties = {
            (float)_shadowmapWidth,
            2.0f, // pcf sample radius
            0.9f, // shadow strength
            0.0f // undetermined
        };
        _scene3DData.time += 1.0f * Timing::get_delta_time();

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



        // TESTING SHADOW PASS -----------------------------------
        // Make sure, initially using correct img layout
        Framebuffer* pShadowFramebuffer = _shadowPassInstance.getFramebuffer(0);
        transition_image_layout(
            currentCommandBuffer,
            pShadowFramebuffer->getDepthAttachment(),
            ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // new layout
            PipelineStage::LATE_FRAGMENT_TESTS_BIT, // src stage
            MemoryAccessFlagBits::MEMORY_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // src access mask
            PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
            MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
        );

        render::begin_render_pass(
            currentCommandBuffer,
            _shadowPassInstance.getRenderPass(),
            pShadowFramebuffer,
            { 1, 0, 1, 1 }
        );
        std::vector<CommandBuffer> shadowpassCommandBuffers;
        shadowpassCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _shadowPassInstance.getRenderPass(),
                (float)pShadowFramebuffer->getWidth(),
                (float)pShadowFramebuffer->getHeight(),
                _batcher.getBatches(RenderPassType::SHADOW_PASS)
            )
        );
        render::exec_secondary_command_buffers(currentCommandBuffer, shadowpassCommandBuffers);
        render::end_render_pass(currentCommandBuffer, _shadowPassInstance.getRenderPass());

        // Set shadowmap into correct format for opaque pass to sample
        transition_image_layout(
            currentCommandBuffer,
            pShadowFramebuffer->getDepthAttachment(),
            ImageLayout::SHADER_READ_ONLY_OPTIMAL, // new layout
            PipelineStage::LATE_FRAGMENT_TESTS_BIT, // src stage
            MemoryAccessFlagBits::MEMORY_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // src access mask
            PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
            MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
        );
        // TESTING END ^^^ -------------------------------------------

        // TESTING OPAQUE PASS -----------------------------------
        render::begin_render_pass(
            currentCommandBuffer,
            _opaquePass,
            _pOpaqueFramebuffer,
            { 1, 0, 1, 1 }
        );
        std::vector<CommandBuffer> opaquePassCommandBuffers;
        opaquePassCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _opaquePass,
                (float)_pOpaqueFramebuffer->getWidth(),
                (float)_pOpaqueFramebuffer->getHeight(),
                _batcher.getBatches(RenderPassType::OPAQUE_PASS)
            )
        );
        render::exec_secondary_command_buffers(currentCommandBuffer, opaquePassCommandBuffers);
        render::end_render_pass(currentCommandBuffer, _opaquePass);

        // Transition the opaque pass' depthmap to samplable for transparent pass
        transition_image_layout(
            currentCommandBuffer,
            _pTransparentFramebuffer->getDepthAttachment(),
            ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // new layout
            PipelineStage::LATE_FRAGMENT_TESTS_BIT, // src stage
            MemoryAccessFlagBits::MEMORY_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // src access mask
            PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
            MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
        );
        // TESTING END ^^^ -------------------------------------------

        // TESTING TRANSPARENT PASS -----------------------------------
        render::begin_render_pass(
            currentCommandBuffer,
            _transparentPass,
            _pTransparentFramebuffer,
            { 1, 0, 1, 1 }
        );
        std::vector<CommandBuffer> transparentPassCommandBuffers;
        transparentPassCommandBuffers.push_back(
            _pRenderer3D->recordCommandBuffer(
                _transparentPass,
                (float)_pTransparentFramebuffer->getWidth(),
                (float)_pTransparentFramebuffer->getHeight(),
                _batcher.getBatches(RenderPassType::TRANSPARENT_PASS)
            )
        );
        render::exec_secondary_command_buffers(currentCommandBuffer, transparentPassCommandBuffers);
        render::end_render_pass(currentCommandBuffer, _transparentPass);
        // TESTING END ^^^ -------------------------------------------

        // Transition color attachment samplable for the post processing pass
        transition_image_layout(
            currentCommandBuffer,
            _pTransparentFramebuffer->getColorAttachments()[0],
            ImageLayout::SHADER_READ_ONLY_OPTIMAL, // new layout
            PipelineStage::COLOR_ATTACHMENT_OUTPUT_BIT, // src stage
            MemoryAccessFlagBits::MEMORY_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // src access mask
            PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
            MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
        );

        // NOTE: Not sure if should transition the transparent depth image here into something else?

        //Framebuffer* pCurrentSwapchainFramebuffer = _swapchainRef.getCurrentFramebuffer();
        //render::begin_render_pass(
        //    currentCommandBuffer,
        //    _swapchainRef.getRenderPass(),
        //    pCurrentSwapchainFramebuffer,
        //    pScene->environmentProperties.clearColor
        //);

        // TODO: Post processing screen pass instead of below

        // NOTE: We create new copies of secondary command buffers here
        // TODO: Figure out some nice way to optimize this!
        //std::vector<CommandBuffer> postProcessColorCommandBuffers;
        //secondaryCommandBuffers.push_back(
        //    _pPostProcessingRenderer->recordCommandBuffer(
        //        _swapchainRef.getRenderPass(),
        //        (float)swapchainExtent.width,
        //        (float)swapchainExtent.height,
        //        _currentFrame
        //    )
        //);


        std::vector<CommandBuffer> screenPassCommandBuffers;
        screenPassCommandBuffers.push_back(
            _pPostProcessingRenderer->recordCommandBuffer(
                currentCommandBuffer,
                _swapchainRef.getCurrentFramebuffer(),
                (float)swapchainExtent.width,
                (float)swapchainExtent.height,
                _currentFrame
            )
        );

        // NOTE: Need to reset batches for next frame's submits
        //      -> Otherwise adding endlessly
        _batcher.resetForNextFrame();

        _pRenderer3D->advanceFrame();

        screenPassCommandBuffers.push_back(
            _pGUIRenderer->recordCommandBuffer(
                _swapchainRef.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                orthographicProjectionMatrix,
                _currentFrame
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, screenPassCommandBuffers);
        render::end_render_pass(currentCommandBuffer, _swapchainRef.getRenderPass());

        currentCommandBuffer.end();

        size_t maxFramesInFlight = _swapchainRef.getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    void MasterRenderer::handleWindowResize()
    {
        Debug::log("___TEST___WINDOW RESIZE!");
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
                destroyOffscreenPassResources();
                createOffscreenPassResources();

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
                _pGUIRenderer->freeBatches();


                _pPostProcessingRenderer->destroyShaderResources();
                _pPostProcessingRenderer->destroyPipelines();
                _pPostProcessingRenderer->destroyFramebuffers();

                _pPostProcessingRenderer->createFramebuffers();
                _pPostProcessingRenderer->createShaderResources(_pColorAttachment);
                _pPostProcessingRenderer->createPipelines(_swapchainRef.getRenderPass());
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

                destroyOffscreenPassResources();
                createOffscreenPassResources();

                _batcher.recreateManagedPipelines();

                _pPostProcessingRenderer->destroyShaderResources();
                _pPostProcessingRenderer->destroyPipelines();
                _pPostProcessingRenderer->destroyFramebuffers();

                _pPostProcessingRenderer->createFramebuffers();
                _pPostProcessingRenderer->createShaderResources(_pColorAttachment);
                _pPostProcessingRenderer->createPipelines(_swapchainRef.getRenderPass());
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
