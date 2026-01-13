#pragma once

#include "platypus/graphics/Shader.hpp"
#include "platypus/utils/Maths.h"
#include "platypus/graphics/Buffers.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>


namespace platypus
{
    unsigned int to_gl_shader(ShaderStageFlagBits stage);
    ShaderDataType to_engine_shader_datatype(const std::string& glslType);

    // For single shader stage (vertex, fragment)
    // *Not to be confused with the whole "combined opengl shader program"
    struct ShaderImpl
    {
        // We want to store the "shader source string" to get uniform names and locations!
        // We'll clear that after we get those uniforms
        std::string source;
        uint32_t id = 0;
    };


    // Called OpenglShader program instead of WebGL or GL ES since this could be used for
    // desktop opengl implementation as well
    class OpenglShaderProgram
    {
    private:
        class ParseError : public std::exception
        {
        private:
            std::string _msg;

        public:
            ParseError(
                int lineNumber,
                const std::vector<std::string>& lineComponents,
                const std::string& msg
            )
            {
                std::string line;
                for (const std::string& str : lineComponents)
                    line += str;

                _msg = "Line(" + std::to_string(lineNumber) + "): " + line + " | " + msg;
            }

            ParseError(
                int lineNumber,
                const std::string& line,
                const std::string& msg
            )
            {
                _msg = "Line(" + std::to_string(lineNumber) + "): " + line + " | " + msg;
            }
            virtual const char* what() const noexcept override { return _msg.c_str(); }
        };

        const ShaderImpl* _pVertexShader;
        const ShaderImpl* _pFragmentShader;
        uint32_t _id = 0;

        // Attrib locations in order of occurance in source code
        // NOTE: if GLSL Layout Qualifiers available this shouldn't be used!
        std::unordered_map<int32_t, ShaderDataType> _attributes;
        std::vector<int32_t> _uniformLocations;
        std::vector<uint32_t> _uniformBlockIndices;

    public:
        OpenglShaderProgram(
            ShaderVersion shaderVersion,
            const ShaderImpl* pVertexShader,
            const ShaderImpl* pFragmentShader
        );
        ~OpenglShaderProgram();

        inline uint32_t getID() const { return _id; }

        void setUniform(int location, const Matrix4f& matrix) const;
        void setUniform(int location, const Vector3f& v) const;
        void setUniform(int location, float val) const;
        void setUniform1i(int location, int val) const;

        ShaderDataType getAttributeType(int32_t location) const;
        int32_t getUniformBlockIndex(size_t index);

        inline const std::vector<int32_t>& getUniformLocations() const { return _uniformLocations; }
        inline const std::vector<uint32_t>& getUniformBlockIndices() const { return _uniformBlockIndices; }

    private:
        bool isConstantVar(const std::vector<std::string>& lineComponents);
        bool isStruct(const std::vector<std::string>& lineComponents);
        // TODO: Allow also writing like: "layout ( location = x ) ", etc and any possible formatted way
        bool isVertexAttrib(
            ShaderStageFlagBits shaderStage,
            const std::vector<std::string>& lineComponents
        );
        bool isUniform(const std::vector<std::string>& lineComponents);
        bool isUniformBlock(const std::string& line);

        void addAttribute(
            int lineNumber,
            const std::vector<std::string>& lineComponents
        );

        void addUniform(
            int lineNumber,
            const std::vector<std::string>& lineComponents,
            const std::unordered_map<std::string, std::vector<std::string>>& structList,
            const std::unordered_map<std::string, std::string>& constVariables
        );

        void addUniformBlock(
            int lineNumber,
            const std::string& line
        );

        // Finds attrib and/or uniform locations from shader source
        // NOTE: Works only with Opengl ES Shading language v3 (layout qualifiers included)
        void findLocationsESSL3(ShaderStageFlagBits shaderStage, const std::string shaderSource);

        void printUniforms();
    };
}
