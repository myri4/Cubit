#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/ImGuizmo.h>

#include "game/Game.h"

namespace wc
{
	class Application 
	{
		GameInstance game;
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
			windowInfo.Width = 1920;
			windowInfo.Height = 1080;
			windowInfo.Resizeable = false;
			windowInfo.AppName = "Cube's Calling";
			windowInfo.StartMode = WindowMode::Normal;
			Globals.window.Create(windowInfo);

			SyncContext::Create();

			descriptorAllocator.Create();

			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.IniFilename = nullptr;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Masterpiece.ttf", 50.f);

			ImGui_ImplGlfw_Init(Globals.window, false);

			ImGui_ImplVulkan_Init(Globals.window.DefaultRenderPass);
			ImGui_ImplVulkan_CreateFontsTexture();

			auto& style = ImGui::GetStyle();
			style.WindowMenuButtonPosition = ImGuiDir_None;
			game.Create(Globals.window.GetSize());
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnInput() 
		{
			Globals.window.PoolEvents();

			if (Globals.window.resized) Resize();

			if (Globals.window.HasFocus() && Globals.gameState == GameState::PLAY)
				game.InputGame();
		}

		void Credits()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
 
			 
			 

			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(0.65f);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Mariyan Fakirov(aka myri) - Programming:  Game Engine and Graphics / Lead Programmer").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("Mariyan Fakirov(aka myri) - Programming:  Game Engine and Graphics / Lead Programmer").y) * 0.5f - 200));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "Mariyan Fakirov(aka myri) - Programming:  Game Engine and Graphics / Lead Programmer");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Martin Geshev(aka geshev) - Programming : Sound / Music / Sound Design").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("Martin Geshev(aka geshev) - Programming : Sound / Music / Sound Design").y) * 0.5f - 150));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "Martin Geshev(aka geshev) - Programming : Sound / Music / Sound Design");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Hristo Arabadzhiev(aka Icaka) - Programming : Player Mechanics and Movement / Level Design / Credits").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("Hristo Arabadzhiev(aka Icaka) - Programming : Player Mechanics and Movement / Level Design / Credits").y) * 0.5f - 100));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "Hristo Arabadzhiev(aka Icaka) - Programming : Player Mechanics and Movement / Level Design / Credits");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Georgi(aka gosho / jojo) - Programming : enemy AI").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("Georgi(aka gosho / jojo) - Programming : enemy AI").y) * 0.5f - 50));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "Georgi(aka gosho / jojo) - Programming : enemy AI");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Bulgaria na 3 moreta").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("Bulgaria na 3 moreta").y) * 0.5f));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "Bulgaria na 3 moreta");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 270) * 0.5f, (ImGui::GetWindowSize().y + 300) * 0.5f));
			if (ImGui::Button("Go Back", ImVec2(270, 100)))Globals.gameState = GameState::MENU;
			ImGui::End();
			ImGui::PopStyleVar(5);
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

			if (Globals.gameState == GameState::PLAY)
			{
				game.Update();
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			if (Globals.gameState == GameState::MENU) game.MENU();
			else if (Globals.gameState == GameState::DEATH) game.DEATH_MENU();
			else if (Globals.gameState == GameState::WIN) game.WIN_MENU();
			else if (Globals.gameState == GameState::PLAY) game.UI();
			else if (Globals.gameState == GameState::CREDITS) Credits();
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

			game.DestroyGame();

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