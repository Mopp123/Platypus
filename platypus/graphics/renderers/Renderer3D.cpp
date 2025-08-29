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

        for (Batch* pBatch : toRender)
        {
            for (uint32_t repeatIndex = 0; repeatIndex < pBatch->repeatCount; ++repeatIndex)
            {
                // DANGER! Might dereference nullptr!
                render::bind_pipeline(
                    currentCommandBuffer,
                    *pBatch->pPipeline
                );

                render::bind_vertex_buffers(
                    currentCommandBuffer,
                    pBatch->vertexBuffers
                );
                render::bind_index_buffer(currentCommandBuffer, pBatch->pIndexBuffer);

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

                if (pBatch->dynamicDescriptorSetRanges.empty())
                {
                    render::bind_descriptor_sets(
                        currentCommandBuffer,
                        pBatch->descriptorSets[_currentFrame],
                        { }
                    );
                }
                else
                {
                    render::bind_descriptor_sets(
                        currentCommandBuffer,
                        pBatch->descriptorSets[_currentFrame],
                        { pBatch->dynamicDescriptorSetRanges[repeatIndex] }
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
