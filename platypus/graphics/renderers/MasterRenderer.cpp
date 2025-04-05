#include "MasterRenderer.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/components/Camera.h"
#include "platypus/ecs/components/Component.h"


namespace platypus
{
    MasterRenderer::MasterRenderer(
        const Window& window
    ) :
        _swapchain(window),
        _descriptorPool(_swapchain),
        _testRenderer(*this, _swapchain, _commandPool, _descriptorPool),
        _dirLightDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    {
                        { 3, ShaderDataType::Float4 },
                        { 4, ShaderDataType::Float4 }
                    }
                }
            }
        )
    {
        // Create common uniform buffers and descriptor sets
        DirLightUniformBufferData dirLightUniformBufferData;

        for (int i = 0; i < _swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pDirLightUniformBuffer = new Buffer(
                _commandPool,
                &dirLightUniformBufferData,
                sizeof(DirLightUniformBufferData),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
            );
            _dirLightUniformBuffer.push_back(pDirLightUniformBuffer);

            _dirLightDescriptorSets.push_back(
                _descriptorPool.createDescriptorSet(
                    &_dirLightDescriptorSetLayout,
                    { pDirLightUniformBuffer }
                )
            );
        }

        // NOTE: May fuckup?
        allocCommandBuffers(_swapchain.getMaxFramesInFlight());
        createPipelines();
    }

    MasterRenderer::~MasterRenderer()
    {
        for (Buffer* pUniformBuffer : _dirLightUniformBuffer)
            delete pUniformBuffer;
        _dirLightUniformBuffer.clear();

        _dirLightDescriptorSetLayout.destroy();
    }

    void MasterRenderer::cleanUp()
    {
        destroyPipelines();
        freeCommandBuffers();
    }

    void MasterRenderer::submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix)
    {
        _testRenderer.submit(pRenderable, transformationMatrix);
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
            Context::get_instance()->submitPrimaryCommandBuffer(
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
        _primaryCommandBuffers = _commandPool.allocCommandBuffers(
            count,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        );
        Debug::log("___TEST___ALLOCATED " + std::to_string(_primaryCommandBuffers.size()) + " PRIMARY CMD BUFS");
        _testRenderer.allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        _testRenderer.freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();
        _primaryCommandBuffers.clear();
    }

    void MasterRenderer::createPipelines()
    {
        const Extent2D swapchainExtent = _swapchain.getExtent();
        _testRenderer.createPipeline(
            _swapchain.getRenderPass(),
            swapchainExtent.width,
            swapchainExtent.height,
            _dirLightDescriptorSetLayout
        );
    }

    void MasterRenderer::destroyPipelines()
    {
        _testRenderer.destroyPipeline();
    }

    const CommandBuffer& MasterRenderer::recordCommandBuffer()
    {
        size_t frame = _swapchain.getCurrentFrame();

        if (frame >= _primaryCommandBuffers.size())
        {
            Debug::log(
                "@MasterRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(frame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_primaryCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _primaryCommandBuffers[frame];

        currentCommandBuffer.begin(_swapchain.getRenderPass());
        render::begin_render_pass(currentCommandBuffer, _swapchain, { 0, 0, 1, 1 }, true);

        // NOTE: We create new copies of secondary command buffers here!
        // Fucking stupid, since the actual command buffers we are using/refering to lives inside the
        // renderer producing the secondary command buffer.
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;

        // TESTING projection matrices
        /*
        Matrix4f projectionMatrix = create_orthographic_projection_matrix(
            -1.0f,
            1.0f,
            -1.0f,
            1.0f,
            0.1f,
            1000.0f
        );*/

        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f viewMatrix = Matrix4f(1.0f);

        Application* pApp = Application::get_instance();
        SceneManager& sceneManager = pApp->getSceneManager();
        Scene* pScene = sceneManager.accessCurrentScene();

        Camera* pCamera = (Camera*)pScene->getComponent(ComponentType::COMPONENT_TYPE_CAMERA);
        Transform* pCameraTransform = (Transform*)pScene->getComponent(ComponentType::COMPONENT_TYPE_TRANSFORM);
        if (pCamera)
            perspectiveProjectionMatrix = pCamera->perspectiveProjectionMatrix;
        if (pCameraTransform)
            viewMatrix = pCameraTransform->globalMatrix.inverse();

        const DirectionalLight* pDirectionalLight = (const DirectionalLight*)pScene->getComponent(ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT);
        if (!pDirectionalLight)
        {
            _useDirLightData = { { 0, 0, 0, 1.0f }, { 0, 0, 0, 1.0f } };
        }
        else
        {
            _useDirLightData.direction = {
                pDirectionalLight->direction.x,
                pDirectionalLight->direction.y,
                pDirectionalLight->direction.z,
                1.0f
            };
            _useDirLightData.color = {
                pDirectionalLight->color.r,
                pDirectionalLight->color.g,
                pDirectionalLight->color.b,
                1.0f
            };
        }
        _dirLightUniformBuffer[frame]->update(
            &_useDirLightData,
            sizeof(DirLightUniformBufferData)
        );

        const Extent2D swapchainExtent = _swapchain.getExtent();
        secondaryCommandBuffers.push_back(
            _testRenderer.recordCommandBuffer(
                _swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                perspectiveProjectionMatrix,
                viewMatrix,
                _dirLightDescriptorSets[frame],
                frame // NOTE: no idea should this be the "frame index" or "image index"
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    void MasterRenderer::handleWindowResize()
    {
        Context* pContext = Context::get_instance();
        Application* pApp = Application::get_instance();
        pContext->waitForOperations();

        Window& window = pApp->getWindow();
        if (!window.isMinimized())
        {
            pContext->handleWindowResize();
            _swapchain.recreate(window);
            cleanUp();
            //allocCommandBuffers(_swapchain.getImageCount()); // NOTE: Isn't this supposed to be frames in flight count?
            allocCommandBuffers(_swapchain.getMaxFramesInFlight()); // Updated to test this...
            createPipelines();
            window.resetResized();
        }
        else
        {
            pApp->getInputManager().waitEvents();
        }
    }
}
