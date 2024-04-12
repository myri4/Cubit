#pragma once

#include "VulkanContext.h"
#include <array>
#include <deque>

namespace wc 
{
	using DescriptorSet = VkDescriptorSet;

	struct DescriptorSetLayout : public VkObject<VkDescriptorSetLayout> 
	{
		DescriptorSetLayout() = default;
		DescriptorSetLayout(const VkDescriptorSetLayout& layout) { m_RendererID = layout; }

		void Create(const VkDescriptorSetLayoutCreateInfo& setinfo) 
		{ vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &setinfo, VulkanContext::GetAllocator(), &m_RendererID); }

		void Destroy() 
		{ 
			vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}

		VkDeviceSize GetDescriptorSize() 
		{
			VkDeviceSize size = 0;
			vkGetDescriptorSetLayoutSizeEXT(VulkanContext::GetLogicalDevice(), m_RendererID, &size);
			return size;
		}

		VkDeviceSize GetDescriptorBindingOffset(uint32_t binding) 
		{
			VkDeviceSize offset = 0;
			vkGetDescriptorSetLayoutBindingOffsetEXT(VulkanContext::GetLogicalDevice(), m_RendererID, binding, &offset);
			return offset;
		}
	};

	inline void GetDescriptor(const VkDescriptorGetInfoEXT* pCreateInfo, size_t dataSize, void* pDescriptor) { vkGetDescriptorEXT(VulkanContext::GetLogicalDevice(), pCreateInfo, dataSize, pDescriptor); }

	struct DescriptorAllocator 
	{
		void Create() 
		{
			currentPool = GrabPool();
			usedPools.push_back(currentPool);
		}

		void ResetPools() 
		{
			for (auto& p : usedPools)
				vkResetDescriptorPool(VulkanContext::GetLogicalDevice(), p, 0);

			freePools = usedPools;
			usedPools.clear();
			currentPool = VK_NULL_HANDLE;
		}

		bool allocate(DescriptorSet& set, const DescriptorSetLayout& layout) { return allocate(set, layout, nullptr, 1); }

		bool allocate(DescriptorSet& set, const VkDescriptorSetLayout& layout, const void* pNext, uint32_t descriptorCount) 
		{
			if (currentPool == VK_NULL_HANDLE)
			{
				currentPool = GrabPool();
				usedPools.push_back(currentPool);
			}

			VkResult allocResult;
			{
				VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = descriptorCount;
				allocInfo.pSetLayouts = &layout;
				allocInfo.pNext = pNext;


				allocResult = vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, &set);
			}
			switch (allocResult) {
			case VK_SUCCESS: return true;
			case VK_ERROR_FRAGMENTED_POOL:
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				//allocate a new pool and retry
				currentPool = GrabPool();
				usedPools.push_back(currentPool);
				
				VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = descriptorCount;
				allocInfo.pSetLayouts = &layout;
				allocInfo.pNext = pNext;

				allocResult = vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, &set);
				
				//if it still fails then we have big issues
				if (allocResult == VK_SUCCESS) return true;
			}

			return false;
		}

		void Destroy() 
		{
			//delete every pool held
			for (auto& p : freePools)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), p, VulkanContext::GetAllocator());
			
			for (auto& p : usedPools)
				vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), p, VulkanContext::GetAllocator());
		}

		VkDescriptorPool GetCurrentPool() { return currentPool; }

	private:
		VkDescriptorPool GrabPool() 
		{
			if (freePools.size() > 0)
			{
				VkDescriptorPool pool = freePools.back();
				freePools.pop_back();
				return pool;
			}
			else 
			{
				uint32_t count = 1000;

				std::pair<VkDescriptorType, float> dSizes[] = 
				{				
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
					//{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
					//{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
					//{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
					//{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
					//{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f },
				};


				std::array<VkDescriptorPoolSize, std::size(dSizes)> sizes;
				for (int i = 0; i < std::size(dSizes); i++)
					sizes[i] = { dSizes[i].first, uint32_t(dSizes[i].second * count)};

				VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
				pool_info.flags = 0;
				pool_info.maxSets = count;
				pool_info.poolSizeCount = (uint32_t)sizes.size();
				pool_info.pPoolSizes = sizes.data();

				VkDescriptorPool descriptorPool;
				vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &pool_info, VulkanContext::GetAllocator(), &descriptorPool);

				return descriptorPool;
			}
		}

		VkDescriptorPool currentPool;
		std::vector<VkDescriptorPool> usedPools;
		std::vector<VkDescriptorPool> freePools;
	}inline descriptorAllocator;

	struct DescriptorWriter 
	{
	private:
		std::deque<VkDescriptorImageInfo> imageInfos;
		std::deque<VkDescriptorBufferInfo> bufferInfos;
	public:
		std::vector<VkWriteDescriptorSet> writes;
		wc::DescriptorSet dstSet = VK_NULL_HANDLE;		

		DescriptorWriter& write_buffer(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType type) 
		{
			auto& info = bufferInfos.emplace_back(bufferInfo);
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pBufferInfo = &info;

			writes.push_back(write);
			return *this;
		}

		DescriptorWriter& write_buffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
		{
			return write_buffer(binding, VkDescriptorBufferInfo{
				.buffer = buffer,
				.offset = offset,
				.range = size
				}, type);
		}

		DescriptorWriter& write_image(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType type) 
		{
			auto& info = imageInfos.emplace_back(imageInfo);
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pImageInfo = &info;

			writes.push_back(write);
			return *this;
		}

		DescriptorWriter& write_image(uint32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
		{
			return write_image(binding, VkDescriptorImageInfo{
				.sampler = sampler,
				.imageView = image,
				.imageLayout = layout
				}, type);
		}

		DescriptorWriter& write_images(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfo, VkDescriptorType type) 
		{
			VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

			write.dstSet = dstSet;
			write.dstBinding = binding;
			write.descriptorCount = (uint32_t)imageInfo.size();
			write.descriptorType = type;
			write.pImageInfo = imageInfo.data();

			writes.push_back(write);
			return *this;
		}

		void Clear()
		{
			imageInfos.clear();
			writes.clear();
			bufferInfos.clear();
		}

		void Update() {	vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr); }
	};
}