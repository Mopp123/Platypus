#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Shader.h"
#include "platypus/core/Debug.h"
#include <unordered_map>
#include <set>

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

    bool vao_deletion_allowed(ContextImpl* pContextImpl, uint32_t vaoID);

    struct ContextImpl
    {
        // key = vaoID, value = bufferIDs of that vao
        std::unordered_map<uint32_t, std::set<uint32_t>> vaoBufferMapping;

        // Some renderer's have their own "complementary buffers" like per instance buffers
        // which aren't part of any actual mesh, so need to detect these in order to
        // destroy VAOs correctly!
        std::set<uint32_t> complementaryVbos;
    };
}
