#pragma once
#include "Globals.h"
#include "wc/vk/SyncContext.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/ImGuizmo.h>

namespace wc
{
	class Application 
	{

		//----------------------------------------------------------------------------------------------------------------------
		bool IsEngineOK() { return Globals.window.IsOpen(); }
		//----------------------------------------------------------------------------------------------------------------------
		void Resize() 
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(Globals.window, &width, &height);
			while (width == 0 || height == 0) 
			{
				glfwGetFramebufferSize(Globals.window, &width, &height);
				glfwWaitEvents();
			}

			VulkanContext::GetLogicalDevice().WaitIdle();
			Globals.window.DestoySwapchain();
			Globals.window.CreateSwapchain(VulkanContext::GetPhysicalDevice(), VulkanContext::GetLogicalDevice(), VulkanContext::GetInstance());
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnCreate() 
		{
			VulkanContext::Create();

			WindowCreateInfo windowInfo;
			windowInfo.Width = 1280;
			windowInfo.Height = 720;
			windowInfo.Resizeable = true;
			windowInfo.AppName = "Ludum dare 55";
			windowInfo.StartMode = WindowMode::Maximized;
			Globals.window.Create(windowInfo);

			SyncContext::Create();

			descriptorAllocator.Create();

			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.IniFilename = nullptr;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			//io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/fonts/OpenSans-Regular.ttf", 17.f);

			ImGui_ImplGlfw_Init(Globals.window, false);

			ImGui_ImplVulkan_Init(Globals.window.DefaultRenderPass);
			ImGui_ImplVulkan_CreateFontsTexture();

			auto& style = ImGui::GetStyle();
			style.WindowMenuButtonPosition = ImGuiDir_None;
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnInput() 
		{
			Globals.window.PoolEvents();

			if (Globals.window.resized) Resize();
		}

		void OnUpdate()
		{
			Globals.UpdateTime();

			uint32_t swapchainImageIndex = 0;

			VkResult result = Globals.window.AcquireNextImage(swapchainImageIndex, SyncContext::PresentSemaphore);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize();
				return;
			}

			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();

			ImGui::Render();			

			CommandBuffer& cmd = SyncContext::MainCommandBuffer;
			
			VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

			rpInfo.renderPass = Globals.window.DefaultRenderPass;
			rpInfo.framebuffer = Globals.window.Framebuffers[swapchainImageIndex];
			rpInfo.renderArea.extent = Globals.window.GetExtent();
			// This section maybe useless (implemented to satisfy vk validation)
			rpInfo.clearValueCount = 1;
			VkClearValue clearValue = {};
			clearValue.color = { 0.f, 0.f, 0.f, 0.f };
			rpInfo.pClearValues = &clearValue;

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, rpInfo, VK_NULL_HANDLE);

			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			auto& io = ImGui::GetIO();

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}


			VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = cmd.GetPointer();

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submit.pWaitDstStageMask = &waitStage;

			submit.waitSemaphoreCount = 1;
			submit.pWaitSemaphores = SyncContext::PresentSemaphore.GetPointer();

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = SyncContext::RenderSemaphore.GetPointer();

			//submit command buffer to the queue and execute it.
			// renderFence will now block until the graphic commands finish execution
			VulkanContext::graphicsQueue.Submit(submit, SyncContext::RenderFence);

			SyncContext::RenderFence.Wait();
			SyncContext::RenderFence.Reset();
			cmd.Reset();
			VkResult presentationResult = Globals.window.Present(swapchainImageIndex, SyncContext::RenderSemaphore, SyncContext::GetPresentQueue());


			if (presentationResult == VK_ERROR_OUT_OF_DATE_KHR || presentationResult == VK_SUBOPTIMAL_KHR || Globals.window.resized)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Globals.window.resized = false;
				Resize();
			}
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnDelete()
		{
			VulkanContext::GetLogicalDevice().WaitIdle();
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();

			descriptorAllocator.Destroy();
						
			SyncContext::Destroy();
			Globals.window.Destroy();

			VulkanContext::Destroy();
		}
		//----------------------------------------------------------------------------------------------------------------------
	public:

		void Start()
		{
			OnCreate();

			while (IsEngineOK())
			{
				OnInput();

				OnUpdate();
			}

			OnDelete();
		}
	};
}