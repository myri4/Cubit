#pragma once

#include "vk/Images.h"

namespace wc 
{
	// @brief Encapsulates a single frame buffer attachment
	struct FramebufferAttachment
	{
		wc::Image image;
		VkImageView view;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageSubresourceRange subresourceRange = {};
		VkAttachmentDescription description = {};

		// @brief Returns true if the attachment has a depth component
		bool hasDepth()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		// @brief Returns true if the attachment has a stencil component
		bool hasStencil()
		{
			std::vector<VkFormat> formats =
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		// @brief Returns true if the attachment is a depth and/or stencil attachment
		bool isDepthStencil() { return(hasDepth() || hasStencil()); }

		void Destroy() 
		{
			image.Destroy();

			vkDestroyImageView(VulkanContext::GetLogicalDevice(), view, nullptr);
			view = VK_NULL_HANDLE;
		}
	};

	// @brief Describes the attributes of an attachment to be created
	struct AttachmentCreateInfo
	{
		uint32_t width = 0, height = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags usage = 0;
	};
	
	// @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	struct Framebuffer 
	{
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
		std::vector<FramebufferAttachment> attachments;

		void Destroy() 
		{
			DestroyAttachments();
			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), renderPass, VulkanContext::GetAllocator());
			renderPass = VK_NULL_HANDLE;

			vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
			framebuffer = VK_NULL_HANDLE;
		}

		void DestroyAttachments() 
		{
			for (auto& attachment : attachments)
				attachment.Destroy();
			attachments.clear();
		}

		/**
		* Add a new attachment described by createinfo to the framebuffer's attachment list
		*
		* @param createinfo Structure that specifies the framebuffer to be constructed
		*
		* @return Index of the new attachment
		*/
		uint32_t AddAttachment(const AttachmentCreateInfo& createinfo)
		{
			FramebufferAttachment attachment;

			attachment.format = createinfo.format;

			VkImageAspectFlags aspectMask = 0;

			// Select aspect mask and layout depending on usage

			// Color attachment
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)			
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;			

			// Depth (and/or stencil) attachment
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (attachment.hasDepth())				
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				
				if (attachment.hasStencil())				
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;				
			}

			assert(aspectMask > 0);

			ImageCreateInfo imageInfo;
			imageInfo.format = createinfo.format;
			imageInfo.width = createinfo.width;
			imageInfo.height = createinfo.height;
			imageInfo.usage = createinfo.usage;

			// Create image for this attachment
			attachment.image.Create(imageInfo);

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;
			attachment.subresourceRange.levelCount = 1;
			attachment.subresourceRange.layerCount = 1;

			VkImageViewCreateInfo imageView = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageView.format = createinfo.format;
			imageView.subresourceRange = attachment.subresourceRange;
			//todo: workaround for depth+stencil attachments
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
			imageView.image = attachment.image;
			vkCreateImageView(VulkanContext::GetLogicalDevice(), &imageView, VulkanContext::GetAllocator(), &attachment.view);

			// Fill attachment description
			attachment.description = {};
			attachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.format = createinfo.format;
			attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			// Final layout
			// If not, final layout depends on attachment type
			if (attachment.hasDepth() || attachment.hasStencil())			
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;			
			else			
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		* Creates a default render pass setup with one sub pass
		*
		* @return VK_SUCCESS if all resources have been created successfully
		*/
		VkResult Create(glm::ivec2 size)
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments)			
				attachmentDescriptions.push_back(attachment.description);			

			// Collect attachment references
			std::vector<VkAttachmentReference> colorReferences;
			VkAttachmentReference depthReference = {};
			bool hasDepth = false;
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto& attachment : attachments)
			{
				if (attachment.isDepthStencil())
				{
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				}
				else
				{
					colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
					hasColor = true;
				}
				attachmentIndex++;
			}

			// Default render pass setup uses only one subpass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor)
			{
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth)			
				subpass.pDepthStencilAttachment = &depthReference;
			

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create render pass
			VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, VulkanContext::GetAllocator(), &renderPass);
			std::vector<VkImageView> attachmentViews;
			for (auto& attachment : attachments)			
				attachmentViews.push_back(attachment.view);
			

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto& attachment : attachments)			
				if (attachment.subresourceRange.layerCount > maxLayers)				
					maxLayers = attachment.subresourceRange.layerCount;
			

			VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = size.x;
			framebufferInfo.height = size.y;
			framebufferInfo.layers = maxLayers;
			vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &framebuffer);
			return VK_SUCCESS;
		}
	};
}