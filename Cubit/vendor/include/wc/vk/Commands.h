#pragma once

#include "VulkanContext.h"
#include "Synchronization.h"
#include "Pipeline.h"
#include "Descriptors.h"

namespace wc 
{
	struct CommandBuffer : public VkObject<VkCommandBuffer> 
	{		
		CommandBuffer() = default;
		CommandBuffer(VkCommandBuffer handle) { m_RendererID = handle; }

		VkResult Begin(const VkCommandBufferBeginInfo& info) const { return vkBeginCommandBuffer(m_RendererID, &info); }

		VkResult Begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) const 
		{
			VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

			info.pInheritanceInfo = nullptr;
			info.flags = flags;
			
			return vkBeginCommandBuffer(m_RendererID, &info);
		}

		void BindGraphicsPipeline(VkPipeline pipeline) const { vkCmdBindPipeline(m_RendererID, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline); }

		void BindComputePipeline(VkPipeline pipeline) const { vkCmdBindPipeline(m_RendererID, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline); }

		void BindIndexBuffer(VkBuffer index_buffer, const VkIndexType& indexType = VK_INDEX_TYPE_UINT32) const 
		{ vkCmdBindIndexBuffer(m_RendererID, index_buffer, 0, indexType); }

		void BindDescriptorSet(VkPipelineBindPoint bindPoint, uint32_t binding, VkPipelineLayout layout, VkDescriptorSet set) const 
		{ vkCmdBindDescriptorSets(m_RendererID, bindPoint, layout, binding, 1, &set, 0, nullptr); }

		void BindDescriptorBuffers(uint32_t bufferCount, VkDescriptorBufferBindingInfoEXT* bindingInfos) 
		{ vkCmdBindDescriptorBuffersEXT(m_RendererID, bufferCount, bindingInfos); }

		void SetDescriptorBufferOffsets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const uint32_t* bufferIndices, const VkDeviceSize* offsets) 
		{ vkCmdSetDescriptorBufferOffsetsEXT(m_RendererID, bindPoint, layout, firstSet, setCount, bufferIndices, offsets); }

		void PushConstants(VkPipelineLayout pipeline_layout, const VkShaderStageFlags& shader_stage_flags, uint32_t size, const void* data, uint32_t offset = 0) const 
		{ vkCmdPushConstants(m_RendererID, pipeline_layout, shader_stage_flags, offset, size, data); }

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) const 
		{ vkCmdDraw(m_RendererID, vertexCount, instanceCount, firstVertex, firstInstance); }

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t vertexOffset = 0, uint32_t firstIndex = 0, uint32_t firstInstance = 0) const 
		{ vkCmdDrawIndexed(m_RendererID, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance); }

		void DrawIndirect(VkBuffer buffer, uint32_t drawCount = 1, uint32_t offset = 0) const 
		{ vkCmdDrawIndirect(m_RendererID, buffer, offset, drawCount, sizeof(VkDrawIndirectCommand)); }

		void DrawIndexedIndirect(VkBuffer buffer, uint32_t drawCount = 1, uint32_t offset = 0) const 
		{ vkCmdDrawIndexedIndirect(m_RendererID, buffer, offset, drawCount, sizeof(VkDrawIndexedIndirectCommand)); }

		void DrawIndexedIndirect(const VkDrawIndexedIndirectCommand& cmd) const 
		{ vkCmdDrawIndexed(m_RendererID, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance); }

		void Dispatch(const glm::ivec3& groupCount) const 
		{ vkCmdDispatch(m_RendererID, groupCount.x, groupCount.y, groupCount.z); }

		void Dispatch(glm::ivec2 groupCount) const 
		{ vkCmdDispatch(m_RendererID, groupCount.x, groupCount.y, 1); }

		void Dispatch(glm::vec2 groupCount) const {	Dispatch(glm::ivec2(groupCount)); }

		void DispatchIndirect(VkBuffer buffer, VkDeviceSize offset = 0) const {	vkCmdDispatchIndirect(m_RendererID, buffer, offset); }

		void Execute(std::function<void()>&& function, Queue queue, Fence fence) 
		{
			Begin();

			function();

			End();

			VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &m_RendererID;

			queue.Submit(submit, fence);

			fence.Wait();
			fence.Reset();

			Reset();
		}

		void Execute(std::function<void()>&& function, Queue queue) 
		{
			Begin();

			function();

			End();

			VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &m_RendererID;

			queue.Submit(submit);

			Reset();
		}

		void BeginRenderPass(const VkRenderPassBeginInfo& info) { vkCmdBeginRenderPass(m_RendererID, &info, VK_SUBPASS_CONTENTS_INLINE); }

		void EndRenderPass() { vkCmdEndRenderPass(m_RendererID); }

		VkResult End() const { return vkEndCommandBuffer(m_RendererID); }

		VkResult Reset(VkCommandBufferResetFlags flags = 0) const { return vkResetCommandBuffer(m_RendererID, flags); }
	};

	struct CommandPool : public VkObject<VkCommandPool> 
	{
		VkResult Create(const VkCommandPoolCreateInfo& createInfo) 
		{ return vkCreateCommandPool(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_RendererID); }

		VkResult Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) 
		{
			VkCommandPoolCreateInfo commandPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

			commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
			commandPoolInfo.flags = createFlags;

			return Create(commandPoolInfo);
		}

		VkResult Allocate(VkCommandBufferLevel level, VkCommandBuffer& commandBuffer) const 
		{
			VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

			//commands will be made from our _commandPool
			cmdAllocInfo.commandPool = m_RendererID;
			//we will allocate 1 command buffer
			cmdAllocInfo.commandBufferCount = 1;
			// command level is Primary
			cmdAllocInfo.level = level;


			return vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdAllocInfo, &commandBuffer);
		}

		VkResult Reset(VkCommandPoolResetFlags flags = 0) { return vkResetCommandPool(VulkanContext::GetLogicalDevice(), m_RendererID, flags); }

		void Trim(VkCommandPoolTrimFlags flags = 0) { vkTrimCommandPool(VulkanContext::GetLogicalDevice(), m_RendererID, flags); }

		void Free(CommandBuffer cmd) { vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), m_RendererID, 1, cmd.GetPointer()); }

		void Destroy() 
		{ 
			vkDestroyCommandPool(VulkanContext::GetLogicalDevice(), m_RendererID, VulkanContext::GetAllocator());
			m_RendererID = VK_NULL_HANDLE;
		}
	};	
}