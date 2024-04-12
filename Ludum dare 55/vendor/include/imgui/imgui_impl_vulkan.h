// dear imgui: Renderer Backend for Vulkan
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [x] Renderer: User texture binding. Use 'VkDescriptorSet' as ImTextureID. Read the FAQ about ImTextureID! See https://github.com/ocornut/imgui/pull/914 for discussions.
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
//  [x] Renderer: Multi-viewport / platform windows. With issues (flickering when creating a new viewport).

// Important: on 32-bit systems, user texture binding is only supported if your imconfig file has '#define ImTextureID ImU64'.
// See imgui_impl_vulkan.cpp file for details.

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#pragma once
#ifndef IMGUI_DISABLE
#include "imgui.h"      // IMGUI_IMPL_API

// [Configuration] in order to use a custom Vulkan function loader:
// (1) You'll need to disable default Vulkan function prototypes.
//     We provide a '#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES' convenience configuration flag.
//     In order to make sure this is visible from the imgui_impl_vulkan.cpp compilation unit:
//     - Add '#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES' in your imconfig.h file
//     - Or as a compilation flag in your build system
//     - Or uncomment here (not recommended because you'd be modifying imgui sources!)
//     - Do not simply add it in a .cpp file!
// (2) Call ImGui_ImplVulkan_LoadFunctions() before ImGui_ImplVulkan_Init() with your custom function.
// If you have no idea what this is, leave it alone!
//#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES

// Vulkan includes
#include <wc/vk/Buffer.h>
#include <wc/vk/Commands.h>
#include <wc/vk/Images.h>
#include <wc/vk/SyncContext.h>
#include <wc/vk/Descriptors.h>

// Initialization data, for ImGui_ImplVulkan_Init()
// - VkDescriptorPool should be created with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
//   and must contain a pool size large enough to hold an ImGui VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER descriptor.
// - When using dynamic rendering, set UseDynamicRendering=true and fill PipelineRenderingCreateInfo structure.
// [Please zero-clear before use!]
inline uint32_t ImguiMinImageCount = 2;                // >= 2
inline uint32_t ImguiImageCount = 2;                   // >= MinImageCount

// Called by user code
bool         ImGui_ImplVulkan_Init(VkRenderPass rp);
void         ImGui_ImplVulkan_Shutdown();
void         ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, wc::CommandBuffer cmd, VkRenderPassBeginInfo rpInfo, VkPipeline pipeline = VK_NULL_HANDLE);
bool         ImGui_ImplVulkan_CreateFontsTexture();
void         ImGui_ImplVulkan_DestroyFontsTexture();

//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
// (Used by example's main.cpp. Used by multi-viewport features. PROBABLY NOT used by your own engine/app.)
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_FrameSemaphores
{
    wc::Semaphore         ImageAcquiredSemaphore;
    wc::Semaphore         RenderCompleteSemaphore;

    void Destroy()
    {
        ImageAcquiredSemaphore.Destroy();
        RenderCompleteSemaphore.Destroy();
    }
};

struct ImGui_ImplVulkanH_Frame
{
    wc::CommandBuffer   CommandBuffer;
    wc::Fence           Fence;
    wc::Image           Backbuffer;
    wc::ImageView       BackbufferView;
    VkFramebuffer       Framebuffer = VK_NULL_HANDLE;

    void Destroy()
    {
        Fence.Destroy();
        SyncContext::CommandPool.Free(CommandBuffer);
        CommandBuffer = VK_NULL_HANDLE;

        BackbufferView.Destroy();
        vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), Framebuffer, VK_NULL_HANDLE);
        Framebuffer = VK_NULL_HANDLE;
    }
};

struct ImGui_ImplVulkanH_Window
{
    int                 Width = 0;
    int                 Height = 0;
    VkSwapchainKHR      Swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR        Surface = VK_NULL_HANDLE;
    VkSurfaceFormatKHR  SurfaceFormat = {};
    VkPresentModeKHR    PresentMode = (VkPresentModeKHR)~0;
    VkRenderPass        RenderPass = VK_NULL_HANDLE;
    VkPipeline          Pipeline = VK_NULL_HANDLE;      // The window pipeline may uses a different VkRenderPass than the one passed in ImGui_ImplVulkan_InitInfo
    bool                ClearEnable = true;
    VkClearValue        ClearValue = {};
    uint32_t            FrameIndex = 0;             // Current frame being rendered to (0 <= FrameIndex < FrameInFlightCount)
    uint32_t            ImageCount = 0;             // Number of simultaneous in-flight frames (returned by vkGetSwapchainImagesKHR, usually derived from min_image_count)
    uint32_t            SemaphoreCount = 0;         // Number of simultaneous in-flight frames + 1, to be able to use it in vkAcquireNextImageKHR
    uint32_t            SemaphoreIndex = 0;         // Current set of swapchain wait semaphores we're using (needs to be distinct from per frame data)
    ImGui_ImplVulkanH_Frame* Frames;
    ImGui_ImplVulkanH_FrameSemaphores* FrameSemaphores;

    void CreateOrResize(int width, int height, uint32_t min_image_count)
    {
        {
            VkResult err;
            VkSwapchainKHR old_swapchain = Swapchain;
            Swapchain = VK_NULL_HANDLE;
            err = vkDeviceWaitIdle(VulkanContext::GetLogicalDevice());

            // We don't use ImGui_ImplVulkanH_DestroyWindow() because we want to preserve the old swapchain to create the new one.
            // Destroy old Framebuffer
            for (uint32_t i = 0; i < ImageCount; i++)
                Frames[i].Destroy();
            for (uint32_t i = 0; i < SemaphoreCount; i++)
                FrameSemaphores[i].Destroy();
            IM_FREE(Frames);
            IM_FREE(FrameSemaphores);
            Frames = nullptr;
            FrameSemaphores = nullptr;
            ImageCount = 0;
            if (RenderPass)
                vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), RenderPass, VK_NULL_HANDLE);
            if (Pipeline)
                vkDestroyPipeline(VulkanContext::GetLogicalDevice(), Pipeline, VK_NULL_HANDLE);

            // If min image count was not specified, request different count of images dependent on selected present mode
            if (min_image_count == 0)
            {
                if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    min_image_count = 3;
                else if (PresentMode == VK_PRESENT_MODE_FIFO_KHR || PresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                    min_image_count = 2;
                else if (PresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    min_image_count = 1;
            }

            // Create Swapchain
            {
                VkSwapchainCreateInfoKHR info = {};
                info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                info.surface = Surface;
                info.minImageCount = min_image_count;
                info.imageFormat = SurfaceFormat.format;
                info.imageColorSpace = SurfaceFormat.colorSpace;
                info.imageArrayLayers = 1;
                info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
                info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                info.presentMode = PresentMode;
                info.clipped = VK_TRUE;
                info.oldSwapchain = old_swapchain;
                VkSurfaceCapabilitiesKHR cap;
                err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanContext::GetPhysicalDevice(), Surface, &cap);
                if (info.minImageCount < cap.minImageCount)
                    info.minImageCount = cap.minImageCount;
                else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
                    info.minImageCount = cap.maxImageCount;

                if (cap.currentExtent.width == 0xffffffff)
                {
                    info.imageExtent.width = Width = width;
                    info.imageExtent.height = Height = height;
                }
                else
                {
                    info.imageExtent.width = Width = cap.currentExtent.width;
                    info.imageExtent.height = Height = cap.currentExtent.height;
                }
                err = vkCreateSwapchainKHR(VulkanContext::GetLogicalDevice(), &info, VK_NULL_HANDLE, &Swapchain);
                err = vkGetSwapchainImagesKHR(VulkanContext::GetLogicalDevice(), Swapchain, &ImageCount, nullptr);
                VkImage backbuffers[16] = {};
                IM_ASSERT(ImageCount >= min_image_count);
                IM_ASSERT(ImageCount < IM_ARRAYSIZE(backbuffers));
                err = vkGetSwapchainImagesKHR(VulkanContext::GetLogicalDevice(), Swapchain, &ImageCount, backbuffers);

                IM_ASSERT(Frames == nullptr && FrameSemaphores == nullptr);
                SemaphoreCount = ImageCount + 1;
                Frames = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * ImageCount);
                FrameSemaphores = (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * SemaphoreCount);
                memset(Frames, 0, sizeof(Frames[0]) * ImageCount);
                memset(FrameSemaphores, 0, sizeof(FrameSemaphores[0]) * SemaphoreCount);
                for (uint32_t i = 0; i < ImageCount; i++)
                    Frames[i].Backbuffer = backbuffers[i];
            }
            if (old_swapchain)
                vkDestroySwapchainKHR(VulkanContext::GetLogicalDevice(), old_swapchain, VK_NULL_HANDLE);

            {
                VkAttachmentDescription attachment = {};
                attachment.format = SurfaceFormat.format;
                attachment.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.loadOp = ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                VkAttachmentReference color_attachment = {};
                color_attachment.attachment = 0;
                color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &color_attachment;
                VkSubpassDependency dependency = {};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = 0;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                VkRenderPassCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.subpassCount = 1;
                info.pSubpasses = &subpass;
                info.dependencyCount = 1;
                info.pDependencies = &dependency;
                err = vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &info, VK_NULL_HANDLE, &RenderPass);

                // We do not create a pipeline by default as this is also used by examples' main.cpp,
                // but secondary viewport in multi-viewport mode may want to create one with:
                //ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, v->Subpass);
            }

            // Create The Image Views
            {
                VkImageViewCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = SurfaceFormat.format;
                info.components.r = VK_COMPONENT_SWIZZLE_R;
                info.components.g = VK_COMPONENT_SWIZZLE_G;
                info.components.b = VK_COMPONENT_SWIZZLE_B;
                info.components.a = VK_COMPONENT_SWIZZLE_A;
                VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                info.subresourceRange = image_range;
                for (uint32_t i = 0; i < ImageCount; i++)
                {
                    ImGui_ImplVulkanH_Frame* fd = &Frames[i];
                    info.image = fd->Backbuffer;
                    fd->BackbufferView.Create(info);
                }
            }

            {
                VkImageView attachment[1];
                VkFramebufferCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                info.renderPass = RenderPass;
                info.attachmentCount = 1;
                info.pAttachments = attachment;
                info.width = Width;
                info.height = Height;
                info.layers = 1;
                for (uint32_t i = 0; i < ImageCount; i++)
                {
                    ImGui_ImplVulkanH_Frame* fd = &Frames[i];
                    attachment[0] = fd->BackbufferView;
                    err = vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &info, VK_NULL_HANDLE, &fd->Framebuffer);
                }
            }
        }

        // Create Command Buffers
        for (uint32_t i = 0; i < ImageCount; i++)
        {
            ImGui_ImplVulkanH_Frame* fd = &Frames[i];
            SyncContext::CommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, fd->CommandBuffer);

            fd->Fence.Create(VK_FENCE_CREATE_SIGNALED_BIT);
        }

        for (uint32_t i = 0; i < SemaphoreCount; i++)
        {
            ImGui_ImplVulkanH_FrameSemaphores* fsd = &FrameSemaphores[i];
            fsd->ImageAcquiredSemaphore.Create();
            fsd->RenderCompleteSemaphore.Create();
        }
    }

    void Destroy()
    {
        vkDeviceWaitIdle(VulkanContext::GetLogicalDevice()); // FIXME: We could wait on the Queue if we had the queue in wd-> (otherwise VulkanH functions can't use globals)
        //vkQueueWaitIdle(bd->Queue);

        for (uint32_t i = 0; i < ImageCount; i++)
            Frames[i].Destroy();
        for (uint32_t i = 0; i < SemaphoreCount; i++)
            FrameSemaphores[i].Destroy();
        IM_FREE(Frames);
        IM_FREE(FrameSemaphores);
        Frames = nullptr;
        FrameSemaphores = nullptr;
        vkDestroyPipeline(VulkanContext::GetLogicalDevice(), Pipeline, VK_NULL_HANDLE);
        vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), RenderPass, VK_NULL_HANDLE);
        vkDestroySwapchainKHR(VulkanContext::GetLogicalDevice(), Swapchain, VK_NULL_HANDLE);
        vkDestroySurfaceKHR(VulkanContext::GetInstance(), Surface, VK_NULL_HANDLE);

        *this = ImGui_ImplVulkanH_Window();
    }
};

// Reusable buffers used for rendering 1 current in-flight frame, for ImGui_ImplVulkan_RenderDrawData()
struct ImGui_ImplVulkan_FrameRenderBuffers
{
    wc::Buffer          VertexBuffer;
    wc::Buffer          IndexBuffer;

    void Destroy()
    {
        VertexBuffer.Free();
        IndexBuffer.Free();
    }
};

// Each viewport will hold 1 ImGui_ImplVulkanH_WindowRenderBuffers
// [Please zero-clear before use!]
struct ImGui_ImplVulkan_WindowRenderBuffers
{
    uint32_t            Index = 0;
    uint32_t            Count = 0;
    ImGui_ImplVulkan_FrameRenderBuffers* FrameRenderBuffers;

    void Destroy()
    {
        for (uint32_t n = 0; n < Count; n++)
            FrameRenderBuffers[n].Destroy();
        IM_FREE(FrameRenderBuffers);
        FrameRenderBuffers = nullptr;
        Index = 0;
        Count = 0;
    }
};

// For multi-viewport support:
// Helper structure we store in the void* RendererUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplVulkan_ViewportData
{
    bool                                    WindowOwned = false;
    ImGui_ImplVulkanH_Window                Window;             // Used by secondary viewports only
    ImGui_ImplVulkan_WindowRenderBuffers    RenderBuffers;      // Used by all viewports
};

// Vulkan data
struct ImGui_ImplVulkan_Data
{
    VkRenderPass                RenderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout       DescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout            PipelineLayout = VK_NULL_HANDLE;
    VkPipeline                  Pipeline = VK_NULL_HANDLE;

    // Font data
    wc::Sampler                 FontSampler;
    wc::Image                   FontImage;
    wc::ImageView               FontView;
    VkDescriptorSet             FontDescriptorSet = VK_NULL_HANDLE;

    // Render buffers for main window
    ImGui_ImplVulkan_WindowRenderBuffers MainWindowRenderBuffers;
};

static ImGui_ImplVulkan_Data* ImGui_ImplVulkan_GetBackendData() { return ImGui::GetCurrentContext() ? (ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData : nullptr; }

inline VkDescriptorSet MakeImGuiDescriptor(VkDescriptorSet dSet, const VkDescriptorImageInfo& imageInfo)
{
    if (dSet == VK_NULL_HANDLE) wc::descriptorAllocator.allocate(dSet, ImGui_ImplVulkan_GetBackendData()->DescriptorSetLayout);

    wc::DescriptorWriter writer;
    writer.dstSet = dSet;
    writer.write_image(0, imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.Update();

    return dSet;
}

#endif // #ifndef IMGUI_DISABLE
