#pragma once
#define GLFW_EXPOSE_NATIVE_WIN32
#undef min
#undef max

#include "../vk/VulkanContext.h"
#include "../vk/Images.h"


#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include "Log.h"
#include <imgui/imgui_impl_glfw.h>

namespace wc 
{
    // @TODO: Implement and remake the whole event system
    enum class Action : uint8_t
    {
        RELEASED = 0,
        JUST_RELEASED,
        PRESSED,
        HOLD
    };

    double scrollX = 0.f, scrollY = 0.f;
    int MouseButtons[GLFW_MOUSE_BUTTON_LAST];
    int KeyButtons[GLFW_KEY_LAST];
    glm::uvec2 cursorPosition = glm::uvec2(0);

    namespace Key 
    {
        using KeyCode = uint16_t;

        enum : KeyCode 
        {
            // From glfw3.h
            Space = 32,
            Apostrophe = 39, /* ' */
            Comma = 44, /* , */
            Minus = 45, /* - */
            Period = 46, /* . */
            Slash = 47, /* / */

            D0 = 48, /* 0 */
            D1 = 49, /* 1 */
            D2 = 50, /* 2 */
            D3 = 51, /* 3 */
            D4 = 52, /* 4 */
            D5 = 53, /* 5 */
            D6 = 54, /* 6 */
            D7 = 55, /* 7 */
            D8 = 56, /* 8 */
            D9 = 57, /* 9 */

            Semicolon = 59, /* ; */
            Equal = 61, /* = */

            A = 65,
            B = 66,
            C = 67,
            D = 68,
            E = 69,
            F = 70,
            G = 71,
            H = 72,
            I = 73,
            J = 74,
            K = 75,
            L = 76,
            M = 77,
            N = 78,
            O = 79,
            P = 80,
            Q = 81,
            R = 82,
            S = 83,
            T = 84,
            U = 85,
            V = 86,
            W = 87,
            X = 88,
            Y = 89,
            Z = 90,

            LeftBracket = 91,  /* [ */
            Backslash = 92,  /* \ */
            RightBracket = 93,  /* ] */
            GraveAccent = 96,  /* ` */

            World1 = 161, /* non-US #1 */
            World2 = 162, /* non-US #2 */

            /* Function keys */
            Escape = 256,
            Enter = 257,
            Tab = 258,
            Backspace = 259,
            Insert = 260,
            Delete = 261,
            Right = 262,
            Left = 263,
            Down = 264,
            Up = 265,
            PageUp = 266,
            PageDown = 267,
            Home = 268,
            End = 269,
            CapsLock = 280,
            ScrollLock = 281,
            NumLock = 282,
            PrintScreen = 283,
            Pause = 284,
            F1 = 290,
            F2 = 291,
            F3 = 292,
            F4 = 293,
            F5 = 294,
            F6 = 295,
            F7 = 296,
            F8 = 297,
            F9 = 298,
            F10 = 299,
            F11 = 300,
            F12 = 301,
            F13 = 302,
            F14 = 303,
            F15 = 304,
            F16 = 305,
            F17 = 306,
            F18 = 307,
            F19 = 308,
            F20 = 309,
            F21 = 310,
            F22 = 311,
            F23 = 312,
            F24 = 313,
            F25 = 314,

            /* Keypad */
            KP0 = 320,
            KP1 = 321,
            KP2 = 322,
            KP3 = 323,
            KP4 = 324,
            KP5 = 325,
            KP6 = 326,
            KP7 = 327,
            KP8 = 328,
            KP9 = 329,
            KPDecimal = 330,
            KPDivide = 331,
            KPMultiply = 332,
            KPSubtract = 333,
            KPAdd = 334,
            KPEnter = 335,
            KPEqual = 336,

            LeftShift = 340,
            LeftControl = 341,
            LeftAlt = 342,
            LeftSuper = 343,
            RightShift = 344,
            RightControl = 345,
            RightAlt = 346,
            RightSuper = 347,
            Menu = 348
        };

        int GetKey(KeyCode keyCode) { return KeyButtons[keyCode]; }
    }

    namespace Mouse 
    {
        using MouseCode = uint16_t;

        enum : MouseCode 
        {
            Button0 = 0,
            Button1 = 1,
            Button2 = 2,
            Button3 = 3,
            Button4 = 4,
            Button5 = 5,
            Button6 = 6,
            Button7 = 7,

            LAST = Button7,
            LEFT = Button0,
            RIGHT = Button1,
            MIDDLE = Button2
        };

        int GetMouse(MouseCode key) { return MouseButtons[key]; }
        glm::uvec2 GetCursorPosition() { return cursorPosition; }

        glm::vec2 GetMouseScroll() { return glm::vec2(scrollX, scrollY); }
    }

    enum WindowMode { Normal, Maximized, Fullscreen };

    struct WindowCreateInfo 
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        WindowMode StartMode = WindowMode::Normal;
        bool VSync = false;
        std::string AppName;
        bool Decorated = true;
        bool Resizeable = true;
    };

    const char* GetClipboard() { return glfwGetClipboardString(nullptr); }
    void SetClipboard(const std::string& string) { glfwSetClipboardString(nullptr, string.c_str()); }

    class Window 
    {
    public:
        void Create(const WindowCreateInfo& info)
        {
            if (info.StartMode == WindowMode::Maximized)
                glfwWindowHint(GLFW_MAXIMIZED, true);
            else if (info.StartMode == WindowMode::Fullscreen)
                m_Monitor = glfwGetPrimaryMonitor();

            glfwWindowHint(GLFW_RESIZABLE, info.Resizeable);

            m_Window = glfwCreateWindow(info.Width, info.Height, info.AppName.c_str(), m_Monitor, nullptr);
            glfwSetWindowUserPointer(m_Window, this);

            glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset) {
                scrollX = xoffset; scrollY = yoffset;

                ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
                });

            glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int, int) { static_cast<Window*>(glfwGetWindowUserPointer(window))->resized = true; });

            glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codepoint) {
                ImGui_ImplGlfw_CharCallback(window, codepoint);
                });

            glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
                KeyButtons[key] = action;

                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
                });

            glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos) {
                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
                cursorPosition = glm::uvec2(xpos, ypos);
                });

            glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
                MouseButtons[button] = action;
                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
                });

            glfwSetCursorEnterCallback(m_Window, [](GLFWwindow* window, int entered) {
                ImGui_ImplGlfw_CursorEnterCallback(window, entered);
                });

            glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* window, int focused) {
                ImGui_ImplGlfw_WindowFocusCallback(window, focused);
                });

            CreateSwapchain(VulkanContext::GetPhysicalDevice(), VulkanContext::GetLogicalDevice(), VulkanContext::GetInstance());
        }

        void CreateSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkInstance instance) 
        {
            if (glfwCreateWindowSurface(instance, m_Window, VulkanContext::GetAllocator(), &surface) != VK_SUCCESS) WC_CORE_ERROR("Failed to create window surface!");

            struct SwapChainSupportDetails 
            {
                VkSurfaceCapabilitiesKHR capabilities = {};
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> presentModes;
            };

            SwapChainSupportDetails swapChainSupport;
            {
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainSupport.capabilities);

                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

                if (formatCount != 0) 
                {
                    swapChainSupport.formats.resize(formatCount);
                    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainSupport.formats.data());
                }

                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

                if (presentModeCount != 0) {
                    swapChainSupport.presentModes.resize(presentModeCount);
                    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapChainSupport.presentModes.data());
                }
            }


            VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
            {
                for (const auto& availableFormat : swapChainSupport.formats)
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        surfaceFormat = availableFormat;
                        break;
                    }
            }
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
            {
                for (const auto& availablePresentMode : swapChainSupport.presentModes)
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
                    {
                        presentMode = availablePresentMode;
                        break;
                    }
            }
            VkExtent2D extent;
            {
                auto& capabilities = swapChainSupport.capabilities;
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                    extent = capabilities.currentExtent;
                else 
                {
                    VkExtent2D actualExtent = GetFramebufferExtent();

                    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                    extent = actualExtent;
                }
            }

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
                imageCount = swapChainSupport.capabilities.maxImageCount;

            VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            //QueueFamilyIndices indices = findQueueFamilies(physicalDevice/*, surface*/);
            //uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value()/*, indices.presentFamily.value()*/ };

            //if (indices.graphicsFamily != indices.presentFamily) {
            //	createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            //	createInfo.queueFamilyIndexCount = 2;
            //	createInfo.pQueueFamilyIndices = queueFamilyIndices;
            //}
            //else 
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;


            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = true;

            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(device, &createInfo, VulkanContext::GetAllocator(), &swapchain) != VK_SUCCESS)
                WC_CORE_ERROR("failed to create swap chain!");

            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
            swapchainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

            swapchainImageFormat = surfaceFormat.format;

            swapchainImageViews.resize(swapchainImages.size());

            for (size_t i = 0; i < swapchainImages.size(); i++) {
                VkImageViewCreateInfo viewCreateInfo{};
                viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCreateInfo.image = swapchainImages[i];
                viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewCreateInfo.format = swapchainImageFormat;
                viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewCreateInfo.subresourceRange.baseMipLevel = 0;
                viewCreateInfo.subresourceRange.levelCount = 1;
                viewCreateInfo.subresourceRange.baseArrayLayer = 0;
                viewCreateInfo.subresourceRange.layerCount = 1;

                if (swapchainImageViews[i].Create(viewCreateInfo) != VK_SUCCESS)
                    WC_CORE_ERROR("Failed to create image views!");
            }

            CreateDefaultRenderPass();
        }

        VkResult AcquireNextImage(uint32_t& swapchainImageIndex, VkSemaphore semaphore = nullptr, VkFence fence = nullptr, uint32_t timeout = 1000000000)
        {
            return vkAcquireNextImageKHR(VulkanContext::GetLogicalDevice(), swapchain, timeout, semaphore, fence, &swapchainImageIndex);
        }

        void DestoySwapchain() 
        {
            DestroyDefaultRenderPass();

            vkDestroySwapchainKHR(VulkanContext::GetLogicalDevice(), swapchain, VulkanContext::GetAllocator());
            vkDestroySurfaceKHR(VulkanContext::GetInstance(), surface, VulkanContext::GetAllocator());
            swapchain = VK_NULL_HANDLE;
            surface = VK_NULL_HANDLE;

            for (auto& view : swapchainImageViews)
                view.Destroy();
        }

        void SetCursorPosCallback(const GLFWcursorposfun& callback) const { glfwSetCursorPosCallback(m_Window, callback); }

        void SetFramebufferSizeCallback(const GLFWframebuffersizefun& callback) { glfwSetFramebufferSizeCallback(m_Window, callback); }

        void SetScrollCallback(const GLFWscrollfun& callback) { glfwSetScrollCallback(m_Window, callback); }

        void SetCharCallback(const GLFWcharfun& callback) { glfwSetCharCallback(m_Window, callback); }

        void SetMouseButtonCallback(const GLFWmousebuttonfun& callback) { glfwSetMouseButtonCallback(m_Window, callback); }

        void SetKeyCallback(const GLFWkeyfun& callback) { glfwSetKeyCallback(m_Window, callback); }

        void SetFocus(bool focus = true) { glfwWindowHint(GLFW_FOCUSED, focus); }

        void Destroy() 
        { 
            DestoySwapchain();
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }

        float GetContentScale() 
        {
            float xscale, yscale;
            glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
            return xscale;
        }

        float GetAspectRatio()
        {
            auto size = GetSize();
            return float((float)size.x / (float)size.y);
        }

        void PoolEvents() const 
        {
            scrollY = scrollX = 0.f;
            glfwPollEvents();
        }

        glm::ivec2 GetPos() const
        {
            int xpos, ypos;
            glfwGetWindowPos(m_Window, &xpos, &ypos);
            return { xpos, ypos };
        }

        glm::ivec2 GetSize() const 
        {
            int width, height;
            glfwGetWindowSize(m_Window, &width, &height);
            return { width, height };
        }

        VkExtent2D GetExtent() const 
        {
            int width, height;
            glfwGetWindowSize(m_Window, &width, &height);
            return { (uint32_t)width, (uint32_t)height };
        }

        VkExtent2D GetFramebufferExtent() const 
        {
            int width, height;
            glfwGetFramebufferSize(m_Window, &width, &height);
            return { (uint32_t)width, (uint32_t)height };
        }

        void Close(bool value = true) const { glfwSetWindowShouldClose(m_Window, value); }

        bool IsOpen() const { return !glfwWindowShouldClose(m_Window); }

        bool HasFocus() const { return glfwGetWindowAttrib(m_Window, GLFW_FOCUSED); }

        void SetCursorPos(glm::ivec2 pos) { glfwSetCursorPos(m_Window, pos.x, pos.y); }

        void SetMaximized(bool maximized) 
        {
            if (maximized) glfwMaximizeWindow(m_Window);
            else glfwRestoreWindow(m_Window);
        }

        void SetPosition(glm::ivec2 pos) { glfwSetWindowPos(m_Window, pos.x, pos.y); }

        void SetTitle(const std::string& title) { glfwSetWindowTitle(m_Window, title.c_str()); }

        void SetSize(glm::ivec2 size) { glfwSetWindowSize(m_Window, size.x, size.y); }

        void SetSizeLimits(glm::ivec2 minSize, glm::ivec2 maxSize) { glfwSetWindowSizeLimits(m_Window, minSize.x, minSize.y, maxSize.x, maxSize.y); }

        void SetCursorMode(int value) { glfwSetInputMode(m_Window, GLFW_CURSOR, value); }

        glm::ivec2 GetCursorPos() 
        {
            double x, y;
            glfwGetCursorPos(m_Window, &x, &y);
            return glm::ivec2(x, y);
        }

        VkResult Present(uint32_t swapchainImageIndex, VkSemaphore renderSemaphore, VkQueue presentQueue) 
        {
            VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

            presentInfo.pSwapchains = &swapchain;
            presentInfo.swapchainCount = 1;

            presentInfo.pWaitSemaphores = &renderSemaphore;
            presentInfo.waitSemaphoreCount = 1;

            presentInfo.pImageIndices = &swapchainImageIndex;

            return vkQueuePresentKHR(presentQueue, &presentInfo);
        }

        inline operator GLFWwindow* () { return m_Window; }
        inline operator GLFWwindow* () const { return m_Window; }

        // image format expected by the windowing system
        VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;

        //array of images from the swapchain
        std::vector<VkImage> swapchainImages;

        //array of image-views from the swapchain
        std::vector<ImageView> swapchainImageViews;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE; // from other articles
        VkSurfaceKHR surface = VK_NULL_HANDLE; // Vulkan window surface

        VkRenderPass DefaultRenderPass;
        std::vector<VkFramebuffer> Framebuffers;

        bool resized = false;

    private:
        void CreateDefaultRenderPass()
        {
            VkAttachmentDescription color_attachment = {};
            color_attachment.format = swapchainImageFormat;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment_ref = {};
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment_ref;
            subpass.pDepthStencilAttachment = nullptr;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
            render_pass_info.attachmentCount = 1;
            render_pass_info.pAttachments = &color_attachment;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;
            render_pass_info.dependencyCount = 1;
            render_pass_info.pDependencies = &dependency;

            vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &render_pass_info, VulkanContext::GetAllocator(), &DefaultRenderPass);
            VulkanContext::SetObjectName(DefaultRenderPass, "DefaultRenderPass");
            VkFramebufferCreateInfo fb_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };

            fb_info.renderPass = DefaultRenderPass;
            fb_info.attachmentCount = 1;
            fb_info.width = GetExtent().width;
            fb_info.height = GetExtent().height;
            fb_info.layers = 1;
            fb_info.attachmentCount = 1;

            uint32_t swapchain_imagecount = (uint32_t)swapchainImages.size();
            Framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

            for (uint32_t i = 0; i < swapchain_imagecount; i++)
            {
                fb_info.pAttachments = (VkImageView*)&swapchainImageViews[i];

                vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &fb_info, VulkanContext::GetAllocator(), &Framebuffers[i]);
                VulkanContext::SetObjectName(Framebuffers[i], std::format("Framebuffers[{}]", i).c_str());
            }
        }

        void DestroyDefaultRenderPass()
        {
            for (auto& framebuffer : Framebuffers)
            {
                vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), framebuffer, VulkanContext::GetAllocator());
                framebuffer = VK_NULL_HANDLE;
            }

            vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), DefaultRenderPass, VulkanContext::GetAllocator());
            DefaultRenderPass = VK_NULL_HANDLE;
        }
    private:

        GLFWwindow* m_Window = nullptr;
        GLFWmonitor* m_Monitor = nullptr;
    };
}