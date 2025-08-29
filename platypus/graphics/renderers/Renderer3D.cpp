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
        _maxBatches = 10;
        _batches.resize(_maxBatches);
    }

    Renderer3D::~Renderer3D()
    {
    }

    void Renderer3D::submit(Batch* pBatch)
    {
        // Not sure how the batches should be provided here.
        // Atm just experimenting...
        if (_usedBatches >= _maxBatches)
        {
            Debug::log(
                "@Renderer3D::submit "
                "All batches already in use. "
                "Maximum batch count is " + std::to_string(_maxBatches),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        _batches[_usedBatches] = pBatch;
        ++_usedBatches;
    }

    CommandBuffer& Renderer3D::recordCommandBuffer(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight
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
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

        for (const Batch& batch : _batches)
        {
            for (uint32_t repeatIndex = 0; repeatIndex < batch.repeatCount; ++repeatIndex)
            {
                // DANGER! Might dereference nullptr!
                render::bind_pipeline(
                    currentCommandBuffer,
                    *batch.pPipeline
                );

                render::bind_vertex_buffers(
                    currentCommandBuffer,
                    batch.vertexBuffers
                );
                render::bind_index_buffer(currentCommandBuffer, batch.pIndexBuffer);

                render::push_constants(
                    currentCommandBuffer,
                    batch.pushConstantsShaderStage,
                    0,
                    batch.pushConstantsSize,
                    batch.pPushConstantsData,
                    batch.pushConstantsUniformInfos
                );

                if (batch.dynamicDescriptorSetRanges.empty())
                {
                    render::bind_descriptor_sets(
                        currentCommandBuffer,
                        batch.descriptorSets[_currentFrame],
                        { }
                    );
                }
                else
                {
                    render::bind_descriptor_sets(
                        currentCommandBuffer,
                        batch.descriptorSets[_currentFrame],
                        { batch.dynamicDescriptorSetRanges[repeatIndex] }
                    );
                }

                render::draw_indexed(
                    currentCommandBuffer,
                    (uint32_t)batch.pIndexBuffer->getDataLength(),
                    batch.instanceCount
                );
            }
        }

        // NOTE: Recording command buffer shouldn't really be responsible for resetting used batches?
        _usedBatches = 0;

        currentCommandBuffer.end();

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    void Renderer3D::allocCommandBuffers()
    {
        _commandBuffers = Device::get_command_pool()->allocCommandBuffers(
            _masterRendererRef.getSwapchain().getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void Renderer3D::freeCommandBuffers()
    {
        for (CommandBuffer& commandBuffer : _commandBuffers)
            commandBuffer.free();

        _commandBuffers.clear();
    }
}
