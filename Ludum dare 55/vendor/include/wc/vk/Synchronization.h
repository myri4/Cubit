#pragma once
#include "VulkanContext.h"

namespace wc 
{
	struct Fence : public VkObject<VkFence> 
	{
		Fence() = default;
		Fence(VkFence fence) { m_RendererID = fence; }

		void Create(const VkFenceCreateInfo& fenceCreateInfo) { vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceCreateInfo, VulkanContext::GetAllocator(), &m_RendererID); }

		void Create(VkFenceCreateFlags flags = 0) 
		{
			VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

			fenceCreateInfo.flags = flags;

			Create(fenceCreateInfo);
		}

		VkResult Wait(uint64_t timeout = UINT64_MAX) { return vkWaitForFences(VulkanContext::GetLogicalDevice(), 1, &m_RendererID, true, timeout); }

		void Reset() { vkResetFences(VulkanContext::GetLogicalDevice(), 1, &m_RendererID); }

		VkResult GetStatus() { return vkGetFenceStatus(VulkanContext::GetLogicalDevice(), m_RendererID); }

		void Destroy() 
		{ 
			vkDestroyFence(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}
	};

	struct Semaphore : public VkObject<VkSemaphore>
	{
		void Create(const VkSemaphoreCreateInfo& semaphoreCreateInfo) {	vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCreateInfo, VulkanContext::GetAllocator(), &m_RendererID); }

		void Create(VkSemaphoreCreateFlags flags = 0) 
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			semaphoreCreateInfo.flags = flags;

			Create(semaphoreCreateInfo);
		}

		void Destroy() 
		{ 
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}
	};

	struct Event : public VkObject<VkEvent> 
	{
		void Create(const VkEventCreateInfo& eventCreateInfo) { vkCreateEvent(VulkanContext::GetLogicalDevice(), &eventCreateInfo, VulkanContext::GetAllocator(), &m_RendererID); }

		void Create(VkEventCreateFlags flags = 0) 
		{
			VkEventCreateInfo eventCreateInfo = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
			eventCreateInfo.flags = flags;

			Create(eventCreateInfo);
		}

		VkResult GetStatus() { return vkGetEventStatus(VulkanContext::GetLogicalDevice(), m_RendererID); }

		void Set() { vkSetEvent(VulkanContext::GetLogicalDevice(), m_RendererID); }

		void Reset() { vkResetEvent(VulkanContext::GetLogicalDevice(), m_RendererID); }

		void Destroy() 
		{ 
			vkDestroyEvent(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}
	};
}