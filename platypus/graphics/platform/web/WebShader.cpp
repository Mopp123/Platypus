#include "platypus/graphics/Shader.h"

namespace platypus
{
    unsigned int to_gl_shader(ShaderStageFlagBits stage)
    {
        switch (stage)
        {
            case ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT:
                return GL_VERTEX_SHADER;
            case ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT:
                return GL_FRAGMENT_SHADER;
            default:
                Debug::log(
                    "@to_gl_shader "
                    "Invalid stage: " + std::to_string((uint32_t)stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return 0;
        }
    }


    Shader::Shader(const std::string& filepath, ShaderStageFlagBits stage)
    {
        GLenum stageType = to_gl_shader(stage);
        uint32_t id = glCreateShader(stageType);

        if (id == 0)
        {
            Debug::log(
                "@Shader::Shader "
                "Failed to create opengl shader for stage: " + std::to_string(stage)
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        const char* srcCStr = source.c_str();
        glShaderSource(id, 1, &srcCStr, NULL);

        glCompileShader(id);

        GLint compileStatus = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);

        if (!compileStatus)
        {
            GLint infoLogLength = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

            if (infoLogLength > 0)
            {
                char* infoLog = new char[infoLogLength];

                glGetShaderInfoLog(id, infoLogLength, &infoLogLength, infoLog);

                Debug::log(
                    "@Shader::Shader "
                    "Failed to compile shader stage(" + std::to_string(stage) + ")\n"
                    "InfoLog:\n" + infoLog,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                delete[] infoLog;
            }
            glDeleteShader(id);
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new ShaderImpl;
        _pImpl->source = source;
        _pImpl->id = id;
        Debug::log("___TEST___Shader created(stage: " + std::to_string(stage) + ")");
    }

    Shader::~Shader()
    {
        if (_pImpl)
        {
            // NOTE: DANGER! You need to unbind current shader
            // and detach this module from its' "program"
            // before destructing this!
            // (OpenglShaderProgram is responsible for detaching in its destructor)
            glDeleteShader(_pImpl->id);
            delete _pImpl;
        }
    }
}
