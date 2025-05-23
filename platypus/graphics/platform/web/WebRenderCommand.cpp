#include "platypus/graphics/RenderCommand.h"
#include "platypus/graphics/Context.h"
#include "WebContext.h"
#include "platypus/graphics/CommandBuffer.h"
#include "WebCommandBuffer.h"
#include "platypus/graphics/Buffers.h"
#include "WebBuffers.h"
#include "platypus/assets/Texture.h"
#include "platypus/assets/platform/web/WebTexture.h"
#include "platypus/utils/Maths.h"
#include "platypus/Common.h"
#include <GL/glew.h>


namespace platypus
{
    namespace render
    {
        // TODO: On context creation -> get texture unit limits and make this take it into account!
        static GLenum binding_to_gl_texture_unit(uint32_t binding)
        {
            switch (binding)
            {
                case 0:
                    return GL_TEXTURE0;
                case 1:
                    return GL_TEXTURE1;
                case 2:
                    return GL_TEXTURE2;
                case 3:
                    return GL_TEXTURE3;
                case 4:
                    return GL_TEXTURE4;
                case 5:
                    return GL_TEXTURE5;
                case 6:
                    return GL_TEXTURE6;
                case 7:
                    return GL_TEXTURE7;
                case 8:
                    return GL_TEXTURE8;
                case 9:
                    return GL_TEXTURE9;
                default:
                    Debug::log(
                        "@binding_to_gl_texture_unit "
                        "Max texture unit(" + std::to_string(9) + " exceeded using binding: " + std::to_string(binding),
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    return 0;
            }
        }


        GLenum index_type_to_glenum(IndexType type)
        {
            switch (type)
            {
                case IndexType::INDEX_TYPE_NONE:
                    Debug::log(
                        "@index_type_to_glenum IndexType was not set",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    return 0;
                case IndexType::INDEX_TYPE_UINT16:
                    return GL_UNSIGNED_SHORT;
                case IndexType::INDEX_TYPE_UINT32:
                    return GL_UNSIGNED_INT;
                default:
                    Debug::log(
                        "@index_type_to_glenum Invalid index type",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    return 0;
            }
        }


        void begin_render_pass(
            const CommandBuffer& primaryCmdBuf,
            const Swapchain& swapchain,
            const Vector4f& clearColor,
            bool clearDepthBuffer
        )
        {
            GL_FUNC(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
            GL_FUNC(glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a));
        }

        void end_render_pass(const CommandBuffer& commandBuffer)
        {
        }

        void exec_secondary_command_buffers(
            const CommandBuffer& primary,
            const std::vector<CommandBuffer>& secondaries
        )
        {
        }

        void bind_pipeline(
            CommandBuffer& commandBuffer,
            const Pipeline& pipeline
        )
        {
            PipelineImpl* pPipelineImpl = pipeline.getImpl();
            ((CommandBufferImpl*)commandBuffer.getImpl())->pPipelineImpl = pPipelineImpl;

            FrontFace frontFace = pPipelineImpl->frontFace;
            GL_FUNC(glFrontFace(frontFace == FrontFace::FRONT_FACE_COUNTER_CLOCKWISE ? GL_CCW : GL_CW));

            GL_FUNC(glUseProgram(pPipelineImpl->pShaderProgram->getID()));

            if (pPipelineImpl->enableDepthTest)
            {
                GL_FUNC(glEnable(GL_DEPTH_TEST));
            }
            else
            {
                GL_FUNC(glDisable(GL_DEPTH_TEST));
            }

            switch(pPipelineImpl->depthCmpOp)
            {
                case DepthCompareOperation::COMPARE_OP_NEVER:
                    GL_FUNC(glDepthFunc(GL_NEVER));
                    break;
                case DepthCompareOperation::COMPARE_OP_LESS:
                    GL_FUNC(glDepthFunc(GL_LESS));
                    break;
                case DepthCompareOperation::COMPARE_OP_EQUAL:
                    GL_FUNC(glDepthFunc(GL_EQUAL));
                    break;
                case DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL:
                    GL_FUNC(glDepthFunc(GL_LEQUAL));
                    break;
                case DepthCompareOperation::COMPARE_OP_GREATER:
                    GL_FUNC(glDepthFunc(GL_GREATER));
                    break;
                case DepthCompareOperation::COMPARE_OP_NOT_EQUAL:
                    GL_FUNC(glDepthFunc(GL_NOTEQUAL));
                    break;
                case DepthCompareOperation::COMPARE_OP_GREATER_OR_EQUAL:
                    GL_FUNC(glDepthFunc(GL_GEQUAL));
                    break;
                case DepthCompareOperation::COMPARE_OP_ALWAYS:
                    GL_FUNC(glDepthFunc(GL_ALWAYS));
                    break;
            }

            switch (pPipelineImpl->cullMode)
            {
                case CullMode::CULL_MODE_NONE:
                    GL_FUNC(glDisable(GL_CULL_FACE));
                    break;
                case CullMode::CULL_MODE_FRONT:
                    GL_FUNC(glEnable(GL_CULL_FACE));
                    GL_FUNC(glCullFace(GL_FRONT));
                    break;
                case CullMode::CULL_MODE_BACK:
                    GL_FUNC(glEnable(GL_CULL_FACE));
                    GL_FUNC(glCullFace(GL_BACK));
                    break;
                default:
                    Debug::log(
                        "@bind_pipeline invalid pipeline cull mode",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    break;
            }

            if (pPipelineImpl->enableColorBlending)
            {
                GL_FUNC(glEnable(GL_BLEND));
                // TODO: allow specifying this
                GL_FUNC(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            }
            else
            {
                GL_FUNC(glDisable(GL_BLEND));
            }
        }

        // NOTE: With Vulkan, if dynamic state not set for pipeline this needs to be called
        // BEFORE BINDING THE PIPELINE!
        void set_viewport(
            const CommandBuffer& commandBuffer,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight,
            float viewportMinDepth,
            float viewportMaxDepth
        )
        {
            GL_FUNC(glViewport(viewportX, viewportY, (int)viewportWidth, (int)viewportHeight));
        }

        void set_scissor(
            const CommandBuffer& commandBuffer,
            Rect2D scissor
        )
        {
            // Scissor isn't used for now so whole window is included
        }

        void bind_vertex_buffers(
            const CommandBuffer& commandBuffer,
            const std::vector<const Buffer*>& vertexBuffers
        )
        {
            PipelineImpl* pPipelineImpl = commandBuffer.getImpl()->pPipelineImpl;

            const OpenglShaderProgram* pShaderProgram = pPipelineImpl->pShaderProgram;
            const std::vector<int32_t>& shaderAttribLocations = pShaderProgram->getAttribLocations();
            const std::vector<VertexBufferLayout>& vbLayouts = pPipelineImpl->vertexBufferLayouts;

            // Not sure if this stuff works here well...
            std::vector<VertexBufferLayout>::const_iterator vbLayoutIt = vbLayouts.begin();

            // Bind the common VAO
            // NOTE: This could probably be done immediately after context creation and never touch again,
            // since we use the same VAO throughout the whole program
            GL_FUNC(glBindVertexArray(Context::get_instance()->getImpl()->vaoID));

            for (const Buffer* pBuffer : vertexBuffers)
            {
                // NOTE: Could maybe remove this kind of checking on release build?
                if (vbLayoutIt == vbLayouts.end())
                    Debug::log(
                        "@bind_vertex_buffers "
                        "No layout exists for inputted buffer",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                if ((pBuffer->getBufferUsage() & BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT) != BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT)
                    Debug::log(
                        "@bind_vertex_buffers "
                        "Buffer wasn't flagged as vertex buffer: " + std::to_string(pBuffer->getBufferUsage()),
                        Debug::MessageType::PLATYPUS_ERROR
                    );

                // NOTE: DANGER! ..again
                uint32_t bufferID = pBuffer->getImpl()->id;
                GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, bufferID));

                // NOTE: Previously did the buffer updating here if the buffer was marked to be updated

                // Currently assuming that each pipeline's vb layout's index
                // corresponds to the order of inputted buffers vector
                // TODO: Some safeguards 'n error handling if this fails
                size_t stride = vbLayoutIt->getStride();
                size_t toNext = 0;
                int32_t lastLocation = 0;
                for (const VertexBufferElement& element : vbLayoutIt->getElements())
                {
                    //int32_t location = shaderAttribLocations[element.getLocation()];

                    // NOTE: If can't find this elem from shader, it may have been ment as mat4
                    //  -> try using last location + i for the matrix' column's indices
                    // NOTE: May cause issues if mat4 is not the last attribute?
                    int32_t location = pShaderProgram->getAttribLocation(element.getLocation());
                    if (location == -1)
                        location = lastLocation + 1;
                    lastLocation = location;

                    ShaderDataType shaderDataType = element.getType();

                    if (shaderDataType != ShaderDataType::Mat4)
                    {
                        GL_FUNC(glEnableVertexAttribArray(location));
                        GL_FUNC(glVertexAttribDivisor(
                            location,
                            vbLayoutIt->getInputRate() == VertexInputRate::VERTEX_INPUT_RATE_INSTANCE ? 1 : 0
                        ));
                        GL_FUNC(glVertexAttribPointer(
                            location,
                            get_shader_datatype_component_count(shaderDataType),
                            to_gl_datatype(shaderDataType),
                            GL_FALSE,
                            stride,
                            (const void*)toNext
                        ));
                        toNext += get_shader_datatype_size(shaderDataType);
                    }
                    // Special case on matrices since on opengl those are set as 4 vec4s
                    // NOTE: Decided to use rather Float4 elements for specifying matrices -> DELETE BELOW else(maybe?)
                    else
                    {
                        for (int i = 0; i < 4; ++i)
                        {
                            GL_FUNC(glEnableVertexAttribArray(location + i));
                            GL_FUNC(glVertexAttribDivisor(
                                location + i,
                                vbLayoutIt->getInputRate() == VertexInputRate::VERTEX_INPUT_RATE_INSTANCE ? 1 : 0
                            ));
                            GL_FUNC(glVertexAttribPointer(
                                location + i,
                                4,
                                to_gl_datatype(ShaderDataType::Float4),
                                GL_FALSE,
                                stride,
                                (const void*)toNext
                            ));
                            toNext += get_shader_datatype_size(ShaderDataType::Float4);
                        }
                    }
                }
                vbLayoutIt++;
            }
        }


        // NOTE: ISSUE!
        // Emscripten website:
        //  This build mode has a limitation that the largest index in client-side index buffer must be smaller than the total number of indices in that buffer!
        //  https://github.com/emscripten-core/emscripten/issues/4214
        void bind_index_buffer(
            const CommandBuffer& commandBuffer,
            const Buffer* indexBuffer
        )
        {
            // quite dumb, but we need to be able to pass this to "drawIndexed" func somehow..
            size_t dataElemSize = indexBuffer->getDataElemSize();
            if (dataElemSize == sizeof(uint16_t))
            {
                ((CommandBufferImpl*)commandBuffer.getImpl())->drawIndexedType = IndexType::INDEX_TYPE_UINT16;
            }
            else if (dataElemSize == sizeof(uint32_t))
            {
                ((CommandBufferImpl*)commandBuffer.getImpl())->drawIndexedType = IndexType::INDEX_TYPE_UINT32;
            }
            else
            {
                Debug::log(
                    "@bind_index_buffer "
                    "Invalid element size: " + std::to_string(dataElemSize),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            GL_FUNC(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getImpl()->id));
        }

        // TODO: Implement!
        void push_constants(
            CommandBuffer& commandBuffer,
            ShaderStageFlagBits shaderStageFlags,
            uint32_t offset,
            uint32_t size,
            const void* pValues,
            std::vector<UniformInfo> glUniformInfo // Only used on opengl side NOTE: Why this passed by value!?
        )
        {
            PipelineImpl* pPipelineImpl = commandBuffer.getImpl()->pPipelineImpl;
            OpenglShaderProgram* pShaderProgram = pPipelineImpl->pShaderProgram;
            const std::vector<int32_t>& shaderUniformLocations = pShaderProgram->getUniformLocations();

            PE_byte* pBuf = (PE_byte*)pValues;
            size_t bufOffset = 0;
            for (const UniformInfo& uInfo : glUniformInfo)
            {
                if (bufOffset >= size)
                {
                    Debug::log(
                        "@push_constants "
                        "Buffer offset out of bounds!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }
                const PE_byte* pCurrentData = pBuf + bufOffset;
                switch (uInfo.type)
                {
                    case ShaderDataType::Int:
                    {
                        int val = *(int*)pCurrentData;
                        GL_FUNC(glUniform1i(shaderUniformLocations[uInfo.locationIndex], val));
                        break;
                    }
                    case ShaderDataType::Float:
                    {
                        float val = *(float*)pCurrentData;
                        GL_FUNC(glUniform1f(shaderUniformLocations[uInfo.locationIndex], val));
                        break;
                    }
                    case ShaderDataType::Float2:
                    {
                        Vector2f vec = *(Vector2f*)pCurrentData;
                        GL_FUNC(glUniform2f(
                            shaderUniformLocations[uInfo.locationIndex],
                            vec.x,
                            vec.y
                        ));
                        break;
                    }
                    case ShaderDataType::Float3:
                    {
                        Vector3f vec = *(Vector3f*)pCurrentData;;
                        GL_FUNC(glUniform3f(
                            shaderUniformLocations[uInfo.locationIndex],
                            vec.x,
                            vec.y,
                            vec.z
                        ));
                        break;
                    }
                    case ShaderDataType::Float4:
                    {
                        Vector4f vec = *(Vector4f*)pCurrentData;
                        GL_FUNC(glUniform4f(
                            shaderUniformLocations[uInfo.locationIndex],
                            vec.x,
                            vec.y,
                            vec.z,
                            vec.w
                        ));
                        break;
                    }
                    case ShaderDataType::Mat4:
                    {
                        Matrix4f matrix = *(Matrix4f*)pCurrentData;
                        GL_FUNC(glUniformMatrix4fv(
                            shaderUniformLocations[uInfo.locationIndex],
                            1,
                            GL_FALSE,
                            (const float*)&matrix
                        ));
                        break;
                    }

                    default:
                        Debug::log(
                            "@push_constants "
                            "Unsupported ShaderDataType. "
                            "Currently implemented types are: "
                            "Int, Float, Float2, Float3, Float4, Mat4",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        break;
                }
                bufOffset += get_shader_datatype_size(uInfo.type);
            }
        }

        void bind_descriptor_sets(
            CommandBuffer& commandBuffer,
            const std::vector<DescriptorSet>& descriptorSets,
            const std::vector<uint32_t>& offsets
        )
        {
            PipelineImpl* pPipelineImpl = commandBuffer.getImpl()->pPipelineImpl;
            OpenglShaderProgram* pShaderProgram = pPipelineImpl->pShaderProgram;
            const std::vector<int32_t>& shaderUniformLocations = pShaderProgram->getUniformLocations();

            for (const DescriptorSet& descriptorSet : descriptorSets)
            {
                const DescriptorSetLayout* pLayout = descriptorSet.getLayout();
                // TODO: Validate that this descriptor set is layout compliant

                const std::vector<const Buffer*>& buffers = descriptorSet.getBuffers();
                const std::vector<const Texture*>& textures = descriptorSet.getTextures();
                // Not to be confused with binding number.
                // This just index of the layout's bindings vector
                int bufferBindingIndex = 0;
                int textureBindingIndex = 0;

                for (const DescriptorSetLayoutBinding& binding : pLayout->getBindings())
                {
                    const std::vector<UniformInfo>& uniformInfo = binding.getUniformInfo();

                    if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                         binding.getType() == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
                    {
                        // TODO: some boundary checking..
                        const Buffer* pBuf = buffers[bufferBindingIndex];
                        const PE_byte* pBufData = (const PE_byte*)pBuf->getData();
                        #ifdef PLATYPUS_DEBUG
                            if (!pBuf)
                            {
                                Debug::log(
                                    "@bind_descriptor_sets "
                                    "Uniform buffer's data was nullptr!",
                                    Debug::MessageType::PLATYPUS_ERROR
                                );
                                PLATYPUS_ASSERT(false);
                            }
                        #endif
                        size_t addDynamicOffset = 0;
                        if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
                        {
                            #ifdef PLATYPUS_DEBUG
                                if (bufferBindingIndex >= offsets.size())
                                {
                                    Debug::log(
                                        "@bind_descriptor_sets "
                                        "Dynamic buffer offset out of bounds! "
                                        "Dynamic offsets provided: " + std::to_string(offsets.size()) + " "
                                        "Current bufferBindingIndex: " + std::to_string(bufferBindingIndex),
                                        Debug::MessageType::PLATYPUS_ERROR
                                    );
                                    PLATYPUS_ASSERT(false);
                                }
                            #endif
                            addDynamicOffset = offsets[bufferBindingIndex];
                        }
                        size_t uboOffset = 0 + addDynamicOffset;
                        for (const UniformInfo& uboInfo : uniformInfo)
                        {
                            size_t valSize = 0;
                            const PE_byte* pCurrentData = pBufData + uboOffset;
                            switch (uboInfo.type)
                            {
                                case ShaderDataType::Int:
                                {
                                    int val = *(int*)pCurrentData;
                                    valSize = sizeof(int);
                                    glUniform1i(shaderUniformLocations[uboInfo.locationIndex], val);
                                    break;
                                }
                                case ShaderDataType::Float:
                                {
                                    float val = *(float*)pCurrentData;
                                    valSize = sizeof(float);
                                    GL_FUNC(glUniform1f(shaderUniformLocations[uboInfo.locationIndex], val));
                                    break;
                                }
                                case ShaderDataType::Float2:
                                {
                                    Vector2f vec;
                                    valSize = sizeof(Vector2f);
                                    memcpy(&vec, pCurrentData, valSize);

                                    GL_FUNC(glUniform2f(
                                        shaderUniformLocations[uboInfo.locationIndex],
                                        vec.x,
                                        vec.y
                                    ));
                                    break;
                                }
                                case ShaderDataType::Float3:
                                {
                                    Vector3f vec;
                                    valSize = sizeof(Vector3f);
                                    memcpy(&vec, pCurrentData, valSize);
                                    GL_FUNC(glUniform3f(
                                        shaderUniformLocations[uboInfo.locationIndex],
                                        vec.x,
                                        vec.y,
                                        vec.z
                                    ));
                                    break;
                                }
                                case ShaderDataType::Float4:
                                {
                                    Vector4f vec;
                                    valSize = sizeof(Vector4f);
                                    memcpy(&vec, pCurrentData, valSize);

                                    GL_FUNC(glUniform4f(
                                        shaderUniformLocations[uboInfo.locationIndex],
                                        vec.x,
                                        vec.y,
                                        vec.z,
                                        vec.w
                                    ));
                                    break;
                                }
                                case ShaderDataType::Mat4:
                                {
                                    valSize = sizeof(Matrix4f) * uboInfo.arrayLen;
                                    for (int i = 0; i < uboInfo.arrayLen; ++i)
                                    {
                                        const PE_byte* pData = pCurrentData + i * sizeof(Matrix4f);
                                        GL_FUNC(glUniformMatrix4fv(
                                            shaderUniformLocations[uboInfo.locationIndex + i],
                                            1,
                                            GL_FALSE,
                                            (const float*)pData
                                        ));
                                    }
                                    break;
                                }

                                default:
                                    Debug::log(
                                        "@bind_descriptor_sets "
                                        "Unsupported ShaderDataType: " + std::to_string(uboInfo.type) + " "
                                        "using location index: " + std::to_string(uboInfo.locationIndex) + " "
                                        "Currently implemented types are: "
                                        "Int, Float, Float2, Float3, Float4, Mat4",
                                        Debug::MessageType::PLATYPUS_ERROR
                                    );
                                    PLATYPUS_ASSERT(false);
                                    break;
                            }
                            uboOffset += valSize;
                        }
                        ++bufferBindingIndex;
                    }
                    else if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        // TODO: some boundary checking..
                        for (const UniformInfo& layoutInfo : uniformInfo)
                        {
                            if (!textures[textureBindingIndex])
                            {
                                Debug::log(
                                    "@bind_descriptor_sets "
                                    "Texture at binding index: " + std::to_string(textureBindingIndex) + " was nullptr",
                                    Debug::MessageType::PLATYPUS_ERROR
                                );
                                PLATYPUS_ASSERT(false);
                            }
                            GL_FUNC(glUniform1i(shaderUniformLocations[layoutInfo.locationIndex], binding.getBinding()));
                            // well following is quite fucking dumb.. dunno how could do this better
                            GL_FUNC(glActiveTexture(binding_to_gl_texture_unit(binding.getBinding())));
                            GL_FUNC(glBindTexture(
                                GL_TEXTURE_2D,
                                textures[textureBindingIndex]->getImpl()->id
                            ));
                            ++textureBindingIndex;
                        }
                    }
                }
            }
        }

        void draw_indexed(
            const CommandBuffer& commandBuffer,
            uint32_t count,
            uint32_t instanceCount
        )
        {
            const IndexType& indexType = commandBuffer.getImpl()->drawIndexedType;
            // NOTE: Don't remember why not giving the ptr to the indices here..
            GL_FUNC(glDrawElementsInstanced(
                GL_TRIANGLES,
                count,
                index_type_to_glenum(indexType),
                0,
                instanceCount
            ));
        }
    }
}
