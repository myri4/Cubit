#pragma once

#include "VulkanContext.h"
#include "Commands.h"

constexpr uint32_t FRAME_OVERLAP = 2;
constexpr uint64_t MAX_PASSES = 200;
inline uint8_t CURRENT_FRAME = 0;

namespace SyncContext
{
	inline wc::Semaphore RenderSemaphores[FRAME_OVERLAP], PresentSemaphores[FRAME_OVERLAP], ImageAvaibleSemaphores[FRAME_OVERLAP];
	inline wc::TimelineSemaphore m_TimelineSemaphore;
	inline uint64_t m_TimelineValue = 0;

	inline wc::Fence RenderFences[FRAME_OVERLAP];
	inline wc::Fence ComputeFences[FRAME_OVERLAP];
	inline wc::Fence ImmediateFence;

	inline wc::CommandBuffer MainCommandBuffers[FRAME_OVERLAP];
	inline wc::CommandBuffer ComputeCommandBuffers[FRAME_OVERLAP];
	inline wc::CommandBuffer UploadCommandBuffer;

	inline wc::CommandPool CommandPool;
	inline wc::CommandPool ComputeCommandPool;
	inline wc::CommandPool UploadCommandPool;

	inline void Create()
	{
		CommandPool.Create(VulkanContext::GraphicsQueue.GetFamily());
		ComputeCommandPool.Create(VulkanContext::ComputeQueue.GetFamily());
		UploadCommandPool.Create(VulkanContext::GraphicsQueue.GetFamily());

		UploadCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, UploadCommandBuffer);
		ImmediateFence.Create();

		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			RenderSemaphores[i].Create(std::format("RenderSemaphore[{}]", i));
			PresentSemaphores[i].Create(std::format("PresentSemaphore[{}]", i));
			ImageAvaibleSemaphores[i].Create(std::format("ImageAvaibleSemaphore[{}]", i));

			CommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, MainCommandBuffers[i]);
			ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, ComputeCommandBuffers[i]);

			MainCommandBuffers[i].SetName(std::format("MainCommandBuffer[{}]", i));
			ComputeCommandBuffers[i].SetName(std::format("ComputeCommandBuffer[{}]", i));

			RenderFences[i].Create(VK_FENCE_CREATE_SIGNALED_BIT);
			ComputeFences[i].Create(/*VK_FENCE_CREATE_SIGNALED_BIT?*/);
		}

		m_TimelineSemaphore.Create("SyncContext::m_TimelineSemaphore");
	}

	inline auto& GetRenderSemaphore() { return RenderSemaphores[CURRENT_FRAME]; }
	inline auto& GetPresentSemaphore() { return PresentSemaphores[CURRENT_FRAME]; }
	inline auto& GetImageAvaibleSemaphore() { return ImageAvaibleSemaphores[CURRENT_FRAME]; }

	inline auto& GetRenderFence() { return RenderFences[CURRENT_FRAME]; }
	inline auto& GetPresentFence() { return ComputeFences[CURRENT_FRAME]; }

	inline auto& GetMainCommandBuffer() { return MainCommandBuffers[CURRENT_FRAME]; }
	inline auto& GetComputeCommandBuffer() { return ComputeCommandBuffers[CURRENT_FRAME]; }

	inline const wc::Queue GetGraphicsQueue() { return VulkanContext::GraphicsQueue; }
	inline const wc::Queue GetComputeQueue() { return VulkanContext::ComputeQueue; }
	inline const wc::Queue GetPresentQueue() { return /*VulkanContext::presentQueue*/VulkanContext::GraphicsQueue; }

	inline void UpdateFrame()
	{
		CURRENT_FRAME = (CURRENT_FRAME + 1) % FRAME_OVERLAP;
		if (m_TimelineValue >= UINT64_MAX - MAX_PASSES)
		{
			m_TimelineSemaphore.Destroy();
			m_TimelineSemaphore.Create("SyncContext::m_TimelineSemaphore");
			m_TimelineValue = 0;
		}
	}

	inline void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) // @TODO: revisit if this is suitable for a inline
	{
		UploadCommandBuffer.Begin();

		function(UploadCommandBuffer);

		UploadCommandBuffer.End();

		VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = UploadCommandBuffer.GetPointer();

		GetGraphicsQueue().Submit(submit, ImmediateFence);

		ImmediateFence.Wait();
		ImmediateFence.Reset();

		UploadCommandBuffer.Reset();
	}

	inline void Submit(wc::CommandBuffer cmd, const wc::Queue& queue, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, wc::Fence fence = VK_NULL_HANDLE)
	{
		const uint64_t waitValue = m_TimelineValue;
		const uint64_t signalValue = ++m_TimelineValue;

		VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
		timelineInfo.waitSemaphoreValueCount = 1;
		timelineInfo.pWaitSemaphoreValues = &waitValue;
		timelineInfo.signalSemaphoreValueCount = 1;
		timelineInfo.pSignalSemaphoreValues = &signalValue;

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext = &timelineInfo;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = m_TimelineSemaphore.GetPointer();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_TimelineSemaphore.GetPointer();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = cmd.GetPointer();
		submitInfo.pWaitDstStageMask = &waitStage;

		queue.Submit(submitInfo, fence);
	}

	inline void Destroy()
	{
		CommandPool.Destroy();
		ComputeCommandPool.Destroy();

		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			RenderSemaphores[i].Destroy();
			PresentSemaphores[i].Destroy();
			ImageAvaibleSemaphores[i].Destroy();

			RenderFences[i].Destroy();
			ComputeFences[i].Destroy();
		}

		m_TimelineSemaphore.Destroy();

		UploadCommandPool.Destroy();
		ImmediateFence.Destroy();
	}
}