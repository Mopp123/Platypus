#include "platypus/graphics/Context.h"
#include "platypus/core/Debug.h"

#include <emscripten.h>
#include <emscripten/html5.h>
// *glew is here also through emscripten
#include <GL/glew.h>


namespace platypus
{
    struct ContextImpl
    {
    };

    Context* Context::s_pInstance = nullptr;
    Context::Context(const char* appName, Window* pWindow)
    {
        EmscriptenWebGLContextAttributes contextAttribs;
        emscripten_webgl_init_context_attributes(&contextAttribs);
        contextAttribs.alpha = false;
        contextAttribs.depth = true;
        contextAttribs.stencil = false;
        contextAttribs.antialias = true;
        contextAttribs.premultipliedAlpha = true;
        contextAttribs.preserveDrawingBuffer = false;

        contextAttribs.failIfMajorPerformanceCaveat = false;

        contextAttribs.enableExtensionsByDefault = 1;

        // using webgl 1.0 (widest support?)
        // NOTE: UPDATE: This is outdated since forcing webgl2 in CMakeLists?
        contextAttribs.majorVersion = 1;
        contextAttribs.majorVersion = 0;

        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webglContext = emscripten_webgl_create_context("#canvas", &contextAttribs);
        if (webglContext >= 0)
        {
            emscripten_webgl_make_context_current(webglContext);

            GLenum glewInitStatus = glewInit();
            // TODO: Query some limits using glGetIntegerv
            if (glewInitStatus != GLEW_OK)
            {
                Debug::log(
                    "@Context::Context "
                    "Failed to init glew",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        else
        {
            Debug::log(
                "@Context::Context "
                "Failed to create WebGL context",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        Debug::log("Context created successfully");

        _pImpl = new ContextImpl;
    }

    Context::~Context()
    {
        if (_pImpl)
            delete _pImpl;
    }

    void Context::submitPrimaryCommandBuffer(Swapchain& swapchain, const CommandBuffer& cmdBuf, size_t frame)
    {
    }

    void Context::waitForOperations()
    {
    }

    void Context::handleWindowResize()
    {}

    Context* Context::get_instance()
    {
        if (!s_pInstance)
        {
            Debug::log(
                "@Context::get_instance "
                "Context instance was nullptr! "
                "Make sure Context has been created successfully before accessing it. "
                "Context should be created on Application construction.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return s_pInstance;
    }
}
