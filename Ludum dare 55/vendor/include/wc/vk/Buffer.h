#pragma once

#include "VulkanContext.h"
#include "Commands.h"

namespace wc
{
	enum BufferUsage
	{
		TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		UNIFORM_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		STORAGE_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		INDEX_BUFFER = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VERTEX_BUFFER = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		INDIRECT_BUFFER = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		DEVICE_ADDRESS = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};

	class StagingBuffer : public VkObject<VkBuffer>
	{
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	public:

		void Allocate(VkDeviceSize bufferSize, VkFlags usage = TRANSFER_SRC)
		{
			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = bufferSize;
			bufferInfo.usage = usage;

			VmaAllocationCreateInfo vmaallocInfo = {};
			vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

			vmaCreateBuffer(VulkanContext::GetMemoryAllocator(), &bufferInfo, &vmaallocInfo, &m_RendererID, &m_Allocation, nullptr);
		}

		void* Map()
		{
			void* data;
			vmaMapMemory(VulkanContext::GetMemoryAllocator(), m_Allocation, &data);
			return data;
		}

		void Unmap() { vmaUnmapMemory(VulkanContext::GetMemoryAllocator(), m_Allocation); }

		void SetData(const void* data, VkDeviceSize size = VK_WHOLE_SIZE)
		{
			memcpy(Map(), data, size);
			Unmap();
		}

		void Free()
		{
			vmaDestroyBuffer(VulkanContext::GetMemoryAllocator(), m_RendererID, m_Allocation);
			m_RendererID = VK_NULL_HANDLE;
		}
	};

	class Buffer : public VkObject<VkBuffer>
	{
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	public:

		void Allocate(VkDeviceSize bufferSize, uint32_t usage = wc::STORAGE_BUFFER | wc::DEVICE_ADDRESS)
		{
			VkBufferCreateInfo bufferInfo = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = bufferSize,
				.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			};

			VmaAllocationCreateInfo vmaallocInfo = {};
			vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			vmaCreateBuffer(VulkanContext::GetMemoryAllocator(), &bufferInfo, &vmaallocInfo, &m_RendererID, &m_Allocation, nullptr);			
		}

		VkDeviceAddress GetDeviceAddress() const
		{
			VkBufferDeviceAddressInfo pInfo = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = m_RendererID
			};
			return vkGetBufferDeviceAddress(VulkanContext::GetLogicalDevice(), &pInfo);
		}

		void SetData(VkCommandBuffer cmd, const VkBufferCopy& copy, const StagingBuffer& stagingBuffer) { vkCmdCopyBuffer(cmd, stagingBuffer, m_RendererID, 1, &copy); }

		void SetData(VkCommandBuffer cmd, const VkBufferCopy& copy, const void* data)
		{
			wc::StagingBuffer stb;
			stb.Allocate(copy.size);
			stb.SetData(data, copy.size);
			SetData(cmd, copy, stb);
			stb.Free();
		}

		void SetData(VkCommandBuffer cmd, const void* data, uint32_t size, uint32_t offset = 0, uint32_t srcOffset = 0)
		{
			VkBufferCopy copy;
			copy.size = size;
			copy.dstOffset = offset;
			copy.srcOffset = srcOffset;
			SetData(cmd, copy, data);
		}

		void SetData(VkCommandBuffer cmd, const StagingBuffer& buffer, uint32_t size, uint32_t offset = 0, uint32_t srcOffset = 0)
		{
			VkBufferCopy copy;
			copy.size = size;
			copy.dstOffset = offset;
			copy.srcOffset = srcOffset;
			SetData(cmd, copy, buffer);
		}

		VkDeviceSize Size() const
		{
			if (!m_Allocation) return 0; // @NOTE: Check if correct

			VmaAllocationInfo inf;
			vmaGetAllocationInfo(VulkanContext::GetMemoryAllocator(), m_Allocation, &inf);
			return inf.size;
		}

		void Free() 
		{ 
			vmaDestroyBuffer(VulkanContext::GetMemoryAllocator(), m_RendererID, m_Allocation); 
			m_RendererID = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}

		VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			VkDescriptorBufferInfo info;
			info.buffer = m_RendererID;
			info.offset = offset;
			info.range = size;
			return info;
		}
	};

	template<typename T>
	struct BufferManager
	{
	private:
		Buffer m_Buffer;
		StagingBuffer m_StagingBuffer;

		T* m_StagingPtr = nullptr;
	public:
		uint32_t Counter = 0;

	public:
		auto& GetBuffer() const { return m_Buffer; }
		auto& GetStagingBuffer() const { return m_StagingBuffer; }

		T* Map() 
		{
			m_StagingPtr = (T*)m_StagingBuffer.Map();
			return m_StagingPtr;
		}

		void Unmap() 
		{ 
			m_StagingPtr = nullptr; 
			m_StagingBuffer.Unmap(); 
		}

		void Allocate(uint32_t elements, uint32_t usage = wc::STORAGE_BUFFER)
		{
			m_Buffer.Allocate(elements * sizeof(T), usage);
			m_StagingBuffer.Allocate(elements * sizeof(T));
			Map();
		}

		void Add(const T& object) { m_StagingPtr[Counter++] = object; }

		void Remove(uint32_t id) { m_StagingPtr[id] = m_StagingPtr[--Counter]; }

		void Free() 
		{ 
			Unmap();
			Counter = 0;
			m_Buffer.Free();
			m_StagingBuffer.Free();
		}

		void Update(VkCommandBuffer cmd, uint32_t elems = 0)
		{
			Unmap();
			m_Buffer.SetData(cmd, { 0,0,sizeof(T) * (elems == 0 ? Counter : elems)}, m_StagingBuffer);
			Map();
		}

		inline operator T* () { return m_StagingPtr; }
		inline operator T* () const { return m_StagingPtr; }
	};
}