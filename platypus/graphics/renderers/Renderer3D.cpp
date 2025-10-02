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

    // NOTE: Just experimenting atm
    // TODO: Clean this shit up
    CommandBuffer& Renderer3D::recordOffscreenCommandBuffer(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        const std::vector<Batch*>& toRender
    )
    {
        #ifdef PLATYPUS_DEBUG
            if (_currentFrame >= _offscreenCommandBuffers.size())
            {
                Debug::log(
                    "@Renderer3D::recordOffscreenCommandBuffer "
                    "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                    "Allocated command buffer count is " + std::to_string(_offscreenCommandBuffers.size()),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        CommandBuffer& currentCommandBuffer = _offscreenCommandBuffers[_currentFrame];
        currentCommandBuffer.begin(&renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

        const size_t currentFrame = _masterRendererRef.getCurrentFrame();
        for (Batch* pBatch : toRender)
        {
            if (!pBatch->pOffscreenPipeline)
                continue;

            // DANGER! Might dereference nullptr!
            render::bind_pipeline(
                currentCommandBuffer,
                *pBatch->pOffscreenPipeline
            );

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

    CommandBuffer& Renderer3D::recordCommandBuffer(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        const std::vector<Batch*>& toRender
    )
    {
        #ifdef PLATYPUS_DEBUG
            if (_currentFrame >= _commandBuffers.size())
            {
                Debug::log(
                    "@Renderer3D::recordCommandBuffer "
                    "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                    "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(&renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

        const size_t currentFrame = _masterRendererRef.getCurrentFrame();
        for (Batch* pBatch : toRender)
        {
            // DANGER! Might dereference nullptr!
            render::bind_pipeline(
                currentCommandBuffer,
                *pBatch->pPipeline
            );

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
        _commandBuffers = Device::get_command_pool()->allocCommandBuffers(
            _masterRendererRef.getSwapchain().getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
        _offscreenCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            _masterRendererRef.getSwapchain().getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void Renderer3D::freeCommandBuffers()
    {
        for (CommandBuffer& commandBuffer : _commandBuffers)
            commandBuffer.free();

        for (CommandBuffer& commandBuffer : _offscreenCommandBuffers)
            commandBuffer.free();

        _commandBuffers.clear();
        _offscreenCommandBuffers.clear();
    }
}
