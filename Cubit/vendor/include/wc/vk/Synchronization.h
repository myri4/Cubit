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

		void Create(const std::string& name, VkSemaphoreCreateFlags flags = 0)
		{
			Create(flags);
			SetName(name);
		}

		void Destroy() 
		{ 
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}
	};

	struct TimelineSemaphore : public VkObject<VkSemaphore>
	{
		void Create(uint64_t initialValue = 0, VkSemaphoreCreateFlags flags = 0)
		{
			VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
			timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
			timelineCreateInfo.initialValue = initialValue;

			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			semaphoreCreateInfo.pNext = &timelineCreateInfo;
			semaphoreCreateInfo.flags = flags;

			vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCreateInfo, VulkanContext::GetAllocator(), &m_RendererID);
		}

		void Create(const std::string& name, uint64_t initialValue = 0)
		{
			Create(initialValue);
			SetName(name);
		}

		void Destroy()
		{
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}

		void Signal(uint64_t value)
		{
			VkSemaphoreSignalInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
			signalInfo.semaphore = m_RendererID;
			signalInfo.value = value;

			vkSignalSemaphore(VulkanContext::GetLogicalDevice(), &signalInfo);
		}

		void Wait(uint64_t waitValue, uint64_t timeout = UINT64_MAX)
		{
			VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
			waitInfo.flags = 0;
			waitInfo.semaphoreCount = 1;
			waitInfo.pSemaphores = &m_RendererID;
			waitInfo.pValues = &waitValue;

			vkWaitSemaphores(VulkanContext::GetLogicalDevice(), &waitInfo, timeout);
		}

		auto GetCounterValue()
		{
			uint64_t value;
			vkGetSemaphoreCounterValue(VulkanContext::GetLogicalDevice(), m_RendererID, &value);
			return value;
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