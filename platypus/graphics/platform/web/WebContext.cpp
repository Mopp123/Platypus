#include "platypus/graphics/Context.hpp"
#include "WebContext.hpp"
#include "platypus/core/Debug.hpp"
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


    bool vao_deletion_allowed(ContextImpl* pContextImpl, uint32_t vaoID)
    {
        if (pContextImpl->vaoDataMapping[vaoID].bufferIDs.empty())
            return true;

        return true;
    }

    ContextImpl* Context::s_pImpl = nullptr;

    void Context::create(const char* appName, Window* pWindow)
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

        // NOTE: Also forcing webgl2 in CMakeLists?
        contextAttribs.majorVersion = 2;
        contextAttribs.minorVersion = 0;

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

        s_pImpl = new ContextImpl;
        s_pImpl->webglContext = webglContext;


        Debug::log("Context created successfully");
    }

    void Context::destroy()
    {
        if (s_pImpl)
        {
            emscripten_webgl_destroy_context(s_pImpl->webglContext);
            delete s_pImpl;
        }
    }

    ContextImpl* Context::get_impl()
    {
        if (!s_pImpl)
        {
            Debug::log(
                "@Context::get_impl "
                "s_pImpl was nullptr!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        return s_pImpl;
    }
}
