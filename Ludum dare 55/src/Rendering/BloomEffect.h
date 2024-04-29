#pragma once

#include <wc/Framebuffer.h>
#include <wc/Math/Camera.h>
#include <wc/Shader.h>
#include <wc/vk/Descriptors.h>

#include <wc/vk/SyncContext.h>

// @TODO: Make an usable api in the future that just applies effects to images.
// The api should be structured like a rendergraph and should have automatic synchronazation just like opengl.

namespace wc
{
	uint32_t m_ComputeWorkGroupSize = 4; // @TODO: REMOVE!!!?
	float BloomThreshold = 1.2f;
	float BloomKnee = 0.6f;

	enum class BloomMode
	{
		Prefilter,
		Downsample,
		UpsampleFirst,
		Upsample
	};

	Sampler m_BloomSampler;
	ComputeShader m_BloomShader;
	Sampler m_ScreenSampler;

	std::vector<VkDescriptorSet> m_BloomSets;

	uint32_t m_BloomMipLevels = 1;

	struct BloomImage
	{
		std::vector<ImageView> imageViews;
		Image image;

		void Create(uint32_t width, uint32_t height, uint32_t mipLevels)
		{
			ImageCreateInfo imgInfo;

			imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

			imgInfo.width = width;
			imgInfo.height = height;

			imgInfo.mipLevels = mipLevels;
			imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

			image.Create(imgInfo);			

			{
				VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				createInfo.flags = 0;
				createInfo.image = image;
				createInfo.subresourceRange.layerCount = 1;
				createInfo.subresourceRange.levelCount = imgInfo.mipLevels;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				{ // Creating the first image view
					ImageView& imageView = imageViews.emplace_back();
					imageView.Create(createInfo);
				}

				// Create The rest
				createInfo.subresourceRange.levelCount = 1;
				for (uint32_t i = 1; i < imgInfo.mipLevels; i++)
				{
					createInfo.subresourceRange.baseMipLevel = i;
					ImageView& imageView = imageViews.emplace_back();
					imageView.Create(createInfo);
				}
			}
		}

		void Destroy()
		{
			for (auto& view : imageViews) view.Destroy();
			image.Destroy();
			imageViews.clear();
		}
	} m_BloomBuffers[3];

	struct BloomBufferSettings
	{
		glm::vec4 Params = glm::vec4(1.f); // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
		float LOD = 0.f;
		int Mode = (int)BloomMode::Prefilter;
	};

	void GenerateBloomDescriptor(const ImageView& outputView, const ImageView& bloomView)
	{
		DescriptorSet& descriptor = m_BloomSets.emplace_back();
		descriptorAllocator.allocate(descriptor, m_BloomShader.GetDescriptorLayout());
		DescriptorWriter writer;
		writer.dstSet = descriptor;
		writer.write_image(0, GetDescriptorData(m_ScreenSampler, outputView, VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.write_image(1, GetDescriptorData(m_ScreenSampler, bloomView, VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(2, GetDescriptorData(m_ScreenSampler, m_BloomBuffers[2].imageViews[0], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.Update();
	}

	// @TODO/NOTE? : mipLevelCount can be ommited and maybe should be
	void CreateBloomImages(glm::vec2 renderSize, uint32_t mipLevelCount)
	{
		glm::ivec2 bloomTexSize = renderSize * 0.5f;
		bloomTexSize += glm::ivec2(m_ComputeWorkGroupSize - bloomTexSize.x % m_ComputeWorkGroupSize, m_ComputeWorkGroupSize - bloomTexSize.y % m_ComputeWorkGroupSize);
		m_BloomMipLevels = mipLevelCount - 4;

		for (int i = 0; i < 3; i++)
		{
			m_BloomBuffers[i].Create(bloomTexSize.x, bloomTexSize.y, m_BloomMipLevels);
			m_BloomBuffers[i].image.SetName(std::format("m_BloomBuffers[{}]", i));
		}

		SyncContext::immediate_submit([&](VkCommandBuffer cmd)
			{
				VkImageSubresourceRange range;
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseArrayLayer = 0;
				range.baseMipLevel = 0;
				range.layerCount = 1;
				range.levelCount = m_BloomMipLevels;
				for (int i = 0; i < 3; i++)
				{
					m_BloomBuffers[i].image.setLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
				}
			});
	}

	void CreateBloom(ImageView imageView)
	{
		ComputeShaderCreateInfo createInfo;
		createInfo.path = "assets/shaders/bloom.comp";
		m_BloomShader.Create(createInfo);


		GenerateBloomDescriptor(m_BloomBuffers[0].imageViews[0], imageView);

		for (uint32_t currentMip = 1; currentMip < m_BloomMipLevels; currentMip++) {
			// Ping 
			GenerateBloomDescriptor(m_BloomBuffers[1].imageViews[currentMip], m_BloomBuffers[0].imageViews[0]);

			// Pong 
			GenerateBloomDescriptor(m_BloomBuffers[0].imageViews[currentMip], m_BloomBuffers[1].imageViews[0]);
		}

		// First Upsample
		GenerateBloomDescriptor(m_BloomBuffers[2].imageViews[m_BloomMipLevels - 1], m_BloomBuffers[0].imageViews[0]);

		for (int currentMip = m_BloomMipLevels - 2; currentMip >= 0; currentMip--)
			GenerateBloomDescriptor(m_BloomBuffers[2].imageViews[currentMip], m_BloomBuffers[0].imageViews[0]);
	}

	void DestroyBloomImages()
	{
		for (int i = 0; i < 3; i++) m_BloomBuffers[i].Destroy();
		m_BloomSets.clear(); // @TODO/NOTE: This is a memory leak. We should be reusing the already generated descriptor sets
	}

	void DeinitBloom()
	{
		m_BloomShader.Destroy();
		m_ScreenSampler.Destroy();
	}
}