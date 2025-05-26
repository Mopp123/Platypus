#include "platypus/graphics/Context.h"
#include "WebContext.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Buffers.h"

#include <emscripten.h>
#include <emscripten/html5.h>
// *glew is here also through emscripten
#include <GL/glew.h>


namespace platypus
{
    unsigned int to_gl_datatype(ShaderDataType shaderDataType)
    {
        switch (shaderDataType)
        {
            case ShaderDataType::None:
                Debug::log(
                    "@to_gl_data_type Invalid shaderDataType <NONE>",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return 0;

            case ShaderDataType::Int:
                return GL_INT;
            case ShaderDataType::Int2:
                return GL_INT;
            case ShaderDataType::Int3:
                return GL_INT;
            case ShaderDataType::Int4:
                return GL_INT;

            case ShaderDataType::Float:
                return GL_FLOAT;
            case ShaderDataType::Float2:
                return GL_FLOAT;
            case ShaderDataType::Float3:
                return GL_FLOAT;
            case ShaderDataType::Float4:
                return GL_FLOAT;

            default:
                Debug::log(
                    "@to_gl_data_type Invalid shaderDataType",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return 0;
        }
    }


    std::string gl_error_to_string(unsigned int error)
    {
        switch (error)
        {
            case GL_INVALID_ENUM :                  return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE :                 return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION :             return "GL_INVALID_OPERATION";
            case GL_STACK_OVERFLOW :                return "GL_STACK_OVERFLOW";
            case GL_STACK_UNDERFLOW :               return "GL_STACK_UNDERFLOW";
            case GL_OUT_OF_MEMORY :                 return "GL_OUT_OF_MEMORY";
            case GL_INVALID_FRAMEBUFFER_OPERATION : return "GL_INVALID_FRAMEBUFFER_OPERATION";

            default:
                Debug::log(
                    "@gl_enum_to_string "
                    "Invalid erro GLenum: " + std::to_string(error)
                );
        }
        return "<not found>";
    }


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

        // Create a single vao and bind immediately to use for everything on opengl side
        uint32_t vaoID = 0;
        GL_FUNC(glGenVertexArrays(1, &vaoID));
        GL_FUNC(glBindVertexArray(vaoID));

        _pImpl = new ContextImpl;
        _pImpl->vaoID = vaoID;

        s_pInstance = this;

        Debug::log("Context created successfully");
    }

    Context::~Context()
    {
        if (_pImpl)
        {
            GL_FUNC(glDeleteVertexArrays(1, &_pImpl->vaoID));
            delete _pImpl;
        }
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
