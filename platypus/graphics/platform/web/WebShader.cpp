#include "platypus/graphics/Shader.h"
#include "WebShader.hpp"
#include "platypus/core/Debug.h"
#include "platypus/utils/FileUtils.h"
#include "platypus/utils/StringUtils.hpp"

#include <set>
#include <sstream>
#include <GL/glew.h>


namespace platypus
{
    // NOTE: Some of the engine side types are None since currently we don't care these
    // on the engine side
    static std::unordered_map<std::string, ShaderDataType> s_ESSLTypesToEngine = {
        { "bool",       ShaderDataType::None  },
        { "int",        ShaderDataType::Int   },
        { "uint",       ShaderDataType::None  },
        { "float",      ShaderDataType::Float },
        { "double",     ShaderDataType::None  },
        { "ivec2",      ShaderDataType::Int2  },
        { "ivec3",      ShaderDataType::Int3  },
        { "ivec4",      ShaderDataType::Int4  },
        { "vec2",       ShaderDataType::Float2 },
        { "vec3",       ShaderDataType::Float3 },
        { "vec4",       ShaderDataType::Float4 },
        { "mat4",       ShaderDataType::Mat4 },
        { "sampler2D",  ShaderDataType::None }
    };

    static std::set<std::string> s_allowedBlockLayouts = {
        "std140"
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


    Shader::Shader(const std::string& filename, ShaderStageFlagBits stage) :
        _stage(stage),
        _filename(filename)
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
                    "Failed to compile shader stage: " + shader_stage_to_string(stage) + " "
                    "using file: " + fullPath + "\n"
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
                if (shaderVersion == ShaderVersion::ESSL3)
                {
                    Debug::log(
                        "\t->Version was ESSL1. Attempting to get attrib and uniform locations..."
                    );
                    findLocationsESSL3(ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT, _pVertexShader->source);
                    findLocationsESSL3(ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT, _pFragmentShader->source);

                    Debug::log("___TEST___block indices:");
                    for (int32_t i : _uniformBlockIndices)
                        Debug::log("    " + std::to_string(i));
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

    OpenglShaderProgram::~OpenglShaderProgram()
    {
        glUseProgram(0);
        glDetachShader(_id, _pVertexShader->id);
        glDetachShader(_id, _pFragmentShader->id);
        // NOTE: Not sure if vertex and fragment shaders need deleting before this?
        // ..those are detached so you'd think thats not required..
        // ...but never fucking know what happens with opengl...
        glDeleteProgram(_id);
    }

    bool OpenglShaderProgram::isConstantVar(const std::vector<std::string>& lineComponents)
    {
        if (lineComponents.empty())
            return false;

        return lineComponents[0] == "const" && lineComponents.size() > 4;
    }

    bool OpenglShaderProgram::isStruct(const std::vector<std::string>& lineComponents)
    {
        if (lineComponents.size() != 2)
            return false;

        return lineComponents[0] == "struct";
    }

    // TODO: Allow also writing like: "layout ( location = x ) ", etc and any possible formatted way
    bool OpenglShaderProgram::isVertexAttrib(
        ShaderStageFlagBits shaderStage,
        const std::vector<std::string>& lineComponents
    )
    {
        if (lineComponents.size() != 6 || shaderStage != ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
            return false;

        return lineComponents[0] == "layout(location";
    }

    bool OpenglShaderProgram::isUniform(const std::vector<std::string>& lineComponents)
    {
        if (lineComponents.size() < 3)
            return false;

        return lineComponents[0] == "uniform";
    }

    bool OpenglShaderProgram::isUniformBlock(const std::string& line)
    {
        size_t layoutBegin = line.find("layout");
        size_t uniformBegin = line.find("uniform");
        return layoutBegin != line.npos && uniformBegin != line.npos;
    }

    void OpenglShaderProgram::addAttribute(
        int lineNumber,
        const std::vector<std::string>& lineComponents
    )
    {
        std::unordered_map<std::string, ShaderDataType>::const_iterator it = s_ESSLTypesToEngine.find(lineComponents[4]);
        if (it == s_ESSLTypesToEngine.end())
        {
            throw ParseError(
                lineNumber,
                lineComponents,
                "Invalid vertex attrib type: " + lineComponents[4]
            );
        }
        int location = -1;
        const std::string& attribLocationStr = lineComponents[2];
        try
        {
            location = std::stoi(attribLocationStr);
        }
        catch (std::out_of_range& ex)
        {
            throw ParseError(
                lineNumber,
                lineComponents,
                "Failed to convert attrib location: " + attribLocationStr + " to int(out_of_range exception)"
            );
        }
        catch (std::invalid_argument& ex)
        {
            throw ParseError(
                lineNumber,
                lineComponents,
                "Failed to convert attrib location: " + attribLocationStr + " to int(invalid_argument exception)"
            );
        }

        _attributes[location] = it->second;
    }


    void OpenglShaderProgram::addUniform(
        int lineNumber,
        const std::vector<std::string>& lineComponents,
        const std::unordered_map<std::string, std::vector<std::string>>& structList,
        const std::unordered_map<std::string, std::string>& constVariables
    )
    {
        const std::string& type = lineComponents[1];
        const std::string& uniformName = lineComponents[2];

        // Check is this array
        bool isArray = false;
        std::string arrSizeStr = "";
        std::string arrName = "";
        size_t arrBegin = lineComponents[2].find("[");
        if (arrBegin != std::string::npos)
        {
            size_t arrEnd = lineComponents[2].find("]");
            arrName = lineComponents[2].substr(0, arrBegin);
            arrSizeStr = lineComponents[2].substr(arrBegin + 1, (arrEnd - 1) - arrBegin);
            // Currently we dont allow struct arrays, only "basic types"
            if (s_ESSLTypesToEngine.find(type) == s_ESSLTypesToEngine.end())
            {
                throw ParseError(
                    lineNumber,
                    lineComponents,
                    "Encountered array uniform that was not of basic type. "
                    "Currently array uniforms are required to be of basic type! "
                    "uniform: " + uniformName + " "
                    "type: " + type
                );
            }
            isArray = true;
        }

        if (s_ESSLTypesToEngine.find(type) != s_ESSLTypesToEngine.end() && !isArray)
        {
            // NOTE: Shouldn't we use GL_FUNC macro here?
            int32_t location = glGetUniformLocation(_id, lineComponents[2].c_str());
            Debug::log("___TEST___Adding uniform: " + lineComponents[2] + " to index: " + std::to_string(_uniformLocations.size()) + " with location: " + std::to_string(location));
            _uniformLocations.push_back(location);
        }
        else if (isArray)
        {
            int arrSize = 0;
            std::unordered_map<std::string, std::string>::const_iterator constVarIt = constVariables.find(arrSizeStr);
            if (constVarIt == constVariables.end())
            {
                arrSize = std::stoi(arrSizeStr);
            }
            else
            {
                arrSize = std::stoi(constVarIt->second);
            }
            if (arrSize <= 0)
            {
                throw ParseError(
                    lineNumber,
                    lineComponents,
                    "Encountered invalid array size: " + std::to_string(arrSize) + " "
                    "Uniform: " + uniformName
                );
            }

            for (int i = 0; i < arrSize; ++i)
            {
                std::string actualUniformName = arrName + "[" + std::to_string(i) + "]";
                _uniformLocations.push_back(glGetUniformLocation(_id, actualUniformName.c_str()));
            }
        }
        else
        {
            // If uniform struct, need to find all like: "uniformstructname.values"
            const std::string& structName = lineComponents[1];
            std::unordered_map<std::string, std::vector<std::string>>::const_iterator structListIt = structList.find(structName);
            if (structListIt != structList.end())
            {
                const std::string& uniformName = lineComponents[2];
                const std::vector<std::string>& structMembers = structListIt->second;
                for (const std::string& structMember : structMembers)
                {
                    std::string finalUniformName = uniformName + "." + structMember;
                    _uniformLocations.push_back(glGetUniformLocation(_id, finalUniformName.c_str()));
                }
            }
        }
    }

    void OpenglShaderProgram::addUniformBlock(
        int lineNumber,
        const std::string& line
    )
    {
        const std::string uniformStr = "uniform";
        size_t uniformBegin = line.find(uniformStr);
        std::string blockName = line.substr(uniformBegin + uniformStr.size(), line.back());
        if (blockName.empty())
        {
            throw ParseError(
                lineNumber,
                line,
                "Couldn't find uniform block name from the line"
            );
        }

        trim_spaces(blockName);

        uint32_t blockIndex = glGetUniformBlockIndex(_id, blockName.c_str());
        if (blockIndex == GL_INVALID_INDEX)
        {
            throw ParseError(
                lineNumber,
                line,
                "Index for block '" + blockName + "' was GL_INVALID_INDEX"
            );
        }
        GLint blockSize = 0;
        // TODO: Maybe store these and make sure that the used buffer satisfies this?
        glGetActiveUniformBlockiv(_id, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
        Debug::log("___TEST___Block index: " + blockName + " = " + std::to_string(blockIndex) + " required size = " + std::to_string(blockSize));

        _uniformBlockIndices.push_back(blockIndex);
    }

    // NOTE: Works only with Opengl ES Shading language v1
    //  * NOT TESTED, MAY NOT WORK!
    //  * Doesn't work properly if source contains /**/ kind of comments!
    // Supposed to be used to automatically get locations of attribs and uniforms
    // NOTE: why source not in as ref?
    void OpenglShaderProgram::findLocationsESSL3(
        ShaderStageFlagBits shaderStage,
        const std::string shaderSource
    )
    {
        std::string line = "";
        std::istringstream in(shaderSource);

        std::unordered_map<std::string, std::vector<std::string>> structList;
        std::unordered_map<std::string, std::string> constVariables;
        bool recordStruct = false;
        std::string recordStructName = "";
        int lineNumber = 1;

        try
        {
            while (getline(in, line))
            {
                std::string fullLine = line;
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
                {
                    ++lineNumber;
                    continue;
                }

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
                        if (isVertexAttrib(shaderStage, components))
                        {
                            addAttribute(lineNumber, components);
                        }
                        else if (isUniform(components))
                        {
                            // May add multiple locations for a single uniform if using
                            // uniform structs (all struct members have their own locations)
                            addUniform(
                                lineNumber,
                                components,
                                structList,
                                constVariables
                            );
                        }
                        else if (isUniformBlock(fullLine))
                        {
                            addUniformBlock(lineNumber, fullLine);
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
                ++lineNumber;
            }
        }
        catch (ParseError& err)
        {
            Debug::log(
                "@OpenglShaderProgram::findLocationsESSL3 "
                "Parse error! Shader stage: " + shader_stage_to_string(shaderStage) + " "
                "ERROR: " + err.what(),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

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

    ShaderDataType OpenglShaderProgram::getAttributeType(int32_t location) const
    {
        std::unordered_map<int32_t, ShaderDataType>::const_iterator it = _attributes.find(location);
        if (it != _attributes.end())
            return it->second;

        return ShaderDataType::None;
    }

    int32_t OpenglShaderProgram::getUniformBlockIndex(size_t index)
    {
        if (index >= _uniformBlockIndices.size())
        {
            Debug::log(
                "@OpenglShaderProgram::getUniformBlockIndex "
                "Uniform block index(" + std::to_string(index) + ") "
                "out of bounds. Shader has " + std::to_string(_uniformBlockIndices.size()) + " "
                "uniform block indices.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return -1;
        }
        return _uniformBlockIndices[index];
    }

    void OpenglShaderProgram::printUniforms()
    {
        for (size_t i = 0; i < _uniformLocations.size(); ++i)
            Debug::log("[" + std::to_string(i) + "] = " + std::to_string(_uniformLocations[i]));
    }
}
