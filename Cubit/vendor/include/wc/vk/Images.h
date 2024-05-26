#pragma once

#include "Buffer.h"
#include <glm/glm.hpp>
#undef min

namespace wc 
{
    inline glm::ivec2 GetMipSize(int level, glm::ivec2 size)
    {
        while (level != 0)
        {
            size.x /= 2;
            size.y /= 2;
            level--;
        }

        return size;
    }

    inline int GetMipLevelCount(glm::vec2 size) { return (int)glm::floor(glm::log2(glm::min(size.x, size.y))); }

    enum class ImageFormat 
    {
        UNDEFINED = 0,
        R8_UNORM = 9,
        R8_SNORM = 10,
        R8_USCALED = 11,
        R8_SSCALED = 12,
        R8_UINT = 13,
        R8_SINT = 14,
        R8_SRGB = 15,
        R8G8_UNORM = 16,
        R8G8_SNORM = 17,
        R8G8_USCALED = 18,
        R8G8_SSCALED = 19,
        R8G8_UINT = 20,
        R8G8_SINT = 21,
        R8G8_SRGB = 22,
        R8G8B8_UNORM = 23,
        R8G8B8_SNORM = 24,
        R8G8B8_USCALED = 25,
        R8G8B8_SSCALED = 26,
        R8G8B8_UINT = 27,
        R8G8B8_SINT = 28,
        R8G8B8_SRGB = 29,
        B8G8R8_UNORM = 30,
        B8G8R8_SNORM = 31,
        B8G8R8_USCALED = 32,
        B8G8R8_SSCALED = 33,
        B8G8R8_UINT = 34,
        B8G8R8_SINT = 35,
        B8G8R8_SRGB = 36,
        R8G8B8A8_UNORM = 37,
        R8G8B8A8_SNORM = 38,
        R8G8B8A8_USCALED = 39,
        R8G8B8A8_SSCALED = 40,
        R8G8B8A8_UINT = 41,
        R8G8B8A8_SINT = 42,
        R8G8B8A8_SRGB = 43,
        B8G8R8A8_UNORM = 44,
        B8G8R8A8_SNORM = 45,
        B8G8R8A8_USCALED = 46,
        B8G8R8A8_SSCALED = 47,
        B8G8R8A8_UINT = 48,
        B8G8R8A8_SINT = 49,
        B8G8R8A8_SRGB = 50,
        R16_UNORM = 70,
        R16_SNORM = 71,
        R16_USCALED = 72,
        R16_SSCALED = 73,
        R16_UINT = 74,
        R16_SINT = 75,
        R16_SFLOAT = 76,
        R16G16_UNORM = 77,
        R16G16_SNORM = 78,
        R16G16_USCALED = 79,
        R16G16_SSCALED = 80,
        R16G16_UINT = 81,
        R16G16_SINT = 82,
        R16G16_SFLOAT = 83,
        R16G16B16_UNORM = 84,
        R16G16B16_SNORM = 85,
        R16G16B16_USCALED = 86,
        R16G16B16_SSCALED = 87,
        R16G16B16_UINT = 88,
        R16G16B16_SINT = 89,
        R16G16B16_SFLOAT = 90,
        R16G16B16A16_UNORM = 91,
        R16G16B16A16_SNORM = 92,
        R16G16B16A16_USCALED = 93,
        R16G16B16A16_SSCALED = 94,
        R16G16B16A16_UINT = 95,
        R16G16B16A16_SINT = 96,
        R16G16B16A16_SFLOAT = 97,
        R32_UINT = 98,
        R32_SINT = 99,
        R32_SFLOAT = 100,
        R32G32_UINT = 101,
        R32G32_SINT = 102,
        R32G32_SFLOAT = 103,
        R32G32B32_UINT = 104,
        R32G32B32_SINT = 105,
        R32G32B32_SFLOAT = 106,
        R32G32B32A32_UINT = 107,
        R32G32B32A32_SINT = 108,
        R32G32B32A32_SFLOAT = 109,
        R64_UINT = 110,
        R64_SINT = 111,
        R64_SFLOAT = 112,
        R64G64_UINT = 113,
        R64G64_SINT = 114,
        R64G64_SFLOAT = 115,
        R64G64B64_UINT = 116,
        R64G64B64_SINT = 117,
        R64G64B64_SFLOAT = 118,
        R64G64B64A64_UINT = 119,
        R64G64B64A64_SINT = 120,
        R64G64B64A64_SFLOAT = 121,
        D16_UNORM = 124,
        D32_SFLOAT = 126,
        S8_UINT = 127,
        D16_UNORM_S8_UINT = 128,
        D24_UNORM_S8_UINT = 129,
        D32_SFLOAT_S8_UINT = 130
    };


    struct ImageCreateInfo
    {
        VkFormat                 format = VK_FORMAT_UNDEFINED;
        uint32_t                 width = 1;
        uint32_t                 height = 1;
        uint32_t                 mipLevels = 1;
        VkImageUsageFlags        usage = 0;
    };

    class Image : public VkObject<VkImage> 
    {
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
    public:
        uint32_t width = 0, height = 0;
        uint32_t mipLevels = 1;
        uint32_t layers = 1;
        VkFormat format = VK_FORMAT_UNDEFINED;

        Image() = default;
        Image(VkImage img) { m_RendererID = img; }

        VkResult Create(const VkImageCreateInfo& dimg_info, VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY) 
        {
            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = usage;
            width = dimg_info.extent.width;
            height = dimg_info.extent.height;
            mipLevels = dimg_info.mipLevels;
            layers = dimg_info.arrayLayers;
            format = dimg_info.format;

            return vmaCreateImage(VulkanContext::GetMemoryAllocator(), &dimg_info, &dimg_allocinfo, &m_RendererID, &m_Allocation, nullptr);
        }

        VkResult Create(const ImageCreateInfo& imageInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY) 
        {
            VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;

            imageCreateInfo.format = imageInfo.format;

            imageCreateInfo.extent.width = imageInfo.width;
            imageCreateInfo.extent.height = imageInfo.height;
            imageCreateInfo.extent.depth = 1;

            imageCreateInfo.mipLevels = imageInfo.mipLevels;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = imageInfo.usage;
            return Create(imageCreateInfo, usage);
        }

        void Destroy() 
        {
            vmaDestroyImage(VulkanContext::GetMemoryAllocator(), m_RendererID, m_Allocation);
            m_RendererID = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }

        glm::ivec2 GetMipSize(int level) const 
        {
            glm::ivec2 size = { width, height };
            while (level != 0)
            {
                size.x /= 2;
                size.y /= 2;
                level--;
            }

            return size;
        }

        glm::ivec2 GetSize() const { return { width, height }; }
        float GetAspectRatio() const { return (float)width / (float)height; }

        int GetMipLevelCount()
        {
            glm::vec2 textureSize = { width, height };
            return (int)glm::floor(glm::log2(glm::min(textureSize.x, textureSize.y)));
        }

        void insertMemoryBarrier(
            const VkCommandBuffer& cmdbuffer,
            const VkAccessFlags& srcAccessMask,
            const VkAccessFlags& dstAccessMask,
            const VkImageLayout& oldImageLayout,
            const VkImageLayout& newImageLayout,
            const VkPipelineStageFlags& srcStageMask,
            const VkPipelineStageFlags& dstStageMask,
            const VkImageSubresourceRange& subresourceRange)
        {
            VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = srcAccessMask;
            imageMemoryBarrier.dstAccessMask = dstAccessMask;
            imageMemoryBarrier.oldLayout = oldImageLayout;
            imageMemoryBarrier.newLayout = newImageLayout;
            imageMemoryBarrier.image = m_RendererID;
            imageMemoryBarrier.subresourceRange = subresourceRange;

            vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        void insertMemoryBarrier(
            const VkCommandBuffer& cmdbuffer,
            const VkImageAspectFlags& aspectMask,
            const VkAccessFlags& srcAccessMask,
            const VkAccessFlags& dstAccessMask,
            const VkImageLayout& oldImageLayout,
            const VkImageLayout& newImageLayout,
            const VkPipelineStageFlags& srcStageMask,
            const VkPipelineStageFlags& dstStageMask)
        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = aspectMask;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.layerCount = layers;

            insertMemoryBarrier(cmdbuffer, srcAccessMask,
                dstAccessMask,
                oldImageLayout,
                newImageLayout,
                srcStageMask,
                dstStageMask, subresourceRange);
        }

        void setLayout(
            const VkCommandBuffer& cmdbuffer,
            const VkImageLayout& oldImageLayout,
            const VkImageLayout& newImageLayout,
            const VkImageSubresourceRange& subresourceRange,
            const VkPipelineStageFlags& srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            const VkPipelineStageFlags& dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            // Create an image barrier object
            VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.oldLayout = oldImageLayout;
            imageMemoryBarrier.newLayout = newImageLayout;
            imageMemoryBarrier.image = m_RendererID;
            imageMemoryBarrier.subresourceRange = subresourceRange;

            // Source layouts (old)
            // Source access mask controls actions that have to be finished on the old layout
            // before it will be transitioned to the new layout
            switch (oldImageLayout)
            {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source 
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            // Target layouts (new)
            // Destination access mask controls the dependency for the new image layout
            switch (newImageLayout)
            {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0)
                {
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            // Put barrier inside setup command buffer
            vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        // Fixed sub resource on first mip level and layer
        void setLayout(
            const VkCommandBuffer& cmdbuffer,
            const VkImageAspectFlags& aspectMask,
            const VkImageLayout& oldImageLayout,
            const VkImageLayout& newImageLayout,
            const VkPipelineStageFlags& srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            const VkPipelineStageFlags& dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = aspectMask;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.layerCount = layers;
            setLayout(cmdbuffer, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
        }

        VkSubresourceLayout SubresourceLayout(const VkImageAspectFlagBits& aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) 
        {
            VkImageSubresource subResource{};
            subResource.aspectMask = aspectMask;
            VkSubresourceLayout subResourceLayout;

            vkGetImageSubresourceLayout(VulkanContext::GetLogicalDevice(), m_RendererID, &subResource, &subResourceLayout);
            return subResourceLayout;
        }

        /**
        * @brief Returns true if the attachment has a depth component
        */
        bool hasDepth() const
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

        /**
        * @brief Returns true if the attachment has a stencil component
        */
        bool hasStencil() const
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

        /**
        * @brief Returns true if the attachment is a depth and/or stencil attachment
        */
        bool isDepthStencil() const { return(hasDepth() || hasStencil()); }
    };

    struct ImageView : public VkObject<VkImageView> 
    {
        ImageView() = default;
        ImageView(VkImageView view) { m_RendererID = view; }
        VkResult Create(const VkImageViewCreateInfo& createInfo) 
        { return vkCreateImageView(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_RendererID); }

        VkResult Create(const VkFormat& format, const VkImage& image, const VkImageAspectFlags& aspectFlags, const VkImageViewType& viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t levelCount = 1, uint32_t layerCount = 1)
        {
            //build a image-view for the depth image to use for rendering
            VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

            info.viewType = viewType;
            info.image = image;
            info.format = format;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = levelCount;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = layerCount;
            info.subresourceRange.aspectMask = aspectFlags;

            return Create(info);
        }

        VkResult Create(const Image& image)
        {
            //build a image-view for the depth image to use for rendering
            VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

            info.viewType = image.layers > 0 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            info.image = image;
            info.format = image.format;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = image.mipLevels;
            info.subresourceRange.baseArrayLayer = 0;
            info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            info.subresourceRange.layerCount = image.layers;
            info.subresourceRange.aspectMask = image.hasDepth() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

            return Create(info);
        }

        void Destroy() 
        {
            vkDestroyImageView(VulkanContext::GetLogicalDevice(), m_RendererID, nullptr);
            m_RendererID = VK_NULL_HANDLE;
        }
    };

    enum class Filter 
    {
        NEAREST = 0,
        LINEAR = 1,
    };

    enum class SamplerMipmapMode 
    {
        NEAREST = 0,
        LINEAR = 1,
    };

    enum SamplerAddressMode 
    {
        REPEAT = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE = 2,
        CLAMP_TO_BORDER = 3
    };

    struct SamplerCreateInfo 
    {
        Filter                magFilter = Filter::NEAREST;
        Filter                minFilter = Filter::NEAREST;
        SamplerMipmapMode     mipmapMode = SamplerMipmapMode::NEAREST;
        SamplerAddressMode    addressModeU = SamplerAddressMode::REPEAT;
        SamplerAddressMode    addressModeV = SamplerAddressMode::REPEAT;
        SamplerAddressMode    addressModeW = SamplerAddressMode::REPEAT;
        float                 mipLodBias = 0.f;
        bool                  anisotropyEnable = false;
        float                 maxAnisotropy = 1.f;
        float                 minLod = 0.f;
        float                 maxLod = 1.f;
    };

    struct Sampler : public VkObject<VkSampler> 
    {
        Sampler() = default;
        Sampler(const VkSampler& sampler) { m_RendererID = sampler; }

        VkResult Create(const VkSamplerCreateInfo& create_info) 
        { return vkCreateSampler(VulkanContext::GetLogicalDevice(), &create_info, nullptr, &m_RendererID); }

        VkResult Create(const SamplerCreateInfo& info) 
        {
            VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

            createInfo.magFilter = (VkFilter)info.magFilter;
            createInfo.minFilter = (VkFilter)info.minFilter;
            createInfo.mipmapMode = (VkSamplerMipmapMode)info.mipmapMode;
            createInfo.addressModeU = (VkSamplerAddressMode)info.addressModeU;
            createInfo.addressModeV = (VkSamplerAddressMode)info.addressModeV;
            createInfo.addressModeW = (VkSamplerAddressMode)info.addressModeW;
            createInfo.mipLodBias = info.mipLodBias;
            createInfo.anisotropyEnable = info.anisotropyEnable;
            createInfo.maxAnisotropy = info.maxAnisotropy;
            createInfo.compareEnable = false;
            createInfo.compareOp = VK_COMPARE_OP_NEVER;
            createInfo.minLod = info.minLod;
            createInfo.maxLod = info.maxLod;
            createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            createInfo.unnormalizedCoordinates = false;
            return Create(createInfo);
        }

        void Destroy() 
        {
            vkDestroySampler(VulkanContext::GetLogicalDevice(), m_RendererID, nullptr);
            m_RendererID = VK_NULL_HANDLE;
        }
    };

    inline VkDescriptorImageInfo GetDescriptorData(VkSampler sampler, VkImageView view, VkImageLayout imageLayout)
    {
        VkDescriptorImageInfo imageInfo;
        imageInfo.sampler = sampler;
        imageInfo.imageView = view;
        imageInfo.imageLayout = imageLayout;
        return imageInfo;
    }
}