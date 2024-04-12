#pragma once

#include "VulkanContext.h"
#include "Commands.h"

namespace SyncContext
{
	inline wc::Semaphore PresentSemaphore, RenderSemaphore;

	inline wc::Fence RenderFence;
	inline wc::Fence ComputeFence;
	inline wc::Fence UploadFence;

	inline wc::CommandBuffer MainCommandBuffer;
	inline wc::CommandBuffer ComputeCommandBuffer;
	inline wc::CommandBuffer UploadCommandBuffer;

	inline wc::CommandPool CommandPool;
	inline wc::CommandPool ComputeCommandPool;
	inline wc::CommandPool UploadCommandPool;

	inline void Create()
	{
		CommandPool.Create(VulkanContext::graphicsQueue.GetFamily());
		ComputeCommandPool.Create(VulkanContext::computeQueue.GetFamily());
		UploadCommandPool.Create(VulkanContext::graphicsQueue.GetFamily());

		CommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, MainCommandBuffer);
		ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, ComputeCommandBuffer);
		UploadCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, UploadCommandBuffer);

		MainCommandBuffer.SetName("MainCommandBuffer");
		ComputeCommandBuffer.SetName("ComputeCommandBuffer");

		RenderFence.Create();
		ComputeFence.Create();
		UploadFence.Create();

		PresentSemaphore.Create();
		RenderSemaphore.Create();
	}

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

	inline const wc::Queue GetGraphicsQueue() { return VulkanContext::graphicsQueue; }
	inline const wc::Queue GetComputeQueue() { return VulkanContext::computeQueue; }
	inline const wc::Queue GetPresentQueue() { return /*VulkanContext::presentQueue*/VulkanContext::graphicsQueue; }

	inline void Destroy()
	{
		CommandPool.Destroy();
		ComputeCommandPool.Destroy();

		RenderFence.Destroy();
		RenderSemaphore.Destroy();
		PresentSemaphore.Destroy();

		ComputeFence.Destroy();

		UploadCommandPool.Destroy();
		UploadFence.Destroy();
	}
}