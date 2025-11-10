#include "Renderer3D.hpp"
#include "MasterRenderer.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    Renderer3D::Renderer3D(MasterRenderer& masterRendererRef) :
        _masterRendererRef(masterRendererRef)
    {
    }

    Renderer3D::~Renderer3D()
    {
    }

    CommandBuffer& Renderer3D::recordCommandBuffer(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        const std::vector<Batch*>& toRender
    )
    {
        RenderPassType renderPassType = renderPass.getType();
        if (_commandBuffers.find(renderPassType) == _commandBuffers.end())
        {
            Debug::log(
                "@Renderer3D::recordCommandBuffer "
                "No allocated command buffers found for render pass type: " + render_pass_type_to_string(renderPassType),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        if (_currentFrame >= _commandBuffers[renderPassType].size())
        {
            Debug::log(
                "@Renderer3D::recordCommandBuffer "
                "Frame index(" + std::to_string(_currentFrame) + ") out of bounds for render pass type: " + render_pass_type_to_string(renderPassType) + " "
                "Allocated command buffer count is " + std::to_string(_commandBuffers[renderPassType].size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _commandBuffers[renderPass.getType()][_currentFrame];
        currentCommandBuffer.begin(&renderPass);

        const size_t currentFrame = _masterRendererRef.getCurrentFrame();
        for (Batch* pBatch : toRender)
        {
            // DANGER! Might dereference nullptr!
            render::bind_pipeline(
                currentCommandBuffer,
                *pBatch->pPipeline
            );
            render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

            // TODO: Fix this mess!
            //      * Should have better way of grouping all
            // TODO: Don't alloc each time here!
            std::vector<const Buffer*> vertexBuffers;
            vertexBuffers.reserve(pBatch->staticVertexBuffers.size() + pBatch->dynamicVertexBuffers.size());
            for (const Buffer* pBuffer : pBatch->staticVertexBuffers)
                vertexBuffers.emplace_back(pBuffer);
            for (std::vector<Buffer*>& dynamicVertexBuffers : pBatch->dynamicVertexBuffers)
            {
                // ISSUE: DANGER! Make sure not accessing outside bounds!
                Buffer* pDynamicVertexBuffer = dynamicVertexBuffers[currentFrame];
                vertexBuffers.emplace_back(pDynamicVertexBuffer);
            }

            render::bind_vertex_buffers(
                currentCommandBuffer,
                vertexBuffers
            );
            render::bind_index_buffer(currentCommandBuffer, pBatch->pIndexBuffer);

            for (uint32_t repeatIndex = 0; repeatIndex < pBatch->repeatCount; ++repeatIndex)
            {
                if (pBatch->pushConstantsSize > 0)
                {
                    render::push_constants(
                        currentCommandBuffer,
                        pBatch->pushConstantsShaderStage,
                        0,
                        pBatch->pushConstantsSize,
                        pBatch->pPushConstantsData,
                        pBatch->pushConstantsUniformInfos
                    );
                }

                if (!pBatch->descriptorSets.empty())
                {
                    if (pBatch->dynamicUniformBufferElementSize == 0)
                    {
                        render::bind_descriptor_sets(
                            currentCommandBuffer,
                            pBatch->descriptorSets[_currentFrame],
                            { }
                        );
                    }
                    else
                    {
                        uint32_t dynamicUniformBufferOffset = repeatIndex * pBatch->dynamicUniformBufferElementSize;
                        render::bind_descriptor_sets(
                            currentCommandBuffer,
                            pBatch->descriptorSets[_currentFrame],
                            { dynamicUniformBufferOffset }
                        );
                    }
                }

                render::draw_indexed(
                    currentCommandBuffer,
                    (uint32_t)pBatch->pIndexBuffer->getDataLength(),
                    pBatch->instanceCount
                );
            }
        }

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    void Renderer3D::advanceFrame()
    {
        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;
    }

    void Renderer3D::allocCommandBuffers()
    {
        _commandBuffers[RenderPassType::SCENE_PASS] = Device::get_command_pool()->allocCommandBuffers(
            _masterRendererRef.getSwapchain().getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
        _commandBuffers[RenderPassType::SHADOW_PASS] = Device::get_command_pool()->allocCommandBuffers(
            _masterRendererRef.getSwapchain().getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void Renderer3D::freeCommandBuffers()
    {
        std::unordered_map<RenderPassType, std::vector<CommandBuffer>>::iterator it;
        for (it = _commandBuffers.begin(); it != _commandBuffers.end(); ++it)
        {
            for (CommandBuffer& commandBuffer : it->second)
                commandBuffer.free();
        }

        _commandBuffers.clear();
    }
}
