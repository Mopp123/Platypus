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
            ShaderObject* pDefinition = nullptr;
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
        private:
            // All possible named vertex attributes
            struct NVertex
            {
                const std::string position  = "vertex.position";
                const std::string normal    = "vertex.normal";
                const std::string weights   = "vertex.weights";
                const std::string jointIDs  = "vertex.jointIDs";
                const std::string texCoord  = "vertex.texCoord";
                const std::string tangent   = "vertex.tangent";
            };

            const std::string _outputPrefix = "out_";
            const std::string _inputPrefix = "in_";
            struct NVertexOut
            {
                const std::string position  = "position"; // Meaning the vertex position
                const std::string normal    = "normal";
                const std::string texCoord  = "texCoord";
                const std::string toCamera  = "toCamera";
                const std::string cameraPosition  = "cameraPosition";
                const std::string lightDirection  = "lightDirection";
                const std::string lightColor  = "lightColor";
                const std::string ambientLightColor  = "ambientLightColor";
            };

            struct NSceneData
            {
                const std::string projectionMatrix  = "sceneData.projectionMatrix";
                const std::string viewMatrix        = "sceneData.viewMatrix";
                const std::string cameraPosition    = "sceneData.cameraPosition";

                const std::string ambientLightColor = "sceneData.ambientLightColor";
                const std::string lightDirection    = "sceneData.lightDirection";
                const std::string lightColor        = "sceneData.lightColor";

                const std::string shadowProperties  = "sceneData.shadowProperties";
            };

            struct NJoint
            {
                const std::string jointMatrices = "jointData.data";
            };

            struct NGlobal
            {
                const std::string toCameraSpace = "toCameraSpace";
                const std::string transformationMatrix = "transformationMatrix";
                const std::string vertexWorldPosition = "vertexWorldPosition";
            };

            static NVertex s_inVertex;
            static NVertexOut s_outVertex;
            static NSceneData s_uSceneData;
            static NJoint s_uJoint;
            static NGlobal s_global;

            ShaderVersion _version;
            uint32_t _shaderStage;
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

            std::vector<ShaderObject> _output;

            const std::string _pushConstantsStructName = "PushConstants";
            const size_t _maxJointsPerVertex = 4;

        public:
            ShaderStageBuilder(
                ShaderVersion version,
                uint32_t shaderStage,
                const std::vector<ShaderObject>& vertexShaderOutput
            );
            virtual ~ShaderStageBuilder() {}
            void build();

            // NOTE: Maybe should call "declare" or "define" instead of "add" here?
            void beginPushConstants();
            void endPushConstants(const std::string& instanceName);
            void beginUniformBlock(
                const std::string& blockName,
                UniformBlockLayout blockLayout,
                uint32_t setIndex,
                uint32_t binding
            );
            void endUniformBlock(const std::string& instanceName);

            void addInput(
                uint32_t location,
                ShaderDataType type,
                const std::string& name
            );

            void addOutput(
                ShaderDataType type,
                const std::string& name,
                const std::string& value
            );

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
                const std::string& structName,
                int arrayLength = 1
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

            void beginIf(const std::string& condition);
            void beginElseIf(const std::string& condition);
            void beginElse();
            void endIf();

            void newVariable(
                ShaderDataType type,
                const std::string& name,
                const std::string& value
            );

            void endSection();

            void addLine(const std::string& line);

            bool variableExists(const std::string& name) const;
            bool variablesExists(
                const std::vector<std::string>& toSearch,
                std::vector<std::string>& missingVariables
            ) const;
            bool structExists(const std::string& name) const;
            bool structMemberExists(
                const ShaderObject& shaderStruct,
                const std::string memberName
            ) const;

            void calcFinalVertexPosition();
            void calcVertexShaderOutput();

            void printVariables();
            void printObjects();

            inline const std::vector<std::string>& getLines() const { return _lines; }
            inline const std::vector<ShaderObject>& getOutput() const { return _output; }

        private:
            void error(const std::string& msg, const char* funcName) const;
            void validateVariable(const std::string& variable, const char* funcName);
        };
    }
}
