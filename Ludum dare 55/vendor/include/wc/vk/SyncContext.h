#pragma once

#include "VulkanContext.h"
#include "Commands.h"

constexpr uint32_t FRAME_OVERLAP = 2;
inline uint8_t CURRENT_FRAME = 0;

namespace SyncContext
{
	inline wc::Semaphore RenderSemaphores[FRAME_OVERLAP], PresentSemaphores[FRAME_OVERLAP], ImageAvaibleSemaphores[FRAME_OVERLAP];

	inline wc::Fence RenderFences[FRAME_OVERLAP];
	inline wc::Fence ComputeFences[FRAME_OVERLAP];
	inline wc::Fence UploadFence;

	inline wc::CommandBuffer MainCommandBuffers[FRAME_OVERLAP];
	inline wc::CommandBuffer ComputeCommandBuffers[FRAME_OVERLAP];
	inline wc::CommandBuffer UploadCommandBuffer;

	inline wc::CommandPool CommandPool;
	inline wc::CommandPool ComputeCommandPool;
	inline wc::CommandPool UploadCommandPool;

	inline void Create()
	{
		CommandPool.Create(VulkanContext::graphicsQueue.GetFamily());
		ComputeCommandPool.Create(VulkanContext::computeQueue.GetFamily());
		UploadCommandPool.Create(VulkanContext::graphicsQueue.GetFamily());

		UploadCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, UploadCommandBuffer);
		UploadFence.Create();
		
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
	}

	inline auto& GetRenderSemaphore() { return RenderSemaphores[CURRENT_FRAME]; }
	inline auto& GetPresentSemaphore() { return PresentSemaphores[CURRENT_FRAME]; }
	inline auto& GetImageAvaibleSemaphore() { return ImageAvaibleSemaphores[CURRENT_FRAME]; }

	inline auto& GetRenderFence() { return RenderFences[CURRENT_FRAME]; }
	inline auto& GetPresentFence() { return ComputeFences[CURRENT_FRAME]; }

	inline auto& GetMainCommandBuffer() { return MainCommandBuffers[CURRENT_FRAME]; }
	inline auto& GetComputeCommandBuffer() { return ComputeCommandBuffers[CURRENT_FRAME]; }

	inline const wc::Queue GetGraphicsQueue() { return VulkanContext::graphicsQueue; }
	inline const wc::Queue GetComputeQueue() { return VulkanContext::computeQueue; }
	inline const wc::Queue GetPresentQueue() { return /*VulkanContext::presentQueue*/VulkanContext::graphicsQueue; }

	inline void UpdateFrame() { CURRENT_FRAME = (CURRENT_FRAME + 1) % FRAME_OVERLAP; }

	inline void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) // @TODO: revisit if this is suitable for a inline
	{
		UploadCommandBuffer.Begin();

		function(UploadCommandBuffer);

		UploadCommandBuffer.End();

		VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = UploadCommandBuffer.GetPointer();

		VulkanContext::graphicsQueue.Submit(submit, UploadFence);

		UploadFence.Wait();
		UploadFence.Reset();

		UploadCommandBuffer.Reset();
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


		UploadCommandPool.Destroy();
		UploadFence.Destroy();
	}
}