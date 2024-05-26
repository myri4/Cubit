#pragma once

#include "VulkanContext.h"

namespace wc 
{
	struct PipelineCacheData 
	{
		void* data = nullptr;
		size_t size = 0;
	};

	struct PipelineCache : public VkObject<VkPipelineCache> 
	{
		VkResult Create(const VkPipelineCacheCreateInfo& createInfo) 
		{ return vkCreatePipelineCache(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_RendererID); }

		void Destroy() 
		{
			vkDestroyPipelineCache(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}

		VkResult MergePipelineCaches(uint32_t count, const VkPipelineCache* caches) 
		{ return vkMergePipelineCaches(VulkanContext::GetLogicalDevice(), m_RendererID, count, caches); }

		PipelineCacheData GetData() const 
		{
			PipelineCacheData data;
			vkGetPipelineCacheData(VulkanContext::GetLogicalDevice(), m_RendererID, &data.size, VulkanContext::GetAllocator());
			void* dataPtr = malloc(data.size);
			vkGetPipelineCacheData(VulkanContext::GetLogicalDevice(), m_RendererID, &data.size, dataPtr);
			data.data = dataPtr;
			return data;
		}

		void SaveToFile(const std::string& filepath) const 
		{
			//PipelineCacheData data = GetData();
			//std::fstream file;
			//file.open(filepath, std::ios::out | std::ios::binary);
			//file.write((char*)data.data, data.size);
			//file.flush();
			//file.close();
			//free(data.data);
		}

		bool Valid(const void* data) 
		{
			VkPipelineCacheHeaderVersionOne header;
			memcpy(&header, data, sizeof(header));
			if (header.headerSize <= 0) return false;
			if (header.headerVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) return false;
			if (header.vendorID != VulkanContext::GetProperties().vendorID)   return false;
			if (header.deviceID != VulkanContext::GetProperties().deviceID)   return false;
			if (memcmp(header.pipelineCacheUUID, VulkanContext::GetProperties().pipelineCacheUUID, sizeof(header.pipelineCacheUUID)) != 0) return false;
			return true;
		}
	};
}