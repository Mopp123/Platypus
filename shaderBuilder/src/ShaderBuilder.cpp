#include "ShaderBuilder.hpp"
#include "platypus/core/Debug.h"
#include <format>


namespace platypus
{
    namespace shaderBuilder
    {
        static std::string vector_to_string(const std::vector<std::string>& v)
        {
            std::string out;
            for (size_t i = 0; i < v.size(); ++i)
                out += "    " + v[i] + "\n";

            return out;
        }

        static std::string shader_datatype_to_glsl(ShaderDataType type)
        {
            switch (type)
            {
                case None: return "void";
                case Int: return "int";
                case Int2: return "ivec2";
                case Int3: return "ivec3";
                case Int4: return "ivec4";

                case Float: return "float";
                case Float2: return "vec2";
                case Float3: return "vec3";
                case Float4: return "vec4";

                case Mat3: return "mat3";
                case Mat4: return "mat4";

                case Sampler2D: return "sampler2D";

                default: {
                    Debug::log(
                        "@shader_datatype_to_glsl(ShaderBuilder) "
                        "Invalide shader datatype: " + shader_datatype_to_string(type),
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return "<ERROR: Invalid type>";
                }
            }
            return "<ERROR: Invalid type>";
        }


        static std::string function_arg_qualifier_to_glsl(FunctionArgQualifier qualifier)
        {
            switch (qualifier)
            {
                case FunctionArgQualifier::None: return "";
                case FunctionArgQualifier::In: return "in";
                case FunctionArgQualifier::Out: return "out";
                case FunctionArgQualifier::InOut: return "inout";
            }
        }

        static bool is_number(const std::string& str)
        {
            if (str.empty())
                return false;

            bool firstDigit = false;
            bool foundDecimal = false;
            for (char c : str)
            {
                if (!std::isdigit(c))
                {
                    if (firstDigit && c == '-')
                        continue;
                    else if (!foundDecimal && c == '.')
                        continue;

                    return false;
                }
            }
            return true;
        }

        static char vector_component_index_to_name(size_t index, bool rgba)
        {
            if (index >= 4)
            {
                Debug::log(
                    "@vector_component_index_to_variable_xyzw "
                    "Index(" + std::to_string(index) + ") out of bounds!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return ' ';
            }

            if (rgba)
            {
                switch (index)
                {
                    case 0: return 'r';
                    case 1: return 'g';
                    case 2: return 'b';
                    case 3: return 'a';
                }
            }
            switch (index)
            {
                case 0: return 'x';
                case 1: return 'y';
                case 2: return 'z';
                case 3: return 'w';
            }
        }

        static std::string texture_channel_to_string(TextureChannel type)
        {
            switch (type)
            {
                case TextureChannel::Blendmap: return "blendmap";
                case TextureChannel::Diffuse: return "diffuse";
                case TextureChannel::Specular: return "Specular";
                case TextureChannel::Normal: return "normal";
                default: return "<Invalid type>";
            }
        }


        ShaderStageBuilder::NVertex ShaderStageBuilder::s_inVertex;
        ShaderStageBuilder::NVertexOut ShaderStageBuilder::s_outVertex;
        ShaderStageBuilder::NSceneData ShaderStageBuilder::s_uSceneData;
        ShaderStageBuilder::NJoint ShaderStageBuilder::s_uJoint;
        //ShaderStageBuilder::NMaterial ShaderStageBuilder::s_uMaterial;
        ShaderStageBuilder::NGlobal ShaderStageBuilder::s_global;
        ShaderStageBuilder::ShaderStageBuilder(
            ShaderVersion version,
            uint32_t shaderStage,
            const std::vector<ShaderObject>& vertexShaderOutput,
            size_t previousStageDescriptorSetCount
        ) :
            _version(version),
            _shaderStage(shaderStage),
            _descriptorSetCount(previousStageDescriptorSetCount)
        {
            // Temporarely adding all basic types here...
            _structDefinitions["int"] = { "int", ShaderDataType::Int };
            _structDefinitions["float"] = { "float", ShaderDataType::Float };

            _structDefinitions["vec2"] = {
                "vec2",
                ShaderDataType::Float2,
                {
                    { "x", ShaderDataType::Float },
                    { "y", ShaderDataType::Float }
                },
                { { "x", 0 }, { "y", 1 } }
            };

            _structDefinitions["vec3"] = {
                "vec3",
                ShaderDataType::Float3,
                {
                    { "x", ShaderDataType::Float },
                    { "y", ShaderDataType::Float },
                    { "z", ShaderDataType::Float }
                },
                { { "x", 0 }, { "y", 1 }, { "z", 2 } }
            };

            _structDefinitions["vec4"] = {
                "vec4",
                ShaderDataType::Float4,
                {
                    { "x", ShaderDataType::Float },
                    { "y", ShaderDataType::Float },
                    { "z", ShaderDataType::Float },
                    { "w", ShaderDataType::Float }
                },
                { { "x", 0 }, { "y", 1 }, { "z", 2 }, { "w", 3 } }
            };

            _structDefinitions["mat3"] = {
                "mat3",
                ShaderDataType::Mat3,
                { }, // NOTE: Not sure what to do with the members here?
                { }
            };
            _structDefinitions["mat4"] = {
                "mat4",
                ShaderDataType::Mat4,
                { }, // NOTE: Not sure what to do with the members here?
                { }
            };

            _structDefinitions["sampler2D"] = {
                "sampler2D",
                ShaderDataType::Sampler2D,
                { }, // NOTE: Not sure what to do with the members here?
                { }
            };

            if (_version == ShaderVersion::VULKAN_GLSL_450)
            {
                addLine("#version 450");
            }
            else if (_version == ShaderVersion::OPENGLES_GLSL_300)
            {
                addLine("#version 300 es");

                // TODO: Make this configurable?
                addLine("precision mediump float;");
            }
            else
            {
                Debug::log(
                    "@ShaderStageBuilder::addVersion "
                    "Invalid shader version: " + shader_version_to_string(version),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            endSection();

            // Add inputs from vertex shader
            for (size_t i = 0; i < vertexShaderOutput.size(); ++i)
            {
                const ShaderObject& obj = vertexShaderOutput[i];
                addInput((uint32_t)i, obj.type, obj.name);
            }
            endSection();
        }

        void ShaderStageBuilder::build()
        {
            beginFunction(
                "main",
                ShaderDataType::None,
                { }
            );
            {
                if (_shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
                {
                    instantiateObject(
                        "Vertex",
                        "vertex",
                        {
                            { ShaderDataType::Float3, "position" },
                            { ShaderDataType::Float4, "weights" },
                            { ShaderDataType::Float4, "jointIDs" },
                            { ShaderDataType::Float3, "normal" },
                            { ShaderDataType::Float2, "texCoord" }
                        }
                    );

                    calcFinalVertexPosition();
                    calcVertexShaderOutput();
                }
                else if (_shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
                {
                    const std::string inTexCoord = _inputPrefix + s_outVertex.texCoord;
                    if (!variableExists(inTexCoord))
                    {
                        error("Variable: " + inTexCoord + " not found!", PLATYPUS_CURRENT_FUNC_NAME);
                    }
                    // Apply texture scale and offset from materialData to texCoord
                    newVariable(
                        ShaderDataType::Float2,
                        s_global.useTexCoord,
                        inTexCoord + " * " + _uMaterial.textureProperties + ".zw + " + _uMaterial.textureProperties + ".xy"
                    );
                    calcTextureColors();
                }
            }
            endFunction({});
        }

        void ShaderStageBuilder::beginPushConstants()
        {
            if (_pActiveWriteObject != nullptr)
            {
                Debug::log(
                    "@VulkanShaderStageBuilder::beginPushConstants "
                    "_pActiveWriteObject wasn't nullptr! "
                    "Make sure you ended previous object write operation with the appropriate "
                    "end function!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            _structDefinitions[_pushConstantsStructName] = { _pushConstantsStructName, ShaderDataType::Struct };
            if (_version == ShaderVersion::VULKAN_GLSL_450)
            {
                addLine("layout(push_constant) uniform " + _pushConstantsStructName);
                addLine("{");
            }
            else if (_version == ShaderVersion::OPENGLES_GLSL_300)
            {
                addLine("struct " + _pushConstantsStructName);
                addLine("{");
            }
            else
            {
                error(
                    "Invalid shader version: " + shader_version_to_string(_version),
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }

            _pActiveWriteObject = &_structDefinitions[_pushConstantsStructName];
        }

        void ShaderStageBuilder::endPushConstants(const std::string& instanceName)
        {
            if (_version == ShaderVersion::VULKAN_GLSL_450)
            {
                addLine("} " + instanceName + ";");
            }
            else if (_version == ShaderVersion::OPENGLES_GLSL_300)
            {
                addLine("};");
                addLine("uniform " + _pushConstantsStructName + " " + instanceName + ";");
            }
            else
            {
                error(
                    "Invalid shader version: " + shader_version_to_string(_version),
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }
            endSection();
            _pActiveWriteObject = nullptr;
        }

        void ShaderStageBuilder::beginUniformBlock(
            const std::string& blockName,
            UniformBlockLayout blockLayout,
            uint32_t setIndex,
            uint32_t binding
        )
        {
            if (_pActiveWriteObject != nullptr)
            {
                error(
                    "_pActiveWriteObject wasn't nullptr! "
                    "Make sure you ended previous object write operation with the appropriate "
                    "end function!",
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }
            _structDefinitions[blockName] = { blockName, ShaderDataType::Struct };
            const std::string blockLayoutStr = uniform_block_layout_to_string(blockLayout);
            if (_version == ShaderVersion::VULKAN_GLSL_450)
            {
                addLine("layout(" + blockLayoutStr + ", set = " + std::to_string(setIndex) + ", binding = " + std::to_string(binding) + ") uniform " + blockName);
                addLine("{");
            }
            else if (_version == ShaderVersion::OPENGLES_GLSL_300)
            {
                addLine("layout(" + blockLayoutStr + ") uniform " + blockName);
                addLine("{");
            }
            else
            {
                error(
                    "Invalid shader version: " + shader_version_to_string(_version),
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }
            _pActiveWriteObject = &_structDefinitions[blockName];
        }

        void ShaderStageBuilder::endUniformBlock(const std::string& instanceName)
        {
            addLine("} " + instanceName + ";");
            endSection();
            _pActiveWriteObject = nullptr;
        }

        void ShaderStageBuilder::addInput(
            uint32_t location,
            ShaderDataType type,
            const std::string& name
        )
        {
            if (variableExists(name))
            {
                error(
                    "Variable: " + name + " already exists!",
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }
            // Also add all the vector components as variables.
            // This is done in order to be able to get anything like:
            //  someObject.vertex.position.z
            // Pretty annoying and dumb, I know...
            if (type != ShaderDataType::Mat3 &&
                type != ShaderDataType::Mat4 &&
                type != ShaderDataType::Int &&
                type != ShaderDataType::Float &&
                type != ShaderDataType::Struct &&
                type != ShaderDataType::None
            )
            {
                size_t attribComponentCount = get_shader_datatype_component_count(type);
                for (size_t i = 0; i < attribComponentCount; ++i)
                {
                    std::string attribMemberNameXYZW = name + "." + vector_component_index_to_name(i, false);
                    std::string attribMemberNameRGBA = name + "." + vector_component_index_to_name(i, true);
                    _variables[attribMemberNameXYZW] = { attribMemberNameXYZW, ShaderDataType::Float };
                    _variables[attribMemberNameRGBA] = { attribMemberNameRGBA, ShaderDataType::Float };
                }
            }

            // Add name prefix only in fragment shader
            std::string namePrefix;
            if (_shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
                namePrefix = _inputPrefix;

            // ES 300 doesn't have layout qualifiers in fragment shader input!
            std::string layoutPrefix;
            if (_version == ShaderVersion::VULKAN_GLSL_450 ||
                (_version == ShaderVersion::OPENGLES_GLSL_300 && _shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT))
            {
                layoutPrefix = "layout(location = " + std::to_string(location) + ") ";
            }
            std::string fullName = namePrefix + name;
            addLine(layoutPrefix + "in " + shader_datatype_to_glsl(type) + " " + fullName + ";");
            _variables[fullName] = { fullName, type };
        }

        void ShaderStageBuilder::addOutput(
            ShaderDataType type,
            const std::string& name,
            const std::string& value
        )
        {
            if (_nextOutDefinitionPos == 0)
            {
                Debug::log(
                    "ShaderStageBuilder::setOutputTEST "
                    "Invalid row(" + std::to_string(_nextOutDefinitionPos) + ") for _nextOutDefinitionPos",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            const size_t location = _output.size();
            const std::string typeName = shader_datatype_to_glsl(type);
            ShaderObject object = _structDefinitions[typeName];
            object.name = name;
            _output.push_back(object);

            addLine(name + " = " + value + ";");

            std::vector<std::string> tempLines;
            tempLines.insert(tempLines.end(), _lines.begin(), _lines.begin() + _nextOutDefinitionPos);
            tempLines.push_back("layout(location = " + std::to_string(location) + ") out " + typeName + " " + name + ";\n");
            size_t nextOutPosition = tempLines.size();
            tempLines.insert(tempLines.end(), _lines.begin() + _nextOutDefinitionPos, _lines.end());
            _nextOutDefinitionPos = nextOutPosition;
            _lines = tempLines;
        }


        void ShaderStageBuilder::addVertexAttributes(
            const std::vector<VertexBufferLayout>& vertexBufferLayouts,
            const std::vector<std::vector<std::string>>& layoutAttributeNames,
            bool useLocationByOrder
        )
        {
            if (vertexBufferLayouts.size() != layoutAttributeNames.size())
            {
                Debug::log(
                    "@ShaderStageBuilder::addVertexAttributes "
                    "Provided " + std::to_string(layoutAttributeNames.size()) + " layout attribute names "
                    "for " + std::to_string(vertexBufferLayouts.size()) + " vertex buffer layouts. "
                    "You must provide matching number of vertex buffer layouts and layout names, "
                    "that contains a name for each of the layout's element",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            std::vector<ShaderObject> vertexAttributes;
            size_t layoutIndex = 0;
            uint32_t locationIndex = 0;
            for (const VertexBufferLayout& layout : vertexBufferLayouts)
            {
                const std::vector<std::string>& attributeNames = layoutAttributeNames[layoutIndex];
                const std::vector<VertexBufferElement>& elements = layout.getElements();
                if (elements.size() != attributeNames.size())
                {
                    Debug::log(
                        "@ShaderStageBuilder::addVertexAttributes "
                        "Provided " + std::to_string(attributeNames.size()) + " attribute names "
                        "for layout at index " + std::to_string(layoutIndex) + " that has " + std::to_string(elements.size()) + " elements."
                        "You must provide matching number of names for each layout's elements.",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
                {
                    const VertexBufferElement& element = elements[elementIndex];
                    const ShaderDataType elementType = element.getType();
                    uint32_t elementLocation = element.getLocation();

                    if (useLocationByOrder)
                        elementLocation = locationIndex;
                    else
                        elementLocation = elementLocation;

                    std::string attributeName = attributeNames[elementIndex];

                    // NOTE: For now all instance data is separate from "actual mesh vertex
                    // attributes"! Currently the instanced vertex attribs are globally accessably.
                    // TODO: separate struct for instanced data?
                    ShaderObject attribObject{ attributeName, elementType };
                    if (layout.getInputRate() == VertexInputRate::VERTEX_INPUT_RATE_VERTEX)
                        vertexAttributes.push_back(attribObject);

                    addInput(elementLocation, element.getType(), attributeName);

                    // Consumes 4 locations, if mat4
                    if (useLocationByOrder && elementType == ShaderDataType::Mat4)
                        locationIndex += 4;
                    else
                        ++locationIndex;
                }
                ++layoutIndex;
            }
            endSection();

            beginStruct("Vertex");
            for (const ShaderObject& vertexAttribute : vertexAttributes)
            {
                addVariable(vertexAttribute.type, vertexAttribute.name, "");
            }
            endStruct();
            endSection();
        }

        void ShaderStageBuilder::addPushConstants(
            const std::vector<UniformInfo>& pushConstantsInfo,
            const std::vector<std::string>& variableNames,
            const std::string& instanceName
        )
        {
            if (pushConstantsInfo.size() != variableNames.size())
            {
                Debug::log(
                    "@ShaderStageBuilder::addPushConstant "
                    "Provided " + std::to_string(variableNames.size()) + " names "
                    "for " + std::to_string(pushConstantsInfo.size()) + " push constant variables. "
                    "You have to provide matching number of push constants and names!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            if (structExists(_pushConstantsStructName))
            {
                Debug::log(
                    "@ShaderStageBuilder::addPushConstant "
                    "Push constants already defined!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            beginPushConstants();
            ++_currentScopeIndentation;
            for (size_t i = 0; i < pushConstantsInfo.size(); ++i)
            {
                const UniformInfo& uniformInfo = pushConstantsInfo[i];
                addVariable(uniformInfo.type, variableNames[i], "");
            }
            --_currentScopeIndentation;
            endPushConstants("shadowPushConstants");

            // Instantiate
            const ShaderObject& pushConstantsDefinition = _structDefinitions[_pushConstantsStructName];
            for (auto member : pushConstantsDefinition.memberIndices)
            {
                const std::string& memberName = member.first;
                std::string fullName = instanceName + "." + memberName;
                _variables[fullName] = pushConstantsDefinition.members[member.second];
            }
        }

        void ShaderStageBuilder::addDescriptorSet(
            const std::vector<DescriptorSetLayoutBinding>& bindings,
            const std::vector<std::vector<std::string>>& bindingNames,
            const std::string& blockName,
            const std::string& blockInstanceName
        )
        {
            if (bindingNames.size() != bindings.size())
            {
                error(
                    "Provided " + std::to_string(bindingNames.size()) + " names "
                    "for " + std::to_string(bindings.size()) + " descriptor set layout bindings. "
                    "You have to provide matching number of elements in bindings and bindingNames!",
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                const DescriptorSetLayoutBinding& binding = bindings[i];
                const std::vector<UniformInfo>& uniformInfo = binding.getUniformInfo();
                if (uniformInfo.size() != bindingNames[i].size())
                {
                    error(
                        "Provided " + std::to_string(bindingNames[i].size()) + " names "
                        "for " + std::to_string(uniformInfo.size()) + " uniform infos at binding index " + std::to_string(i) + " "
                        "You have to provide matching number of elements in bindings' uniform infos and binding names!",
                        PLATYPUS_CURRENT_FUNC_NAME
                    );
                }
                if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    if (binding.getUniformInfo().size() != 1)
                    {
                        error(
                            "Texture bindings can't have more than one uniform info!",
                            PLATYPUS_CURRENT_FUNC_NAME
                        );
                    }

                    std::string layoutPrefix;
                    if (_version == ShaderVersion::VULKAN_GLSL_450)
                        layoutPrefix = "layout(set = " + std::to_string(_descriptorSetCount) + ", binding = " + std::to_string(i) + ") ";

                    ShaderObject obj{ bindingNames[i][0], ShaderDataType::Sampler2D };
                    addLine(layoutPrefix + "uniform " + shader_datatype_to_glsl(obj.type) + " " + obj.name + ";");
                    reqisterVariable(obj, "");
                }
                else if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                        binding.getType() == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
                {
                    beginUniformBlock(
                        blockName,
                        UniformBlockLayout::std140,
                        _descriptorSetCount,
                        i
                    );
                    ++_currentScopeIndentation;
                    for (size_t j = 0; j < uniformInfo.size(); ++j)
                    {
                        addVariable(uniformInfo[j].type, bindingNames[i][j], "", uniformInfo[j].arrayLen);
                    }
                    --_currentScopeIndentation;
                    endUniformBlock(blockInstanceName);

                    // Instantiate
                    const ShaderObject& blockDefinition = _structDefinitions[blockName];
                    for (auto member : blockDefinition.memberIndices)
                    {
                        const std::string& memberName = member.first;
                        std::string fullName = blockInstanceName + "." + memberName;
                        // Need to change the internal name of the member to global here..
                        //  ...annoying and shit fuck.. this is becomming a mess..
                        ShaderObject globalObject = blockDefinition.members[member.second];
                        globalObject.name = fullName;
                        _variables[fullName] = globalObject;
                    }
                }
                else
                {
                    error(
                        "Invalid descriptor type at binding index: " + std::to_string(i),
                        PLATYPUS_CURRENT_FUNC_NAME
                    );
                }
            }
            ++_descriptorSetCount;
        }

        void ShaderStageBuilder::addMaterial(
            const DescriptorSetLayoutBinding& blendmapTextureBinding,
            const std::vector<DescriptorSetLayoutBinding>& diffuseTextureBindings,
            const std::vector<DescriptorSetLayoutBinding>& specularTextureBindings,
            const std::vector<DescriptorSetLayoutBinding>& normalTextureBindings,
            const DescriptorSetLayoutBinding& dataBinding
        )
        {
            std::vector<DescriptorSetLayoutBinding> combinedBindings;
            std::vector<std::vector<std::string>> bindingNames;
            if (blendmapTextureBinding.getType() != DescriptorType::DESCRIPTOR_TYPE_NONE)
            {
                combinedBindings.push_back(blendmapTextureBinding);
                std::string textureName = texture_channel_to_string(TextureChannel::Blendmap) + "Texture";
                _uMaterial.textures[TextureChannel::Blendmap].push_back(textureName);
                bindingNames.push_back({ textureName });
            }

            for (size_t i = 0; i < diffuseTextureBindings.size(); ++i)
            {
                combinedBindings.push_back(diffuseTextureBindings[i]);
                std::string textureName = texture_channel_to_string(TextureChannel::Diffuse) + "Texture" + std::to_string(i);
                _uMaterial.textures[TextureChannel::Diffuse].push_back(textureName);
                bindingNames.push_back({ textureName });
            }
            for (size_t i = 0; i < specularTextureBindings.size(); ++i)
            {
                combinedBindings.push_back(specularTextureBindings[i]);
                std::string textureName = texture_channel_to_string(TextureChannel::Specular) + "Texture" + std::to_string(i);
                _uMaterial.textures[TextureChannel::Specular].push_back(textureName);
                bindingNames.push_back({ textureName });
            }
            for (size_t i = 0; i < normalTextureBindings.size(); ++i)
            {
                combinedBindings.push_back(normalTextureBindings[i]);
                std::string textureName = texture_channel_to_string(TextureChannel::Normal) + "Texture" + std::to_string(i);
                _uMaterial.textures[TextureChannel::Normal].push_back(textureName);
                bindingNames.push_back({ textureName });
            }

            combinedBindings.push_back(dataBinding);
            bindingNames.push_back({ "lightingProperties", "textureProperties" });

            addDescriptorSet(
                combinedBindings,
                bindingNames,
                _materialDataStructName,
                _materialDataInstanceName
            );
        }

        // TODO: Allow adding/defining structs as members
        void ShaderStageBuilder::addVariable(
            ShaderDataType type,
            const std::string& name,
            const std::string& structName,
            int arrayLength
        )
        {
            if (!_pActiveWriteObject)
            {
                Debug::log(
                    "@ShaderStageBuilder::addVariable "
                    "Active write object was nullptr! "
                    "Make sure you began push constant, uniform block or struct with the "
                    "appropriate function!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            if (name.empty())
            {
                Debug::log(
                    "@ShaderStageBuilder::addVariable "
                    "name required!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            ShaderObject newVariable{ name, type };
            std::string useType = shader_datatype_to_glsl(type);
            if (type == ShaderDataType::Struct)
            {
                if (!structExists(structName))
                {
                    Debug::log(
                        "@ShaderStageBuilder::addVariable "
                        "Variable type was struct but no struct named: " + structName + " found!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }
                useType = structName;
                const ShaderObject& shaderStruct = _structDefinitions[structName];
                newVariable.members = shaderStruct.members;
                newVariable.memberIndices = shaderStruct.memberIndices;
                _variables[_pActiveWriteObject->name + "." + name] = newVariable;
            }

            if (_pActiveWriteObject->memberIndices.find(name) != _pActiveWriteObject->memberIndices.end())
            {
                Debug::log(
                    "@ShaderStageBuilder::addVariable "
                    "Struct: " + structName + " already has member: " + name,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            _pActiveWriteObject->memberIndices[name] = _pActiveWriteObject->members.size();
            _pActiveWriteObject->members.push_back(newVariable);

            std::string line = useType + " " + name;
            if (arrayLength > 1)
                line += "[" + std::to_string(arrayLength) + "]";

            addLine(line + ";");
        }

        void ShaderStageBuilder::reqisterVariable(const ShaderObject& variable, const std::string& parentName)
        {
            std::string fullName = parentName.empty() ? variable.name : parentName + "." + variable.name;
            _variables[fullName] = variable;
            for (const ShaderObject& member : variable.members)
            {
                reqisterVariable(member, fullName);
            }
        }

        void ShaderStageBuilder::beginStruct(const std::string& name)
        {
            if (_pActiveWriteObject != nullptr)
            {
                Debug::log(
                    "@ShaderStageBuilder::beginStruct "
                    "_pActiveWriteObject wasn't nullptr! "
                    "Make sure you ended previous object write operation with the appropriate "
                    "end function!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            if (structExists(name))
            {
                Debug::log(
                    "@ShaderStageBuilder::beginStruct "
                    "Struct: " + name + " already exists!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            addLine("struct " + name);
            addLine("{");
            ++_currentScopeIndentation;

            _structDefinitions[name] = { name, ShaderDataType::Struct };
            _pActiveWriteObject = &_structDefinitions[name];
        }

        void ShaderStageBuilder::endStruct()
        {
            --_currentScopeIndentation;
            addLine("};");
            _pActiveWriteObject = nullptr;
        }

        void ShaderStageBuilder::instantiateObject(
            const std::string& structName,
            const std::string& instanceName,
            const std::vector<std::pair<ShaderDataType, std::string>>& args
        )
        {
            if (!structExists(structName))
            {
                std::string definedStructs;
                for (auto definedStruct : _structDefinitions)
                    definedStructs += definedStruct.first + "\n";

                Debug::log(
                    "@ShaderStageBuilder::instantiateObject "
                    "No struct definition found for: " + structName + " "
                    "Currently defined structs:\n" + definedStructs,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            if (args.empty())
            {
                Debug::log(
                    "@ShaderStageBuilder::instantiateObject "
                    "Provided no args. No structs should exist without members!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            // TODO: Validate already existing variable names in current scope!
            std::string line = structName + " " + instanceName + " = " + structName + "(";
            const ShaderObject& structDefinition = _structDefinitions[structName];
            if (structDefinition.members.size() != args.size())
            {
                Debug::log(
                    "@ShaderStageBuilder::instantiateObject "
                    "Provided " + std::to_string(args.size()) + " args for"
                    " " + structName + " struct which has " + std::to_string(structDefinition.members.size()) + " "
                    "members!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            for (size_t i = 0; i < args.size(); ++i)
            {
                ShaderDataType argType = args[i].first;
                const std::string& arg = args[i].second;
                if (argType != structDefinition.members[i].type)
                {
                    Debug::log(
                        "@ShaderStageBuilder::instantiateObject "
                        "Invalid type: " + shader_datatype_to_string(argType) + " "
                        "for arg: " + arg,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                if (!is_number(arg) && !variableExists(arg))
                {
                    Debug::log(
                        "@ShaderStageBuilder::instantiateObject "
                        "Invalid argument: " + arg,
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                const std::string& fullVariableName = instanceName + "." + structDefinition.members[i].name;
                _variables[fullVariableName] = {
                    fullVariableName,
                    structDefinition.members[i].type,
                    structDefinition.members[i].members,
                    structDefinition.members[i].memberIndices,
                };

                line += arg;
                if (i < args.size() - 1)
                    line += ", ";
            }

            line += ");";

            addLine(line);
            endSection();
        }

        void ShaderStageBuilder::beginFunction(
            const std::string& functionName,
            ShaderDataType returnType,
            const std::vector<FunctionInput>& args
        )
        {
            if (functionName == "main")
            {
                _nextOutDefinitionPos = _lines.size();
            }

            std::string line;
            line += shader_datatype_to_glsl(returnType) + " " + functionName + "(";
            for (size_t i = 0; i < args.size(); ++i)
            {
                const FunctionInput& arg = args[i];
                std::string qualifier = function_arg_qualifier_to_glsl(arg.argQualifier);
                line += qualifier;
                if (!qualifier.empty())
                    line += " ";

                std::string useType = shader_datatype_to_glsl(arg.type);
                if (arg.type == ShaderDataType::Struct)
                {
                    if (!structExists(arg.structName))
                    {
                        Debug::log(
                            "@ShaderStageBuilder::beginFunction "
                            "No struct: " + arg.structName + " found!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        return;
                    }
                    useType = arg.structName;
                }

                line += useType + " " + arg.name;

                if (i < args.size() - 1)
                    line += ", ";
            }
            addLine(line + ")");
            addLine("{");
            ++_currentScopeIndentation;
        }

        void ShaderStageBuilder::endFunction(const std::string& returnValueName)
        {
            if (!returnValueName.empty())
            {
                addLine(" return " + returnValueName + ";");
            }
            --_currentScopeIndentation;
            addLine("}");
            endSection();
        }

        void ShaderStageBuilder::beginIf(const std::string& condition)
        {
            addLine("if (" + condition + ")");
            addLine("{");
            ++_currentScopeIndentation;
        }

        void ShaderStageBuilder::beginElseIf(const std::string& condition)
        {
            --_currentScopeIndentation;
            addLine("}");
            addLine("else if (" + condition + ")");
            addLine("{");
            ++_currentScopeIndentation;
        }

        void ShaderStageBuilder::beginElse()
        {
            --_currentScopeIndentation;
            addLine("}");
            addLine("else");
            addLine("{");
            ++_currentScopeIndentation;
        }

        void ShaderStageBuilder::endIf()
        {
            --_currentScopeIndentation;
            addLine("}");
        }

        ShaderObject ShaderStageBuilder::newVariable(
            ShaderDataType type,
            const std::string& name,
            const std::string& value
        )
        {
            const std::string typeName = shader_datatype_to_glsl(type);
            ShaderObject variable = _structDefinitions[typeName];
            variable.name = name;
            reqisterVariable(variable, "");
            addLine(typeName + " " + name + " = " + value + ";");
            return variable;
        }

        void ShaderStageBuilder::endSection()
        {
            addLine("");
        }

        void ShaderStageBuilder::addLine(const std::string& line)
        {
            std::string totalIndentations;
            for (int i = 0; i < _currentScopeIndentation; ++i)
                totalIndentations += _indentation;
            _lines.push_back(totalIndentations + line + "\n");
        }

        bool ShaderStageBuilder::variableExists(const std::string& name) const
        {
            Debug::log("___TEST___searching var: " + name);
            if (_variables.find(name) != _variables.end())
            {
                return true;
            }

            size_t dotPos = name.find(".");

            if (dotPos == name.npos)
                return false;

            std::string next = name.substr(dotPos + 1, name.size() - dotPos);
            return variableExists(next);
        }

        bool ShaderStageBuilder::variablesExists(
            const std::vector<std::string>& toSearch,
            std::vector<std::string>& missingVariables
        ) const
        {
            for (const std::string& name : toSearch)
            {
                if (!variableExists(name))
                    missingVariables.push_back(name);
            }
            return missingVariables.empty();
        }

        bool ShaderStageBuilder::structExists(const std::string& name) const
        {
            return _structDefinitions.find(name) != _structDefinitions.end();
        }

        bool ShaderStageBuilder::structMemberExists(
            const ShaderObject& shaderStruct,
            const std::string memberName
        ) const
        {
            return shaderStruct.memberIndices.find(memberName) != shaderStruct.memberIndices.end();
        }

        void ShaderStageBuilder::calcFinalVertexPosition()
        {
            // If skinning related stuff exists, make sure they ALL exist!
            std::vector<std::string> skinningDataNames = {
                s_inVertex.weights,
                s_inVertex.jointIDs,
                s_uJoint.jointMatrices
            };
            std::vector<std::string> missingSkinningData;
            bool doSkinning = variablesExists(skinningDataNames, missingSkinningData);
            if (!missingSkinningData.empty() && (missingSkinningData != skinningDataNames))
            {
                error(
                    "Some skinning data found, but missing:\n" + vector_to_string(missingSkinningData) + ""
                    "Skinning requires providing all:\n" + vector_to_string(skinningDataNames) + " "
                    "not just some of them!",
                    PLATYPUS_CURRENT_FUNC_NAME
                );
            }

            if (doSkinning)
            {
                if (variableExists(s_global.transformationMatrix))
                {
                    error(
                        "Shader was using vertex weights but transformationMatrix already exists! "
                        "Doing skinning constructs the transformation matrix so it can't exist already.",
                        PLATYPUS_CURRENT_FUNC_NAME
                    );
                }
                // Calc final transform according to used joints and weights.
                // For some reason I've been defaulting to first joint,
                // if weights sum < 1.0. Don't know if this should even be done...
                newVariable(
                    ShaderDataType::Float,
                    "weightsSum",
                    "vertex.weights[0] + vertex.weights[1] + vertex.weights[2] + vertex.weights[3]"
                );
                newVariable(
                    ShaderDataType::Mat4,
                    s_global.transformationMatrix,
                    s_uJoint.jointMatrices + "[int(" + s_inVertex.jointIDs + "[0])]"
                );
                beginIf("weightSum >= 1.0");
                {
                    for (size_t i = 0; i < _maxJointsPerVertex; ++i)
                    {
                        // Assign first and add the rest since the current isn't correct!
                        std::string oper = i == 0 ? " = " : " += ";
                        addLine(
                            s_global.transformationMatrix + oper +
                            s_uJoint.jointMatrices + "[int(" + s_inVertex.jointIDs + "[" + std::to_string(i) + "])] * " + s_inVertex.weights + "[" + std::to_string(i) + "];"
                        );
                    }
                }endIf();
            }
            validateVariable(s_uSceneData.projectionMatrix, PLATYPUS_CURRENT_FUNC_NAME);
            validateVariable(s_uSceneData.viewMatrix, PLATYPUS_CURRENT_FUNC_NAME);
            validateVariable(s_global.transformationMatrix, PLATYPUS_CURRENT_FUNC_NAME);

            newVariable(
                ShaderDataType::Float4,
                s_global.vertexWorldPosition,
                s_global.transformationMatrix + " * vec4" + s_inVertex.position + ", 1.0)"
            );
            newVariable(
                ShaderDataType::Mat4,
                s_global.toCameraSpace,
                s_uSceneData.viewMatrix + " * " + s_global.transformationMatrix
            );

            addLine(
                "gl_Position = " +
                s_uSceneData.projectionMatrix + " * " +
                s_uSceneData.viewMatrix + " * " +
                s_global.vertexWorldPosition + ";"
            );

            endSection();
        }

        void ShaderStageBuilder::calcVertexShaderOutput()
        {
            if (variableExists(s_inVertex.tangent))
            {
                if (!variableExists(s_global.toCameraSpace))
                {
                    error(
                        "Variable: " + s_global.toCameraSpace + " required for shaders using "
                        "tangent vertex attribute. Currently this is created by "
                        "calcFinalVertexPosition(). Make sure you called it before this function!",
                        PLATYPUS_CURRENT_FUNC_NAME
                    );
                }
                if (!variableExists(s_global.vertexWorldPosition))
                {
                    error(
                        "Variable: " + s_global.vertexWorldPosition + " required for shaders using "
                        "tangent vertex attribute. Currently this is created by "
                        "calcFinalVertexPosition(). Make sure you called it before this function!",
                        PLATYPUS_CURRENT_FUNC_NAME
                    );
                }
                // Create toTangentSpace matrix and calc toCamera and lightDir vectors
                // in tangent space
                newVariable(
                    ShaderDataType::Float3,
                    "transformedNormal",
                    std::format(
                        "normalize({} * vec4({}, 0.0)).xyz",
                        s_global.toCameraSpace,
                        s_inVertex.normal
                    )
                );
                newVariable(
                    ShaderDataType::Float3,
                    "transformedTangent",
                    std::format(
                        "normalize(({} * vec4({}.xyz, 0.0)).xyz)",
                        s_global.toCameraSpace,
                        s_inVertex.tangent
                    )
                );
                // T = normalize(T - dot(T, N) * N);
                addLine(
                    "transformedTangent = normalize("
                    "transformedTangent - "
                    "dot(transformedTangent, transformedNormal) * "
                    "transformedNormal);"
                );
                addLine(
                    "vec3 biTangent = normalize(cross(transformedNormal, transformedTangent));"
                );
                addLine(
                    "mat3 toTangentSpace = transpose("
                    "mat3(transformedTangent, biTangent, transformedNormal)"
                    ");"
                );
                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.toCamera,
                    std::format(
                        "normalize("
                        "toTangentSpace * (({}) * vec4({}, 0.0)).xyz"
                        ")",
                        s_uSceneData.viewMatrix,
                        s_uSceneData.cameraPosition + ".xyz - " + s_global.vertexWorldPosition + ".xyz"
                    )
                );
                // NOTE: This probably results in unit vec, but why not normalize just in case?
                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.lightDirection,
                    std::format(
                        "toTangentSpace * ("
                        "{} * vec4({}.xyz, 0.0)"
                        ").xyz",
                        s_uSceneData.viewMatrix,
                        s_uSceneData.lightDirection
                    )
                );
            }
            else
            {
                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.position,
                    s_global.vertexWorldPosition + ".xyz"
                );
                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.normal,
                    "(" + s_global.transformationMatrix + " * vec4(" + s_inVertex.normal + ", 0.0)).xyz"
                );
                addOutput(
                    ShaderDataType::Float2,
                    s_outVertex.texCoord,
                    s_inVertex.texCoord
                );

                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.cameraPosition,
                    s_uSceneData.cameraPosition + ".xyz"
                );
                addOutput(
                    ShaderDataType::Float3,
                    s_outVertex.lightDirection,
                    s_uSceneData.lightDirection
                );
            }

            addOutput(
                ShaderDataType::Float4,
                s_outVertex.lightColor,
                s_uSceneData.lightColor
            );
            addOutput(
                ShaderDataType::Float4,
                s_outVertex.ambientLightColor,
                s_uSceneData.ambientLightColor
            );

            endSection();
        }

        void ShaderStageBuilder::calcTextureColors()
        {
            // TODO: Blending!
            if (_uMaterial.textures.find(TextureChannel::Blendmap) != _uMaterial.textures.end())
            {
            }
            else
            {
                validateVariable(s_global.useTexCoord, PLATYPUS_CURRENT_FUNC_NAME);
                std::unordered_map<TextureChannel, std::vector<std::string>>::const_iterator it;
                for (it = _uMaterial.textures.begin(); it != _uMaterial.textures.end(); ++it)
                {
                    for (const std::string& texture : it->second)
                    {
                        ShaderObject textureColor = newVariable(
                            ShaderDataType::Sampler2D,
                            texture + "Color",
                            std::format(
                                "texture({}, {});",
                                texture,
                                s_global.useTexCoord
                            )
                        );
                    }
                }
            }
        }

        void ShaderStageBuilder::printVariables()
        {
            Debug::log("Shader variables:");
            std::string allVariables;
            for (auto variable : _variables)
            {
                allVariables += "    " + variable.first + "\n";
            }
            Debug::log(allVariables);
        }

        void ShaderStageBuilder::printObjects()
        {
            Debug::log("Shader objects:");
            std::string allStructs;
            for (auto structDefinition : _structDefinitions)
            {
                allStructs += "    " + structDefinition.first + "\n";
                for (auto member : structDefinition.second.memberIndices)
                    allStructs += "        " + member.first + "\n";

            }
            Debug::log(allStructs);
        }

        void ShaderStageBuilder::error(const std::string& msg, const char* funcName) const
        {
            std::string funcNameStr(funcName);
            Debug::log(
                "@ShaderStageBuilder::" + funcNameStr + " " + msg,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        void ShaderStageBuilder::validateVariable(const std::string& variable, const char* funcName)
        {
            if (!variableExists(variable))
            {
                error(
                    "No variable: " + variable + " found!",
                    funcName
                );
                PLATYPUS_ASSERT(false);
            }
        }
    }
}
