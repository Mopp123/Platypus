#include "platypus/graphics/RenderCommand.h"
#include "platypus/graphics/Context.hpp"
#include "platypus/graphics/CommandBuffer.h"
#include "WebContext.hpp"
#include "WebCommandBuffer.h"
#include "platypus/graphics/Buffers.h"
#include "WebFramebuffer.hpp"
#include "WebBuffers.h"
#include "WebDescriptors.hpp"
#include "WebShader.hpp"
#include "platypus/assets/Texture.h"
#include "platypus/assets/platform/web/WebTexture.h"
#include "platypus/utils/Maths.h"
#include "platypus/Common.h"
#include "platypus/core/Application.h"
#include "platypus/graphics/renderers/MasterRenderer.h"
#include <GL/glew.h>


namespace platypus
{
    namespace render
    {
        // TODO: On context creation -> get texture unit limits and make this take it into account!
        static GLenum binding_to_gl_texture_unit(uint32_t binding)
        {
            GLenum textureUnit = GL_TEXTURE0 + binding;
            if (textureUnit >= GL_MAX_TEXTURE_UNITS)
            {
                Debug::log(
                    "@binding_to_gl_texture_unit "
                    "Requested texture unit: " + std::to_string(textureUnit) + " "
                    "exceeded the maximum limit: " + std::to_string(GL_MAX_TEXTURE_UNITS),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return GL_TEXTURE0;
            }
            return textureUnit;
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
            CommandBuffer& commandBuffer,
            const RenderPass& renderPass,
            const Framebuffer* pFramebuffer,
            Texture* pDepthAttachment,
            const Vector4f& clearColor,
            bool clearDepthBuffer
        )
        {
            if (renderPass.isOffscreenPass() && pFramebuffer)
            {
                GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->getImpl()->id));
            }
            else
            {
                GL_FUNC(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            }

            GL_FUNC(glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a));
            GL_FUNC(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        }

        void end_render_pass(CommandBuffer& commandBuffer)
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
            ((CommandBufferImpl*)commandBuffer.getImpl())->pBoundPipeline = &pipeline;

            // Testing getting rid of UniformInfos
            // Reset useLocationIndex from "prev round"
            pPipelineImpl->constantsPushed = false;
            pPipelineImpl->firstDescriptorSetLocation = 0;
            pPipelineImpl->boundUniformBuffers.clear();

            GL_FUNC(glFrontFace(pipeline.getFaceWindingOrder() == FrontFace::FRONT_FACE_COUNTER_CLOCKWISE ? GL_CCW : GL_CW));

            GL_FUNC(glUseProgram(pPipelineImpl->pShaderProgram->getID()));

            if (pipeline.isDepthTestEnabled())
            {
                GL_FUNC(glEnable(GL_DEPTH_TEST));
            }
            else
            {
                GL_FUNC(glDisable(GL_DEPTH_TEST));
            }

            switch(pipeline.getDepthCompareOperation())
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

            switch (pipeline.getCullMode())
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

            if (pipeline.isColorBlendEnabled())
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

        // Creates VAOs for inputted vertexBuffers using inputted vertexBufferLayouts.
        static void create_vao(
            const OpenglShaderProgram* pShaderProgram,
            const std::vector<VertexBufferLayout>& vertexBufferLayouts,
            const std::vector<const Buffer*>& vertexBuffers
        )
        {
            ContextImpl* pContextImpl = Context::get_impl();

            VAOData newVAOData;

            // Not sure if this stuff works here well...
            std::vector<VertexBufferLayout>::const_iterator vbLayoutIt = vertexBufferLayouts.begin();

            uint32_t vaoID = 0;
            std::set<uint32_t> bufferIDs;
            GL_FUNC(glGenVertexArrays(1, &vaoID));
            GL_FUNC(glBindVertexArray(vaoID));

            for (const Buffer* pBuffer : vertexBuffers)
            {
                // NOTE: Could maybe remove this kind of checking on release build?
                if (vbLayoutIt == vertexBufferLayouts.end())
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
                int32_t lastLocation = 0; // NOTE: Not used anymore?!?!
                for (const VertexBufferElement& element : vbLayoutIt->getElements())
                {
                    // If using Mat4 attribute, it uses 4 attrib locations instead of the single one
                    // specified in the element!
                    //  NOTE: May cause issues if mat4 is not the last attribute?
                    //      -> shouldn't be the case anymore since using the actual attrib locations specified in the glsl
                    int32_t location = element.getLocation();
                    //if (pShaderProgram->getAttributeType(location) == ShaderDataType::Mat4)
                    //    location = lastLocation + 1;
                    //lastLocation = location;

                    ShaderDataType shaderDataType = element.getType();

                    if (shaderDataType != ShaderDataType::Mat4)
                    {
                        GL_FUNC(glEnableVertexAttribArray(location));
                        VertexInputRate inputRate = vbLayoutIt->getInputRate();
                        GL_FUNC(glVertexAttribDivisor(
                            location,
                            inputRate == VertexInputRate::VERTEX_INPUT_RATE_INSTANCE ? 1 : 0
                        ));
                        // NOTE: This works atm since currently allow a single buffer to
                        // be used for per vertex or per instance but never both.
                        // TODO: Figure out should that ever change and what then?
                        if (inputRate == VertexInputRate::VERTEX_INPUT_RATE_INSTANCE)
                        {
                            //pContextImpl->complementaryVbos.insert(bufferID);
                            newVAOData.complementaryBufferIDs.insert(bufferID);
                        }

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
                pBuffer->getImpl()->vaos.insert(vaoID);
                newVAOData.bufferIDs.insert(bufferID);
            }
            newVAOData.bufferLayouts = vertexBufferLayouts;
            pContextImpl->vaoDataMapping[vaoID] = newVAOData;
        }


        // Returns a VAO that includes all the inputted vertex buffers if such VAO exists.
        // Returns 0 if no VAO found.
        // TODO: Rename? It's not common since its per buffers...
        static uint32_t get_vao(
            const std::vector<const Buffer*>& vertexBuffers,
            const std::vector<VertexBufferLayout>& vertexBufferLayouts
        )
        {
            std::set<uint32_t> bufferIDs;
            for (const Buffer* pBuffer : vertexBuffers)
                bufferIDs.insert(pBuffer->getImpl()->id);

            // Must be one of these...
            std::unordered_map<uint32_t, VAOData>& vaoDataMapping = Context::get_impl()->vaoDataMapping;
            for (uint32_t vaoID : vertexBuffers[0]->getImpl()->vaos)
            {
                const VAOData& vaoData = vaoDataMapping[vaoID];
                const std::set<uint32_t>& vaoBuffers = vaoData.bufferIDs;
                const std::vector<VertexBufferLayout>& vaoBufferLayouts = vaoData.bufferLayouts;
                if (bufferIDs == vaoBuffers && vaoBufferLayouts == vertexBufferLayouts)
                    return vaoID;
            }
            return 0;
        }


        void bind_vertex_buffers(
            const CommandBuffer& commandBuffer,
            const std::vector<const Buffer*>& vertexBuffers
        )
        {
            const Pipeline* pPipeline = commandBuffer.getImpl()->pBoundPipeline;
            PipelineImpl* pPipelineImpl = pPipeline->getImpl();

            uint32_t vaoID = get_vao(vertexBuffers, pPipeline->getVertexBufferLayouts());
            if (vaoID == 0)
            {
                create_vao(
                    pPipelineImpl->pShaderProgram,
                    pPipeline->getVertexBufferLayouts(),
                    vertexBuffers
                );
            }

            GL_FUNC(glBindVertexArray(vaoID));
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
            // NOTE: Issue when getting rid of UniformInfos -> How to figure out the type of the values?
            std::vector<UniformInfo> glUniformInfo // Only used on opengl side NOTE: Why this passed by value!?
        )
        {
            PipelineImpl* pPipelineImpl = commandBuffer.getImpl()->pBoundPipeline->getImpl();
            OpenglShaderProgram* pShaderProgram = pPipelineImpl->pShaderProgram;
            const std::vector<int32_t>& shaderUniformLocations = pShaderProgram->getUniformLocations();

            PE_byte* pBuf = (PE_byte*)pValues;
            size_t bufOffset = 0;
            int useLocationIndex = 0;
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
                        GL_FUNC(glUniform1i(shaderUniformLocations[useLocationIndex], val));
                        ++useLocationIndex;
                        break;
                    }
                    case ShaderDataType::Float:
                    {
                        float val = *(float*)pCurrentData;
                        GL_FUNC(glUniform1f(shaderUniformLocations[useLocationIndex], val));
                        ++useLocationIndex;
                        break;
                    }
                    case ShaderDataType::Float2:
                    {
                        Vector2f vec = *(Vector2f*)pCurrentData;
                        GL_FUNC(glUniform2f(
                            shaderUniformLocations[useLocationIndex],
                            vec.x,
                            vec.y
                        ));
                        ++useLocationIndex;
                        break;
                    }
                    case ShaderDataType::Float3:
                    {
                        Vector3f vec = *(Vector3f*)pCurrentData;;
                        GL_FUNC(glUniform3f(
                            shaderUniformLocations[useLocationIndex],
                            vec.x,
                            vec.y,
                            vec.z
                        ));
                        ++useLocationIndex;
                        break;
                    }
                    case ShaderDataType::Float4:
                    {
                        Vector4f vec = *(Vector4f*)pCurrentData;
                        GL_FUNC(glUniform4f(
                            shaderUniformLocations[useLocationIndex],
                            vec.x,
                            vec.y,
                            vec.z,
                            vec.w
                        ));
                        ++useLocationIndex;
                        break;
                    }
                    case ShaderDataType::Mat4:
                    {
                        Matrix4f matrix = *(Matrix4f*)pCurrentData;
                        GL_FUNC(glUniformMatrix4fv(
                            shaderUniformLocations[useLocationIndex],
                            1,
                            GL_FALSE,
                            (const float*)&matrix
                        ));
                        ++useLocationIndex;
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
            pPipelineImpl->firstDescriptorSetLocation = useLocationIndex;
            pPipelineImpl->constantsPushed = true;
        }

        void bind_descriptor_sets(
            CommandBuffer& commandBuffer,
            const std::vector<DescriptorSet>& descriptorSets,
            const std::vector<uint32_t>& offsets
        )
        {
            const Pipeline* pBoundPipeline = commandBuffer.getImpl()->pBoundPipeline;
            const std::vector<DescriptorSetLayout>& descriptorSetLayouts = pBoundPipeline->getDescriptorSetLayouts();
            PipelineImpl* pPipelineImpl = pBoundPipeline->getImpl();
            #ifdef PLATYPUS_DEBUG
                if ((pBoundPipeline->getPushConstantsSize() > 0) && !pPipelineImpl->constantsPushed)
                {
                    Debug::log(
                        "@bind_descriptor_sets "
                        "Bound pipeline has push constants that haven't yet been pushed! "
                        "If pipeline has push constants these need to be pushed before binding descriptor sets!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }

                if (descriptorSets.size() != descriptorSetLayouts.size())
                {
                    Debug::log(
                        "@bind_descriptor_sets "
                        "Bound pipeline has " + std::to_string(descriptorSetLayouts.size()) + " "
                        "descriptor set layouts, but " + std::to_string(descriptorSets.size()) + " were provided. "
                        "Currently you have to provide all the descriptor sets for each layout at once!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }
            #endif

            OpenglShaderProgram* pShaderProgram = pPipelineImpl->pShaderProgram;
            const std::vector<int32_t>& shaderUniformLocations = pShaderProgram->getUniformLocations();

            // TODO: Make safer!
            DescriptorPool& descriptorPool = Application::get_instance()->getMasterRenderer()->getDescriptorPool();
            DescriptorPoolImpl* pDescriptorPoolImpl = descriptorPool.getImpl();

            int descriptorSetIndex = 0;
            int useLocationIndex = pPipelineImpl->firstDescriptorSetLocation;
            uint32_t uniformBlockBindingPoint = 0;
            for (const DescriptorSetLayout& descriptorSetLayout : descriptorSetLayouts)
            {
                const ID_t descriptorSetID = descriptorSets[descriptorSetIndex].getImpl()->id;
                int bindingIndex = 0;
                for (const DescriptorSetLayoutBinding& binding : descriptorSetLayout.getBindings())
                {
                    const std::vector<UniformInfo>& uniformInfo = binding.getUniformInfo();
                    DescriptorSetComponent* pDescriptorSetComponent = get_pool_descriptor_set_data(pDescriptorPoolImpl, descriptorSetID, bindingIndex);

                    DescriptorType bindingType = binding.getType();

                    if (bindingType == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    {
                        const Buffer* pUniformBuffer = (const Buffer*)pDescriptorSetComponent->pData;
                        std::set<uint32_t>& boundUniformBuffers = pPipelineImpl->boundUniformBuffers;
                        uint32_t uniformBufferID = pUniformBuffer->getImpl()->id;
                        if (boundUniformBuffers.find(uniformBufferID) == boundUniformBuffers.end())
                        {
                            GL_FUNC(glBindBufferBase(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, uniformBufferID));
                            boundUniformBuffers.insert(uniformBufferID);
                            ++uniformBlockBindingPoint;
                        }
                        ++useLocationIndex;
                    }
                    else if (bindingType == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER)
                    {
                        ++useLocationIndex;
                    }
                    else if (binding.getType() == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        // TODO: some boundary checking..
                        for (const UniformInfo& layoutInfo : uniformInfo)
                        {
                            GL_FUNC(glUniform1i(shaderUniformLocations[useLocationIndex], binding.getBinding()));
                            // well following is quite fucking dumb.. dunno how could do this better
                            GL_FUNC(glActiveTexture(binding_to_gl_texture_unit(binding.getBinding())));
                            const Texture* pTexture = (const Texture*)pDescriptorSetComponent->pData;
                            PLATYPUS_ASSERT(pTexture);
                            PLATYPUS_ASSERT(pTexture->getImpl());
                            GL_FUNC(glBindTexture(
                                GL_TEXTURE_2D,
                                pTexture->getImpl()->id
                            ));
                            ++useLocationIndex;
                        }
                    }
                    ++bindingIndex;
                }
                ++descriptorSetIndex;
            }
            pPipelineImpl->constantsPushed = false;
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
