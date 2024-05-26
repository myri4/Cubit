#pragma once

#include <filesystem>
#include <imgui/imgui_impl_vulkan.h>
#include <stb_image/stb_image.h>
#include <wc/vk/Images.h>
#include <wc/vk/SyncContext.h>

namespace wc 
{
    struct TextureCreateInfo
    {
        // Image
		VkFormat                 format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32_t                 width = 1;
		uint32_t                 height = 1;
        bool                     mipMapping = false;
		VkImageUsageFlags        usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        // Sampler
		Filter                magFilter = Filter::NEAREST;
		Filter                minFilter = Filter::NEAREST;
		SamplerMipmapMode     mipmapMode = SamplerMipmapMode::NEAREST;
		SamplerAddressMode    addressModeU = SamplerAddressMode::REPEAT;
		SamplerAddressMode    addressModeV = SamplerAddressMode::REPEAT;
		SamplerAddressMode    addressModeW = SamplerAddressMode::REPEAT;
    };

    class Texture 
    {
        Image image;
        ImageView view;
        Sampler sampler;
        VkDescriptorSet imageID = VK_NULL_HANDLE;
    public:

        Texture() = default;

		void Allocate(const TextureCreateInfo& createInfo)
		{
			ImageCreateInfo imageCreateInfo;

			imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

			imageCreateInfo.width = createInfo.width;
			imageCreateInfo.height = createInfo.height;

			imageCreateInfo.mipLevels = createInfo.mipMapping ? GetMipLevelCount(glm::vec2(createInfo.width, createInfo.height)) : 1;
			imageCreateInfo.usage = createInfo.usage;

			image.Create(imageCreateInfo);

			view.Create(imageCreateInfo.format, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, image.mipLevels);

			SamplerCreateInfo samplerInfo;
			samplerInfo.addressModeU = createInfo.addressModeU;
            samplerInfo.addressModeV = createInfo.addressModeV;
            samplerInfo.addressModeW = createInfo.addressModeW;
			samplerInfo.maxLod = (float)image.mipLevels;

			if (VulkanContext::GetSupportedFeatures().samplerAnisotropy && createInfo.mipMapping)
			{
				samplerInfo.anisotropyEnable = true;
				samplerInfo.maxAnisotropy = VulkanContext::GetProperties().limits.maxSamplerAnisotropy;
			}
            			
			samplerInfo.magFilter = createInfo.magFilter;
			samplerInfo.minFilter = createInfo.minFilter;
			samplerInfo.mipmapMode = createInfo.mipmapMode;			

			sampler.Create(samplerInfo);
			imageID = MakeImGuiDescriptor(imageID, { .sampler = sampler, .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		}

        void Allocate(uint32_t width, uint32_t height, bool mipMaping = false)
        {
            ImageCreateInfo imageCreateInfo;

            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

            imageCreateInfo.width = width;
            imageCreateInfo.height = height;

            imageCreateInfo.mipLevels = mipMaping ? GetMipLevelCount(glm::vec2(width, height)) : 1;
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            image.Create(imageCreateInfo);

            view.Create(imageCreateInfo.format, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, image.mipLevels);
            
            SamplerCreateInfo samplerInfo;
            samplerInfo.addressModeU = SamplerAddressMode::REPEAT;
            samplerInfo.addressModeV = SamplerAddressMode::REPEAT;
            samplerInfo.addressModeW = SamplerAddressMode::REPEAT;
            samplerInfo.maxLod = (float)image.mipLevels;

            if (VulkanContext::GetSupportedFeatures().samplerAnisotropy && mipMaping) 
            {
                samplerInfo.anisotropyEnable = true;
                samplerInfo.maxAnisotropy = VulkanContext::GetProperties().limits.maxSamplerAnisotropy;
            }

            if (width <= 128 || height <= 128) 
            {
                samplerInfo.magFilter = Filter::NEAREST;
                samplerInfo.minFilter = Filter::NEAREST;
                samplerInfo.mipmapMode = SamplerMipmapMode::NEAREST;
            }
            else 
            {
                samplerInfo.magFilter = Filter::LINEAR;
                samplerInfo.minFilter = Filter::LINEAR;
                samplerInfo.mipmapMode = SamplerMipmapMode::LINEAR;
            }

            sampler.Create(samplerInfo);         
            imageID = MakeImGuiDescriptor(imageID, { .sampler = sampler, .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
        }

        void Load(const std::string& filepath, bool mipMaping = false) 
        {
            int32_t width = 0, height = 0, fnrComponents;
            auto data = stbi_load(filepath.c_str(), &width, &height, &fnrComponents, 4);
            
            if (data) 
                Load(data, width, height, mipMaping);            
            else
                WC_CORE_ERROR("Could not find file at location {0}", filepath.c_str());

            stbi_image_free(data);
        }

        void Load(const void* data, uint32_t width, uint32_t height, bool mipMaping = false) 
        {
            Allocate(width, height, mipMaping);
            SetData(data, width, height, 0, 0, mipMaping);
        }

        void SetData(const void* data, uint32_t width, uint32_t height, uint32_t offsetX = 0, uint32_t offsetY = 0, bool mipMaping = false)
        {
            VkDeviceSize imageSize = width * height * 4;

            StagingBuffer stagingBuffer;
            stagingBuffer.Allocate(imageSize);
            stagingBuffer.SetData(data, imageSize);

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            SyncContext::immediate_submit([&](VkCommandBuffer cmd) {
                image.setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkBufferImageCopy copyRegion = {};
                copyRegion.bufferOffset = 0;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;

                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = 0;
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount = 1;
                copyRegion.imageExtent = {
                .width = width,
                .height = height,
                .depth = 1 };
                copyRegion.imageOffset =
                {
                    .x = (int32_t)offsetX,
                    .y = (int32_t)offsetY,
                    .z = 0,
                };

                vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                if (mipMaping)
                {
                    image.insertMemoryBarrier(
                        cmd,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        subresourceRange);

                    // Copy down mips from n-1 to n
                    for (uint32_t i = 1; i < image.mipLevels; i++)
                    {
                        VkImageBlit imageBlit{};

                        // Source
                        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageBlit.srcSubresource.layerCount = 1;
                        imageBlit.srcSubresource.mipLevel = i - 1;
                        imageBlit.srcSubresource.baseArrayLayer = 0;
                        imageBlit.srcOffsets[1].x = int32_t(image.width >> (i - 1));
                        imageBlit.srcOffsets[1].y = int32_t(image.height >> (i - 1));
                        imageBlit.srcOffsets[1].z = 1;

                        // Destination
                        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageBlit.dstSubresource.layerCount = 1;
                        imageBlit.dstSubresource.mipLevel = i;
                        imageBlit.dstSubresource.baseArrayLayer = 0;
                        imageBlit.dstOffsets[1].x = int32_t(image.width >> i);
                        imageBlit.dstOffsets[1].y = int32_t(image.height >> i);
                        imageBlit.dstOffsets[1].z = 1;

                        VkImageSubresourceRange mipSubRange = {};
                        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        mipSubRange.baseMipLevel = i;
                        mipSubRange.baseArrayLayer = 0;
                        mipSubRange.levelCount = 1;
                        mipSubRange.layerCount = 1;

                        // Prepare current mip level as image blit destination
                        image.insertMemoryBarrier(
                            cmd, 0,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            mipSubRange);

                        // Blit from previous level
                        vkCmdBlitImage(
                            cmd,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1,
                            &imageBlit,
                            VK_FILTER_LINEAR);

                        // Prepare current mip level as image blit source for next level
                        image.insertMemoryBarrier(
                            cmd,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_TRANSFER_READ_BIT,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            mipSubRange);
                    }

                    // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
                    subresourceRange.levelCount = image.mipLevels;
                    image.insertMemoryBarrier(
                        cmd,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        subresourceRange);
                }
                else
                    image.setLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                });

            stagingBuffer.Free();
        }

        void MakeRenderable()
        {
            SyncContext::immediate_submit([&](VkCommandBuffer cmd) {
                    image.setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                });
        }

        void Destroy() 
        {
            image.Destroy();
            view.Destroy();
            sampler.Destroy();
        }

        glm::ivec2 GetSize() { return { image.width, image.height }; }

        ImageView GetView() const { return view; }
        Sampler GetSampler() const { return sampler; }
        Image GetImage() const { return image; }

        void SetName(const std::string& name) 
        {
            view.SetName(name + "_view");
            sampler.SetName(name + "_sampler");
            image.SetName(name + "_image");
        }

        operator ImTextureID () const { return (ImTextureID)imageID; }
    };
}