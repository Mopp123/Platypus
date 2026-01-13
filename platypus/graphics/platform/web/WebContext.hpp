#pragma once

#include <emscripten/html5.h>
#include "platypus/graphics/Buffers.hpp"
#include "platypus/core/Debug.hpp"
#include <unordered_map>
#include <set>
#include <GL/glew.h>

#ifdef PLATYPUS_DEBUG
    #define GL_FUNC(func)	func;																				\
        switch (glGetError()){																						\
            case GL_NO_ERROR:break;																						\
            case GL_INVALID_ENUM:					Debug::log("Opengl Error: GL_INVALID_ENUM", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;					\
            case GL_INVALID_VALUE:					Debug::log("Opengl Error: GL_INVALID_VALUE", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;					\
            case GL_INVALID_OPERATION:				Debug::log("Opengl Error: GL_INVALID_OPERATION", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;				\
            case GL_INVALID_FRAMEBUFFER_OPERATION:	Debug::log("Opengl Error: GL_INVALID_FRAMEBUFFER_OPERATION", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;	\
            case GL_OUT_OF_MEMORY:					Debug::log("Opengl Error: GL_OUT_OF_MEMORY", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;					\
            case GL_STACK_UNDERFLOW:				Debug::log("Opengl Error: GL_STACK_UNDERFLOW", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;				\
            case GL_STACK_OVERFLOW:					Debug::log("Opengl Error: GL_STACK_OVERFLOW", Debug::MessageType::PLATYPUS_ERROR);PLATYPUS_ASSERT(false);break;				\
        default: break;}
#else
    #define GL_FUNC(func)	func;
#endif


namespace platypus
{
    unsigned int to_gl_datatype(ShaderDataType shaderDataType);
    std::string gl_error_to_string(unsigned int error);

    struct VAOData
    {
        // NOTE: WARNING! bufferIDs and complementaryBufferIDs should rather be std::vectors
        // since the order should matter when attempting to find suitable VAO!
        std::set<uint32_t> bufferIDs;
        // Sometimes "complementary buffers" like per instance buffers are required
        // which aren't part of any actual mesh, so need to have these here separately.A
        // (These buffers may have longer lifetime than mesh vertex buffers, so these won't
        // affect the VAO's lifetime)
        std::set<uint32_t> complementaryBufferIDs;
        std::vector<VertexBufferLayout> bufferLayouts;
    };

    struct ContextImpl
    {
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webglContext;

        // key = vaoID
        //
        // EXPLANATION:
        //      When binding vertex buffers VAO is searched that includes all the
        //      vertex buffers and layouts.
        //          -> If VAO is found, this gets bound.
        //          -> If VAO not found, a new one gets created
        //      When vertex buffer gets deleted, it gets removed from the VAOData.
        //      When all the VAO's vertex buffers are deleted, the VAO is deleted
        //      (through Buffer's destructor).
        //      NOTE: WARNING! Possible issue, if trying to use the VAO after a single
        //      vbo gets deleted -> the VAO becomes invalid!
        std::unordered_map<uint32_t, VAOData> vaoDataMapping;
    };

    bool vao_deletion_allowed(ContextImpl* pContextImpl, uint32_t vaoID);
}
