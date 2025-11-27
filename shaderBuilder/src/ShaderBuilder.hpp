#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"
#include <string>
#include <map>
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

        enum class TextureChannel
        {
            Diffuse,
            Specular,
            Normal
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

            struct NVertexOut
            {
                const std::string position  = "var_position"; // Meaning the vertex position
                const std::string positionLightSpace  = "var_positionLightSpace"; // Meaning the vertex position
                const std::string normal    = "var_normal";
                const std::string texCoord  = "var_texCoord";
                const std::string toCamera  = "var_toCamera";
                const std::string cameraPosition  = "var_cameraPosition";
                const std::string lightDirection  = "var_lightDirection";
                const std::string lightColor  = "var_lightColor";
                const std::string ambientLightColor  = "var_ambientLightColor";
                const std::string shadowProperties  = "var_shadowProperties";
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

            struct NMaterial
            {
                // Actual textures
                const std::string blendmapTexture = "blendmapTexture";
                std::map<TextureChannel, std::vector<std::string>> textures;

                // Sampled colors from textures
                const std::string blendmapTextureColor = "blendmapTextureColor";
                std::unordered_map<TextureChannel, std::vector<std::string>> textureColors;
                const std::map<TextureChannel, std::string> totalTextureColors = {
                    { TextureChannel::Diffuse, "totalDiffuseTextureColor" },
                    { TextureChannel::Specular, "totalSpecularTextureColor" },
                    { TextureChannel::Normal, "totalNormalTextureColor" }
                };

                const std::string lightingProperties = "materialData.lightingProperties";
                const std::string textureProperties = "materialData.textureProperties";

                // How many textures per channel can be blended with blendmapTextureColor.rgba
                // (+1 since also using the "blackness")
                const size_t maxChannelEntryBlends = 5;
            };

            // NOTE: These should be part of scene data if having multiple shadow casters?
            struct NShadow
            {
                const std::string pushConstants = "shadowPushConstants";
                const std::string projectionMatrix = pushConstants + ".projectionMatrix";
                const std::string viewMatrix = pushConstants + ".viewMatrix";
                const std::string shadowmapTexture = "shadowmapTexture";
            };

            struct NGlobal
            {
                const std::string toCameraSpace = "toCameraSpace";
                const std::string transformationMatrix = "transformationMatrix";
                const std::string vertexWorldPosition = "vertexWorldPosition";
                const std::string useTexCoord = "useTexCoord";
            };

            struct NFunctions
            {
                const std::string calcShadow = "calcShadow";
            };

            // TODO: None of these should be static?
            static NVertex s_inVertex;
            static NVertexOut s_outVertex;
            static NSceneData s_uSceneData;
            static NJoint s_uJoint;
            NMaterial _uMaterial;
            static NShadow s_uShadow;
            static NGlobal s_global;
            static NFunctions s_functionNames;

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

            std::set<std::string> _functionDefinitions;
            std::unordered_map<std::string, ShaderObject> _structDefinitions;
            std::unordered_map<std::string, ShaderObject> _variables;

            std::vector<ShaderObject> _output;

            const std::string _pushConstantsStructName = "PushConstants";
            const std::string _materialDataStructName = "Material";
            const std::string _materialDataInstanceName = "material";
            const size_t _maxJointsPerVertex = 4;

        public:
            ShaderStageBuilder(
                ShaderVersion version,
                uint32_t shaderStage,
                const std::vector<ShaderObject>& vertexShaderOutput,
                size_t previousStageDescriptorSetCount
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
            void addReceiveShadowPushConstants();

            // NOTE: You can't control the descriptor set number here!
            // All descriptor sets needs to be given in order!
            void addDescriptorSet(
                const std::vector<DescriptorSetLayoutBinding>& bindings,
                const std::vector<std::vector<std::string>>& bindingNames,
                const std::string& blockName,
                const std::string& blockInstanceName
            );

            void addMaterial(
                bool hasBlendmap,
                size_t diffuseTextureBindings,
                size_t specularTextureBindings,
                size_t normalTextureBindings,
                const DescriptorSetLayoutBinding& dataBinding,
                bool receiveShadows
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

            // Used for just pushing lines of func def without anything fancy!
            void pushFunction(
                const std::vector<std::string>& definition,
                const std::string name
            );

            void beginIf(const std::string& condition);
            void beginElseIf(const std::string& condition);
            void beginElse();
            void endIf();

            ShaderObject newVariable(
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

            bool functionExists(const std::string& name) const;

            void calcFinalVertexPosition();
            void calcVertexShaderOutput();
            void calcTextureColors();
            void calcNormal();
            void calcDiffuseLighting();

            void printVariables();
            void printObjects();

            static NFunctions getFunctions();

            inline const std::vector<std::string>& getLines() const { return _lines; }
            inline const std::vector<ShaderObject>& getOutput() const { return _output; }
            inline const size_t getDescriptorSetCount() const { return _descriptorSetCount; }

        private:
            void error(const std::string& msg, const char* funcName) const;
            void validateVariable(const std::string& variable, const char* funcName);
        };
    }
}
