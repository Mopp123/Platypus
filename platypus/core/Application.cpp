#include "Application.h"
#include "platypus/graphics/Swapchain.h"
#include "Debug.h"

#include "platypus/graphics/Buffers.h"


namespace platypus
{
    Application* Application::s_pInstance = nullptr;

    static Buffer* s_pTestBuffer = nullptr;

    Application::Application(
        const std::string& name,
        int width,
        int height,
        bool resizable,
        bool fullscreen
    ) :
        _window(name, width, height, resizable, fullscreen),
        _inputManager(&_window),
        _context(name.c_str(), &_window),
        _swapchain(_window)
    {
        if (s_pInstance)
        {
            Debug::log(
                "@Application::Application "
                "Attempted to create Application but s_pInstance wasn't nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        s_pInstance = this;

        // TESTING!
        std::vector<float> buf = {
            1, 2, 3, 4
        };
        s_pTestBuffer = new Buffer(
            buf.data(),
            sizeof(float),
            buf.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float4 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };

        pTestVertexShader = new Shader("assets/shaders/TestVertexShader.spv", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT);
        pTestFragmentShader = new Shader("assets/shaders/TestFragmentShader.spv", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT);

        Extent2D swapchainExtent = _swapchain.getExtent();
        testPipeline.create(
            _swapchain.getRenderPass(),
            { vbLayout },
            //const std::vector<DescriptorSetLayout>& descriptorLayouts,
            *pTestVertexShader,
            *pTestFragmentShader,
            swapchainExtent.width,
            swapchainExtent.height,
            {
                 0, 0, swapchainExtent.width, swapchainExtent.height
            },
            CullMode::CULL_MODE_NONE,
            FRONT_FACE_COUNTER_CLOCKWISE,
            false, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
            false, // enable color blending
            0, // push constants size
            0 // push constants stage flags
        );

    }

    Application::~Application()
    {
    }

    void Application::run()
    {
        while (!_window.isCloseRequested())
        {
            _inputManager.pollEvents();
            /*
            _swapchain.acquireImage();
            CommandBuffer& cmdBuf = _masterRenderer.recordCommandBuffer();
            _context.submitPrimaryCommandBuffer(_swapchain, cmdBuf, frame);
            _swapchain.present(frame);
            */
        }

        testPipeline.destroy();
        delete s_pTestBuffer;
        delete pTestVertexShader;
        delete pTestFragmentShader;
    }

    Application* Application::get_instance()
    {
        if (!s_pInstance)
        {
            Debug::log(
                "@Application::get_instance "
                "s_pInstance was nullptr! Make sure you have created Application explicitly.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return s_pInstance;
    }
}
