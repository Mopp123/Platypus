#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Shader.h"


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


namespace platypus
{
    unsigned int to_gl_data_type(ShaderDataType shaderDataType);
    std::string gl_error_to_string(unsigned int error);


    struct ContextImpl
    {
        uint32_t vaoID;
    };
}
