#include "platypus/graphics/Shader.h"
#include "WebShader.h"
#include "platypus/core/Debug.h"
#include "platypus/utils/FileUtils.h"

#include <unordered_map>
#include <set>
#include <sstream>
#include <GL/glew.h>


namespace platypus
{
    static std::set<std::string> s_ESSLBasicTypes = {
        "bool",
        "int",
        "uint",
        "float",
        "double",
        "ivec2",
        "ivec3",
        "ivec4",
        "vec2",
        "vec3",
        "vec4",
        "mat4",
        "sampler2D"
    };


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


    Shader::Shader(const std::string& filename, ShaderStageFlagBits stage)
    {
        const std::string fullPath = "assets/shaders/web/" + filename + ".glsl";
        std::string source = load_text_file(fullPath);

        GLenum stageType = to_gl_shader(stage);
        uint32_t id = glCreateShader(stageType);

        if (id == 0)
        {
            Debug::log(
                "@Shader::Shader "
                "Failed to create opengl shader for stage: " + std::to_string(stage),
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


    OpenglShaderProgram::OpenglShaderProgram(
        ShaderVersion shaderVersion,
        const ShaderImpl* pVertexShader,
        const ShaderImpl* pFragmentShader
    )
    {
        _pVertexShader = pVertexShader;
        _pFragmentShader = pFragmentShader;

        _id = glCreateProgram();
        const uint32_t vertexShaderID = _pVertexShader->id;
        const uint32_t fragmentShaderID = _pFragmentShader->id;

        if (vertexShaderID && fragmentShaderID && _id)
        {
            glAttachShader(_id, vertexShaderID);
            glAttachShader(_id, fragmentShaderID);

            glLinkProgram(_id);

            GLint linkStatus = 0;
            glGetProgramiv(_id, GL_LINK_STATUS, &linkStatus);
            if (!linkStatus)
            {
                GLint infoLogLength = 0;
                glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &infoLogLength);

                if (infoLogLength > 0)
                {
                    char* infoLog = new char[infoLogLength];

                    glGetProgramInfoLog(_id, infoLogLength, &infoLogLength, infoLog);

                    std::string info_str(infoLog);

                    Debug::log(
                        "@OpenglShaderProgram::OpenglShaderProgram "
                        "Failed to link OpenglShader\nInfoLog:\n" + info_str,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    delete[] infoLog;
                }
            }
            else
            {
                glValidateProgram(_id);
                Debug::log("OpenglShader created successfully");
                // NOTE: Below not tested!! May not work!
                if (shaderVersion == ShaderVersion::ESSL1)
                {
                    Debug::log(
                        "\t->Version was ESSL1. Attempting to get attrib and uniform locations..."
                    );
                    findLocationsESSL1(_pVertexShader->source);
                    findLocationsESSL1(_pFragmentShader->source);
                }
            }
        }
        else
        {
            Debug::log(
                "@OpenglShaderProgram::OpenglShaderProgram "
                "Failed to create OpenglShader object",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    OpenglShaderProgram:: ~OpenglShaderProgram()
    {
        glUseProgram(0);
        glDetachShader(_id, _pVertexShader->id);
        glDetachShader(_id, _pFragmentShader->id);
        // NOTE: Not sure if vertex and fragment shaders need deleting before this?
        // ..those are detached so you'd think thats not required..
        // ...but never fucking know what happens with opengl...
        glDeleteProgram(_id);
    }

    // NOTE: Works only with Opengl ES Shading language v1
    //  * NOT TESTED, MAY NOT WORK!
    //  * Doesn't work properly if source contains /**/ kind of comments!
    // Supposed to be used to automatically get locations of attribs and uniforms
    // NOTE: why source not in as ref?
    void OpenglShaderProgram::findLocationsESSL1(const std::string shaderSource)
    {
        std::string line = "";
        std::istringstream in(shaderSource);

        std::unordered_map<std::string, std::vector<std::string>> structList;
        std::unordered_map<std::string, std::string> constVariables;
        bool recordStruct = false;
        std::string recordStructName = "";

        while (getline(in, line))
        {
            std::vector<std::string> components;
            size_t nextDelim = 0;
            // remove ';' completely, we dont need that here for anythin..
            size_t endpos = line.find(";");
            if (endpos != std::string::npos)
                line.erase(endpos, 1);

            while ((nextDelim = line.find(" ")) != std::string::npos)
            {
                std::string component = line.substr(0, nextDelim);
                if (component != " " && component != "\t" && component != "\0")
                    components.push_back(component);
                line.erase(0, nextDelim + 1);
            }
            components.push_back(line);

            if (components[0].substr(0, 2) == "//")
                continue;

            if (!recordStruct)
            {
                if (components.size() > 0)
                {
                    if (components[0] == "const")
                    {
                        if (components.size() > 4)
                            constVariables[components[2]] = components[4];
                    }
                }

                if (components.size() == 2)
                {
                    if (components[0] == "struct")
                    {
                        recordStructName = components[1];
                        recordStruct = true;
                    }
                }
                if (components.size() >= 3)
                {
                    if (components[0] == "attribute")
                    {
                        _attribLocations.push_back(glGetAttribLocation(_id, components[2].c_str()));
                    }
                    else if (components[0] == "uniform")
                    {
                        const std::string& type = components[1];
                        const std::string& uniformName = components[2];

                        // Check is this array
                        bool isArray = false;
                        std::string arrSizeStr = "";
                        std::string arrName = "";
                        size_t arrBegin = components[2].find("[");
                        if (arrBegin != std::string::npos)
                        {
                            size_t arrEnd = components[2].find("]");
                            arrName = components[2].substr(0, arrBegin);
                            arrSizeStr = components[2].substr(arrBegin + 1, (arrEnd - 1) - arrBegin);
                            // Currently we dont allow struct arrays, only "basic types"
                            if (s_ESSLBasicTypes.find(type) == s_ESSLBasicTypes.end())
                            {
                                Debug::log(
                                    "@OpenglShaderProgram::findLocationsESSL1 "
                                    "Encountered array uniform that was not of basic type. "
                                    "Currently array uniforms are required to be of basic type! "
                                    "uniform: " + uniformName + " "
                                    "type: " + type,
                                    Debug::MessageType::PLATYPUS_ERROR
                                );
                                continue;
                            }
                            isArray = true;
                        }

                        if (s_ESSLBasicTypes.find(type) != s_ESSLBasicTypes.end() && !isArray)
                        {
                            _uniformLocations.push_back(glGetUniformLocation(_id, components[2].c_str()));
                        }
                        else if (isArray)
                        {
                            int arrSize = 0;
                            if (constVariables.find(arrSizeStr) == constVariables.end())
                            {
                                arrSize = std::stoi(arrSizeStr);
                            }
                            else
                            {
                                arrSize = std::stoi(constVariables[arrSizeStr]);
                            }
                            if (arrSize <= 0)
                            {
                                Debug::log(
                                    "@OpenglShaderProgram::findLocationsESSL1 "
                                    "Encountered invalid array size: " + std::to_string(arrSize) + " "
                                    "Uniform: " + uniformName,
                                    Debug::MessageType::PLATYPUS_ERROR
                                );
                                continue;
                            }
                            for (int i = 0; i < arrSize; ++i)
                            {
                                std::string actualUniformName = arrName + "[" + std::to_string(i) + "]";
                                int location = glGetUniformLocation(_id, actualUniformName.c_str());
                                _uniformLocations.push_back(location);
                            }
                        }
                        else
                        {
                            // If uniform struct, need to find all like: "uniformstructname.values"
                            const std::string& structName = components[1];
                            if (structList.find(structName) != structList.end())
                            {
                                const std::string& uniformName = components[2];
                                for (std::string s : structList[structName])
                                {
                                    std::string finalUniformName = uniformName + "." + s;
                                    _uniformLocations.push_back(glGetUniformLocation(_id, finalUniformName.c_str()));
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (components.size() > 0)
                {
                    if (components[0] == "}")
                    {
                        recordStructName.clear();
                        recordStruct = false;
                    }
                    else if (components[0] != "{" && components.size() >= 2)
                    {
                        structList[recordStructName].push_back(components[1]);
                    }
                }
            }
        }
    }

    // TODO: delete commented out?
    //int32_t OpenglShaderProgram::getAttribLocation(const char* name) const
    //{
    //    return glGetAttribLocation(_id, name);
    //}

    //int32_t OpenglShaderProgram::getUniformLocation(const char* name) const
    //{
    //    return glGetUniformLocation(_id, name);
    //}

    void OpenglShaderProgram::setUniform(int location, const Matrix4f& matrix) const
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix.getRawArray());
    }

    void OpenglShaderProgram::setUniform(int location, const Vector3f& v) const
    {
        glUniform3f(location, v.x, v.y, v.z);
    }

    void OpenglShaderProgram::setUniform(int location, float val) const
    {
        glUniform1fv(location, 1, &val);
    }

    void OpenglShaderProgram::setUniform1i(int location, int val) const
    {
        glUniform1i(location, val);
    }
}
