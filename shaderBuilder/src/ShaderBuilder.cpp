#include "ShaderBuilder.hpp"
#include "platypus/core/Debug.h"
#include <cctype>


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

        static std::string operation_type_to_string(OperationType type)
        {
            switch (type)
            {
                case OperationType::Assign: return "=";
                case OperationType::Add:    return "+";
                case OperationType::Neg:    return "-";
                case OperationType::Mul:    return "*";
                case OperationType::Div:    return "/";
                default:{
                    Debug::log(
                        "@operation_type_to_char "
                        "Invalid operation type",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return "";
                }
            }
        }


        ShaderStageBuilder::ShaderStageBuilder()
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
                    const uint32_t elementLocation = element.getLocation();
                    const ShaderDataType elementType = element.getType();

                    std::string locationStr;
                    if (useLocationByOrder)
                        locationStr = std::to_string(locationIndex);
                    else
                        locationStr = std::to_string(elementLocation);

                    std::string elementTypeStr = shader_datatype_to_glsl(elementType);
                    std::string attributeName = attributeNames[elementIndex];

                    // NOTE: For now all instance data is separate from "actual mesh vertex
                    // attributes"! Currently the instanced vertex attribs are globally accessably.
                    // TODO: separate struct for instanced data?
                    ShaderObject attribObject{ attributeName, elementType };
                    if (layout.getInputRate() == VertexInputRate::VERTEX_INPUT_RATE_VERTEX)
                        vertexAttributes.push_back(attribObject);

                    addLine("layout(location = " + locationStr + ") in " + elementTypeStr + " " + attributeName + ";");

                    // Add the attribute as variable too!
                    if (variableExists(attributeName))
                    {
                        Debug::log(
                            "@ShaderStageBuilder::addVertexAttributes "
                            "Variable: " + attributeName + " already exists!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        return;
                    }
                    _variables[attributeName] = attribObject;
                    // Also add all the components of the attrib as variables.
                    // This is done in order to be able to get anything like:
                    //  someObject.vertex.position.z
                    // Pretty annoying and dumb, I know...
                    if (elementType != ShaderDataType::Mat3 &&
                        elementType != ShaderDataType::Mat4 &&
                        elementType != ShaderDataType::Int &&
                        elementType != ShaderDataType::Float &&
                        elementType != ShaderDataType::Struct &&
                        elementType != ShaderDataType::None
                    )
                    {
                        size_t attribComponentCount = get_shader_datatype_component_count(elementType);
                        for (size_t i = 0; i < attribComponentCount; ++i)
                        {
                            std::string attribMemberNameXYZW = attributeName + "." + vector_component_index_to_name(i, false);
                            std::string attribMemberNameRGBA = attributeName + "." + vector_component_index_to_name(i, true);
                            _variables[attribMemberNameXYZW] = { attribMemberNameXYZW, ShaderDataType::Float };
                            _variables[attribMemberNameRGBA] = { attribMemberNameRGBA, ShaderDataType::Float };
                        }
                    }

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

        void ShaderStageBuilder::addUniformBlock(
            const DescriptorSetLayoutBinding& descriptorSetLayoutBinding,
            const std::vector<std::string>& variableNames,
            const std::string& blockName,
            const std::string& instanceName
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
            ++_currentScopeIndentation;
            for (size_t i = 0; i < uniformInfo.size(); ++i)
            {
                addVariable(uniformInfo[i].type, variableNames[i], "");
            }
            --_currentScopeIndentation;
            endUniformBlock(instanceName);
            ++_descriptorSetCount;

            // Instantiate
            const ShaderObject& blockDefinition = _structDefinitions[blockName];
            for (auto member : blockDefinition.memberIndices)
            {
                const std::string& memberName = member.first;
                std::string fullName = instanceName + "." + memberName;
                // Need to change the internal name of the member to global here..
                //  ...annoying and shit fuck.. this is becomming a mess..
                ShaderObject globalObject = blockDefinition.members[member.second];
                globalObject.name = fullName;
                _variables[fullName] = globalObject;
            }
        }

        // TODO: Allow adding/defining structs as members
        void ShaderStageBuilder::addVariable(
            ShaderDataType type,
            const std::string& name,
            const std::string& structName
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

            addLine(useType + " " + name + ";");
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

        void ShaderStageBuilder::setOutputTEST(
            uint32_t location,
            ShaderDataType type,
            const std::string name
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
            addLine(name + " = TESTING;");
            std::vector<std::string> tempLines;
            tempLines.insert(tempLines.end(), _lines.begin(), _lines.begin() + _nextOutDefinitionPos);
            tempLines.push_back("layout(location = " + std::to_string(location) + ") out " + shader_datatype_to_glsl(type) + " " + name + ";\n");
            size_t nextOutPosition = tempLines.size();
            tempLines.insert(tempLines.end(), _lines.begin() + _nextOutDefinitionPos, _lines.end());
            _nextOutDefinitionPos = nextOutPosition;
            _lines = tempLines;
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


        // TESTING -----
        void ShaderStageBuilder::eval(Operation* pOperation, std::string& line)
        {
            const ShaderObject& object = pOperation->object;

            if (object.type == ShaderDataType::Struct)
            {
                Debug::log(
                    "@ShaderStageBuilder::eval "
                    "Operations aren't supported for non in built structs!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            // If non constant named object and variable with the name not found?
            // (if non named object, it means the object is temporary and ignore this)
            //  -> instantiate new var if assignment operation
            //  -> error if assignment operation
            if (!object.name.empty() && !is_number(object.name))
            {
                if (!variableExists(object.name))
                {
                    if (pOperation->type != OperationType::Assign)
                    {
                        Debug::log(
                            "@ShaderStageBuilder::eval "
                            "Variable: " + object.name + " not found!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        return;
                    }
                    reqisterVariable(object, "");
                    line += shader_datatype_to_glsl(object.type) + " ";
                }
            }

            line += pOperation->object.name;
            if (pOperation->type != OperationType::None)
            {
                line += " " + operation_type_to_string(pOperation->type) + " ";
            }

            if (pOperation->pRight)
            {
                eval(pOperation->pRight.get(), line);
            }
            else
            {
                addLine(line + ";");
            }
        }

        void ShaderStageBuilder::calcVertexWorldPosition()
        {
            ShaderObject finalPos = _structDefinitions["vec4"];
            finalPos.name = "finalPos";

            Operation testOper{
                finalPos,
                OperationType::Assign,
                newOper(
                    _variables["sceneData.projectionMatrix"],
                    OperationType::Mul,
                    newOper(
                        _variables["sceneData.viewMatrix"],
                        OperationType::Mul,
                        newOper(
                            _variables["vertex.position"],
                            OperationType::None
                        )
                    )
                )
            };

            std::string testLine;
            eval(&testOper, testLine);

            // ---
            //ShaderObject worldSpaceVertex = newVec4(_varNameVertexWorldSpace);
            //ShaderObject transformationMatrix = _variables["transformationMatrix"];
            //ShaderObject vertexPos = _variables["vertex.position"];
            //addLine(
            //    newVarAssign(ShaderDataType::Float4, _varNameVertexWorldSpace) + " ="
            //    " "+ transformationMatrix.name + " * " + vertexPos.name
            //);

            endSection();
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


        VulkanShaderStageBuilder::~VulkanShaderStageBuilder()
        {
        }

        void VulkanShaderStageBuilder::beginPushConstants()
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
            addLine("layout(push_constant) uniform PushConstants");
            addLine("{");
            _pActiveWriteObject = &_structDefinitions[_pushConstantsStructName];
        }

        void VulkanShaderStageBuilder::endPushConstants(const std::string& instanceName)
        {
            addLine("} " + instanceName + ";");
            endSection();
            _pActiveWriteObject = nullptr;
        }

        void VulkanShaderStageBuilder::beginUniformBlock(const std::string& blockName, uint32_t setIndex, uint32_t binding)
        {
            if (_pActiveWriteObject != nullptr)
            {
                Debug::log(
                    "@VulkanShaderStageBuilder::beginUniformBlock "
                    "_pActiveWriteObject wasn't nullptr! "
                    "Make sure you ended previous object write operation with the appropriate "
                    "end function!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            _structDefinitions[blockName] = { blockName, ShaderDataType::Struct };
            addLine("layout(set = " + std::to_string(setIndex) + ", binding = " + std::to_string(binding) + ") uniform " + blockName);
            addLine("{");
            _pActiveWriteObject = &_structDefinitions[blockName];
        }

        void VulkanShaderStageBuilder::endUniformBlock(const std::string& instanceName)
        {
            addLine("} " + instanceName + ";");
            endSection();
            _pActiveWriteObject = nullptr;
        }
    }
}
