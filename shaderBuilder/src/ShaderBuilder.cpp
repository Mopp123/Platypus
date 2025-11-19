#include "ShaderBuilder.hpp"
#include "platypus/core/Debug.h"


namespace platypus
{
    namespace shaderBuilder
    {
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

                case Mat4: return "mat4";

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


        void ShaderStageBuilder::addVersion(ShaderVersion version)
        {
            if (version == ShaderVersion::VULKAN_GLSL_450)
            {
                addLine("#version 450");
            }
            else if (version == ShaderVersion::OPENGLES_GLSL_300)
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
                    const uint32_t elementLocation = element.getLocation();
                    const ShaderDataType elementType = element.getType();
                    std::string locationStr;
                    if (useLocationByOrder)
                        locationStr = std::to_string(locationIndex);
                    else
                        locationStr = std::to_string(elementLocation);

                    std::string elementTypeStr = shader_datatype_to_glsl(elementType);
                    std::string attributeName = attributeNames[elementIndex];

                    addLine("layout(location = " + locationStr + ") in " + elementTypeStr + " " + attributeName + ";");

                    // Consumes 4 locations, if mat4
                    if (useLocationByOrder && elementType == ShaderDataType::Mat4)
                        locationIndex += 4;
                    else
                        ++locationIndex;
                }
                ++layoutIndex;
            }
            endSection();
        }

        void ShaderStageBuilder::addPushConstants(
            const std::vector<UniformInfo>& pushConstantsInfo,
            const std::vector<std::string>& variableNames
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

            beginPushConstants();
            for (size_t i = 0; i < pushConstantsInfo.size(); ++i)
            {
                const UniformInfo& uniformInfo = pushConstantsInfo[i];
                addVariable(uniformInfo.type, variableNames[i], true);
            }
            endPushConstants("shadowPushConstants");
        }

        void ShaderStageBuilder::addUniformBlock(
            const DescriptorSetLayoutBinding& descriptorSetLayoutBinding,
            const std::vector<std::string>& variableNames,
            const std::string& blockName,
            const std::string& blockVariableName
        )
        {
            const std::vector<UniformInfo>& uniformInfo = descriptorSetLayoutBinding.getUniformInfo();
            if (uniformInfo.size() != variableNames.size())
            {
                Debug::log(
                    "@ShaderStageBuilder::addUniformBlock "
                    "Provided " + std::to_string(variableNames.size()) + " names "
                    "for " + std::to_string(uniformInfo.size()) + " uniform block variables. "
                    "You have to provide matching number of elements in uniform info and variable names!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            beginUniformBlock(blockName, _descriptorSetCount, descriptorSetLayoutBinding.getBinding());
            for (size_t i = 0; i < uniformInfo.size(); ++i)
            {
                addVariable(uniformInfo[i].type, variableNames[i], true);
            }
            endUniformBlock(blockVariableName);
            ++_descriptorSetCount;
        }

        void ShaderStageBuilder::addVariable(ShaderDataType type, const std::string name, bool indent)
        {
            addLine(shader_datatype_to_glsl(type) + " " + name + ";", indent ? 1 : 0);
        }

        void ShaderStageBuilder::beginFunction(
            const std::string& functionName,
            ShaderDataType returnType,
            const std::vector<FunctionInput>& args
        )
        {
            if (functionName == "main")
            {
                _mainFuncBeginRow = _lines.size();
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

                line += shader_datatype_to_glsl(arg.variable.type) + " " + arg.variable.name;

                if (i == args.size() - 1)
                    line += ")";
                else
                    line += ", ";
            }
            addLine(line);
            addLine("{");
        }

        void ShaderStageBuilder::endFunction(ShaderVariable returnValue)
        {
            if (returnValue.type != ShaderDataType::None)
            {
                addLine(_indentation + " return " + returnValue.name + ";");
            }
            addLine("}");
            endSection();
        }

        void ShaderStageBuilder::endSection()
        {
            addLine("");
        }

        void ShaderStageBuilder::addLine(const std::string& line, int indentations)
        {
            std::string totalIndentations = "";
            for (int i = 0; i < indentations; ++i)
                totalIndentations += _indentation;
            _lines.push_back(totalIndentations + line + "\n");
        }

        void ShaderStageBuilder::setOutputTEST(uint32_t location, ShaderVariable variable)
        {
            if (_mainFuncBeginRow == 0)
            {
                Debug::log(
                    "ShaderStageBuilder::setOutputTEST "
                    "Invalid row(" + std::to_string(_mainFuncBeginRow) + ") for main func!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            addLine(variable.name + " = TESTING;");
            std::vector<std::string> tempLines;
            tempLines.insert(tempLines.end(), _lines.begin(), _lines.begin() + _mainFuncBeginRow);
            tempLines.push_back("layout(location = " + std::to_string(location) + ") out " + shader_datatype_to_glsl(variable.type) + " " + variable.name + ";\n");
            tempLines.insert(tempLines.end(), _lines.begin() + _mainFuncBeginRow, _lines.end());
            _lines = tempLines;
        }


        VulkanShaderStageBuilder::~VulkanShaderStageBuilder()
        {
        }

        void VulkanShaderStageBuilder::beginPushConstants()
        {
            addLine("layout(push_constant) uniform PushConstants");
            addLine("{");
        }

        void VulkanShaderStageBuilder::endPushConstants(const std::string& variableName)
        {
            addLine("} " + variableName + ";");
            endSection();
        }

        void VulkanShaderStageBuilder::beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding)
        {
            addLine("layout(set = " + std::to_string(setIndex) + ", binding = " + std::to_string(binding) + ") uniform " + blockName);
        }

        void VulkanShaderStageBuilder::endUniformBlock(const std::string& variableName)
        {
            addLine("} " + variableName + ";");
            endSection();
        }
    }
}
