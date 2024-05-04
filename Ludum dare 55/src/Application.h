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
			//Globals.window.DestoySwapchain();
			//Globals.window.CreateSwapchain(VulkanContext::GetPhysicalDevice(), VulkanContext::GetLogicalDevice(), VulkanContext::GetInstance());
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnCreate() 
		{
			VulkanContext::Create();

			WindowCreateInfo windowInfo;
			windowInfo.Width = 1280;
			windowInfo.Height = 720;
			windowInfo.VSync = false;
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
			io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/ST-SimpleSquare.ttf", 50.f);

			ImGui_ImplGlfw_Init(Globals.window, false);

			ImGui_ImplVulkan_Init(Globals.window.DefaultRenderPass);
			ImGui_ImplVulkan_CreateFontsTexture();

			CreateAudio();

			auto& style = ImGui::GetStyle();
			style.WindowMenuButtonPosition = ImGuiDir_None;
			game.Create(Globals.window.GetSize());
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnInput() 
		{
			Globals.window.PoolEvents();

			if (Globals.window.resized) Resize();

			if (Globals.window.HasFocus() && Globals.gameState == GameState::PLAY) {
				game.InputGame();
				Globals.levelTime += Globals.deltaTime;
			}
		}

		

		void CreateAudio()
		{
			ma_engine_init(NULL, &Globals.sfx_engine);
			ma_engine_init(NULL, &Globals.music_engine);
			ma_engine_set_volume(&Globals.sfx_engine, Globals.sfx_volume);
			ma_engine_set_volume(&Globals.music_engine, 0.8f);
			ma_sound_init_from_file(&Globals.music_engine, "assets/sound/music/menu.wav", 0, 0, 0, &Globals.music_menu);
			ma_sound_init_from_file(&Globals.music_engine, "assets/sound/music/main_new.wav", 0, 0, 0, &Globals.music_main);
			ma_sound_init_from_file(&Globals.music_engine, "assets/sound/music/GameOver.wav", 0, 0, 0, &Globals.music_GameOver);
			ma_sound_init_from_file(&Globals.music_engine, "assets/sound/music/win_ver1.wav", 0, 0, 0, &Globals.music_win);
			ma_sound_set_looping(&Globals.music_menu, true);
			ma_sound_set_looping(&Globals.music_main, true);
		}

		bool main_menu = false;
		bool play = false;
		bool death = false;
		bool win = false;

		void UpdateMusic()
		{
			if (Globals.gameState == GameState::MENU && !main_menu)
			{
				main_menu = true;
				play = false;
				death = false;
				win = false;

				ma_sound_set_fade_in_milliseconds(&Globals.music_main, -1, 0, Globals.music_fade_time_mls);
				ma_sound_set_fade_in_milliseconds(&Globals.music_GameOver, -1, 0, Globals.music_fade_time_mls);
				ma_sound_set_fade_in_milliseconds(&Globals.music_win, -1, 0, Globals.music_fade_time_mls);

				ma_sound_seek_to_pcm_frame(&Globals.music_menu, 0);
				ma_sound_set_fade_in_milliseconds(&Globals.music_menu, 0, 1, Globals.music_fade_time_mls);
				ma_sound_start(&Globals.music_menu);
			}

			if (Globals.gameState == GameState::PLAY && !play)
			{
				play = true;
				main_menu = false;
				death = false;
				win = false;

				ma_sound_set_fade_in_milliseconds(&Globals.music_menu, -1, 0, Globals.music_fade_time_mls);

				ma_sound_seek_to_pcm_frame(&Globals.music_main, 0);
				ma_sound_set_fade_in_milliseconds(&Globals.music_main, 0, 1, Globals.music_fade_time_mls);
				ma_sound_start(&Globals.music_main);
			}

			if (Globals.gameState == GameState::DEATH && !death)
			{
				death = true;
				main_menu = false;
				play = false;
				win = false;

				ma_sound_set_fade_in_milliseconds(&Globals.music_main, -1, 0, Globals.music_fade_time_mls);

				ma_sound_seek_to_pcm_frame(&Globals.music_GameOver, 0);
				ma_sound_set_fade_in_milliseconds(&Globals.music_GameOver, 0, 1, Globals.music_fade_time_mls);
				ma_sound_start(&Globals.music_GameOver);
			}

			if (Globals.gameState == GameState::WIN && !win)
			{
				win = true;
				main_menu = false;
				death = false;
				play = false;

				ma_sound_set_fade_in_milliseconds(&Globals.music_main, -1, 0, Globals.music_fade_time_mls);

				ma_sound_seek_to_pcm_frame(&Globals.music_win, 0);
				ma_sound_set_fade_in_milliseconds(&Globals.music_win, 0, 1, Globals.music_fade_time_mls);
				ma_sound_start(&Globals.music_win);
			}
		}

		void DestroySound()
		{
			ma_sound_uninit(&Globals.music_menu);
			ma_sound_uninit(&Globals.music_main);
			ma_sound_uninit(&Globals.music_GameOver);
			ma_sound_uninit(&Globals.music_win);

			ma_engine_uninit(&Globals.music_engine);
			ma_engine_uninit(&Globals.sfx_engine);
		}

		void OnUpdate()
		{
			SyncContext::GetRenderFence().Wait();
			SyncContext::GetRenderFence().Reset();
			SyncContext::GetMainCommandBuffer().Reset();

			Globals.UpdateTime();

			UpdateMusic();

			uint32_t swapchainImageIndex = 0;

			VkResult result = Globals.window.AcquireNextImage(swapchainImageIndex, SyncContext::GetImageAvaibleSemaphore());

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Resize();
				return;
			}


			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::BeginFrame();
			if (Globals.gameState == GameState::MENU) game.MENU();
			else if (Globals.gameState == GameState::DEATH) game.DEATH_MENU();
			else if (Globals.gameState == GameState::WIN) game.WIN_MENU();
			else if (Globals.gameState == GameState::PLAY) game.UI();
			else if (Globals.gameState == GameState::SETTINGS) game.SETTINGS_MENU();
			ImGui::Render();			

			if (Globals.gameState == GameState::PLAY) game.Update();
			CommandBuffer& cmd = SyncContext::GetMainCommandBuffer();
			
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

			VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

			submit.pWaitDstStageMask = waitStage;

			Semaphore waitSemaphores[] = { SyncContext::GetImageAvaibleSemaphore(), m_Renderer.RenderSemaphore[CURRENT_FRAME],};

			submit.waitSemaphoreCount = ARRAYSIZE(waitSemaphores);
			submit.pWaitSemaphores = (VkSemaphore*)waitSemaphores;

			if (Globals.gameState != GameState::PLAY) submit.waitSemaphoreCount = 1;

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = SyncContext::GetRenderSemaphore().GetPointer();

			SyncContext::GetGraphicsQueue().Submit(submit, SyncContext::GetRenderFence());

			
			VkResult presentationResult = Globals.window.Present(swapchainImageIndex, SyncContext::GetRenderSemaphore(), SyncContext::GetPresentQueue());

			if (presentationResult == VK_ERROR_OUT_OF_DATE_KHR || presentationResult == VK_SUBOPTIMAL_KHR || Globals.window.resized)
			{
				VulkanContext::GetLogicalDevice().WaitIdle();
				Globals.window.resized = false;
				Resize();
			}

			SyncContext::UpdateFrame();
		}
		//----------------------------------------------------------------------------------------------------------------------
		void OnDelete()
		{
			VulkanContext::GetLogicalDevice().WaitIdle();
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();

			DestroySound();

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