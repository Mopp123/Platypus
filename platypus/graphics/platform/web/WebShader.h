#pragma once

#include "platypus/graphics/Shader.h"
#include "platypus/utils/Maths.h"
#include <string>
#include <vector>


namespace platypus
{
    unsigned int to_gl_shader(ShaderStageFlagBits stage);

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
        const ShaderImpl* _pVertexShader;
        const ShaderImpl* _pFragmentShader;
        uint32_t _id = 0;

        // Attrib locations in order of occurance in source code
        // NOTE: if GLSL Layout Qualifiers available this shouldn't be used!
        std::vector<int32_t> _attribLocations;
        std::vector<int32_t> _uniformLocations;

    public:
        OpenglShaderProgram(
            ShaderVersion shaderVersion,
            const ShaderImpl* pVertexShader,
            const ShaderImpl* pFragmentShader
        );
        ~OpenglShaderProgram();

        inline uint32_t getID() const { return _id; }

        // TODO: delete commented out?
        //  -> reason: atm we started getting these automatically
        //  and started just returning the _attribLocations and _uniformLocations
        //int32_t getAttribLocation(const char* name) const;
        //int32_t getUniformLocation(const char* name) const;
        void setUniform(int location, const Matrix4f& matrix) const;
        void setUniform(int location, const Vector3f& v) const;
        void setUniform(int location, float val) const;
        void setUniform1i(int location, int val) const;

        int32_t getAttribLocation(size_t index) const;

        inline const std::vector<int32_t>& getAttribLocations() const { return _attribLocations; }
        inline const std::vector<int32_t>& getUniformLocations() const { return _uniformLocations; }

    private:
        // Finds attrib and/or uniform locations from shader source
        // NOTE: Works only with Opengl ES Shading language v1 (no layout qualifiers)
        void findLocationsESSL1(const std::string shaderSource);
    };
}
