#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"
#include <string>


namespace platypus
{
    namespace shaderBuilder
    {
        struct ShaderVariable
        {
            ShaderDataType type = ShaderDataType::None;
            std::string name = "";
        };

        enum class FunctionArgQualifier
        {
            None,
            In,
            Out,
            InOut
        };

        struct FunctionInput
        {
            ShaderVariable variable;
            FunctionArgQualifier argQualifier;
        };

        class ShaderStageBuilder
        {
        protected:
            std::vector<std::string> _lines;
            const std::string _indentation = "    ";
            size_t _currentScopeIndentation = 0;
            size_t _descriptorSetCount = 0;

            // At which line the next encountered output definition gets put.
            // All outputs are defined right before the main function.
            size_t _nextOutDefinitionPos = 0;

        public:
            virtual ~ShaderStageBuilder() {}

            // NOTE: Maybe should call "declare" or "define" instead of "add" here?
            void addVersion(ShaderVersion version);
            void addVertexAttributes(
                const std::vector<VertexBufferLayout>& vertexBufferLayouts,
                const std::vector<std::vector<std::string>>& layoutAttributeNames,
                bool useLocationByOrder = false
            );
            void addPushConstants(
                const std::vector<UniformInfo>& pushConstantsInfo,
                const std::vector<std::string>& variableNames
            );
            void addUniformBlock(
                const DescriptorSetLayoutBinding& descriptorSetLayoutBinding,
                const std::vector<std::string>& variableNames,
                const std::string& blockName,
                const std::string& blockVariableName
            );
            void addVariable(ShaderDataType type, const std::string name);

            void beginFunction(
                const std::string& functionName,
                ShaderDataType returnType,
                const std::vector<FunctionInput>& args
            );
            void endFunction(ShaderVariable returnValue);

            void endSection();

            void addLine(const std::string& line);

            void setOutputTEST(uint32_t location, ShaderVariable variable);

            virtual void beginPushConstants() = 0;
            virtual void endPushConstants(const std::string& variableName) = 0;
            virtual void beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding) = 0;
            virtual void endUniformBlock(const std::string& variableName) = 0;

            inline const std::vector<std::string>& getLines() const { return _lines; }
        };

        class VulkanShaderStageBuilder : public ShaderStageBuilder
        {
        public:
            ~VulkanShaderStageBuilder();
            virtual void beginPushConstants();
            virtual void endPushConstants(const std::string& variableName);
            virtual void beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding);
            virtual void endUniformBlock(const std::string& variableName);
        };
    }
}
