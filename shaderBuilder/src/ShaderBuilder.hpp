#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"
#include <string>
#include <unordered_map>


namespace platypus
{
    namespace shaderBuilder
    {
        struct ObjectCalculation;
        struct ShaderObject
        {
            std::string name;
            ShaderDataType type = ShaderDataType::None;
            std::vector<ShaderObject> members; // Members, if this type == Struct
            std::unordered_map<std::string, size_t> memberIndices;
        };

        enum class CalcOperation
        {
            Mul
        };

        struct ObjectCalculation
        {
            ShaderObject left;
            ShaderObject right;
            CalcOperation operation;
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
            ShaderDataType type;
            std::string name;
            std::string structName;
            FunctionArgQualifier argQualifier;
        };

        class ShaderStageBuilder
        {
        protected:
            std::vector<std::string> _lines;
            const std::string _indentation = "    ";

            ShaderObject* _pActiveWriteObject = nullptr;

            size_t _currentScopeIndentation = 0;
            size_t _descriptorSetCount = 0;

            // At which line the next encountered output definition gets put.
            // All outputs are defined right before the main function.
            size_t _nextOutDefinitionPos = 0;

            std::unordered_map<std::string, ShaderObject> _structDefinitions;
            std::unordered_map<std::string, ShaderObject> _variables;

            const std::string _pushConstantsStructName = "PushConstants";

            const std::string _varNameVertexWorldSpace = "vertexWorldSpace";

        public:
            ShaderStageBuilder();
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
                const std::vector<std::string>& variableNames,
                const std::string& instanceName
            );
            void addUniformBlock(
                const DescriptorSetLayoutBinding& descriptorSetLayoutBinding,
                const std::vector<std::string>& variableNames,
                const std::string& blockName,
                const std::string& instanceName
            );

            // Used for adding members to object definitions
            void addVariable(
                ShaderDataType type,
                const std::string& name,
                const std::string& structName
            );
            void reqisterVariable(const ShaderObject& variable, const std::string& parentName);

            void beginStruct(const std::string& name);
            void endStruct();
            // TODO: Make less fucked!
            void instantiateObject(
                const std::string& structName,
                const std::string& instanceName,
                const std::vector<std::pair<ShaderDataType, std::string>>& args
            );

            void beginFunction(
                const std::string& functionName,
                ShaderDataType returnType,
                const std::vector<FunctionInput>& args
            );
            void endFunction(const std::string& returnValueName);

            void endSection();

            void addLine(const std::string& line);

            void setOutputTEST(
                uint32_t location,
                ShaderDataType type,
                const std::string name
            );

            bool variableExists(const std::string& name) const;
            bool structExists(const std::string& name) const;
            bool structMemberExists(
                const ShaderObject& shaderStruct,
                const std::string memberName
            ) const;


            // TESTING -----
            ShaderObject newVec4(const std::string& instanceName);
            ShaderObject newVec4(
                const std::string& instanceName,
                const ShaderObject& vec3,
                const std::string& w
            );

            void eval(
                const ShaderObject& target,
                ObjectCalculation calculation
            );

            void calcVertexWorldPosition();

            void printVariables();
            void printObjects();

            virtual void beginPushConstants() = 0;
            virtual void endPushConstants(const std::string& instanceName) = 0;
            virtual void beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding) = 0;
            virtual void endUniformBlock(const std::string& instanceName) = 0;

            inline const std::vector<std::string>& getLines() const { return _lines; }
        };

        class VulkanShaderStageBuilder : public ShaderStageBuilder
        {
        public:
            ~VulkanShaderStageBuilder();
            virtual void beginPushConstants();
            virtual void endPushConstants(const std::string& instanceName);
            virtual void beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding);
            virtual void endUniformBlock(const std::string& instanceName);
        };
    }
}
