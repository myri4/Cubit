#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include <wc/vk/SyncContext.h>

#include <wc/Utils/Time.h>

#include <wc/Utils/List.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/YAML.h>

// Physics
#include <box2d/box2d.h>

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../Globals.h"
#include "../Rendering/Renderer2D.h"
#include "UI/Widgets.h"

#include "Map.h"

namespace wc
{
	struct GameInstance
	{
	protected:
		Map m_Map;
		uint32_t m_LevelID = 0;

	public:	

		Texture MenuBackgroundTexture;
		Texture PlayButtonTexture;
		glm::vec2 TextureRatio;

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();
			m_Renderer.Init(m_Map.camera);
			m_Map.font.Load("assets/fonts/ST-SimpleSquare.ttf", m_RenderData);

			m_Tileset.Load();

			MenuBackgroundTexture.Load("assets/textures/cubitMenuConcept1.png");
			PlayButtonTexture.Load("assets/textures/button.png");
			TextureRatio = Globals.settings.WindowSize / (glm::vec2)MenuBackgroundTexture.GetSize();
			m_Map.SwordTexture = m_RenderData.LoadTexture("assets/textures/Sword.png");

			{
				auto& blaster = WeaponStats[(int)WeaponType::Blaster];
				blaster.BulletType = BulletType::Blaster;
				blaster.WeaponClass = WeaponClass::Primary;
				blaster.canZoom = true;
				blaster.Damage = 30;
				blaster.FireRate = 0.3f;
				blaster.MaxMag = 15;
				blaster.ReloadSpeed = 1.5f;
				blaster.Range = 50.f;
				blaster.BulletSpeed = 25.f;
				blaster.BulletSize = { 0.25f, 0.25f };
				blaster.Offset = { 0.25f, -0.15f };
				blaster.Size = { 1.f, 0.45f };
				blaster.TextureID = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			}

			{
				auto& laser = WeaponStats[(int)WeaponType::Laser];
				laser.WeaponClass = WeaponClass::Primary;
				laser.canZoom = true;
				laser.Damage = 60;
				laser.FireRate = 1.5f;
				laser.MaxMag = 5;
				laser.ReloadSpeed = 2.5f;
				laser.Range = 50.f;
				laser.BulletSize = { 0.25f, 0.25f };
				laser.Offset = { 0.15f, -0.15f };
				laser.Size = { 1.5f, 0.45f };
				laser.TextureID = m_RenderData.LoadTexture("assets/textures/LaserGun.png");
			}

			{
				auto& shotgun = WeaponStats[(int)WeaponType::Shotgun];
				shotgun.BulletType = BulletType::Shotgun;
				shotgun.WeaponClass = WeaponClass::Secondary;
				shotgun.Damage = 18;
				shotgun.FireRate = 1.1f;
				shotgun.MaxMag = 4;
				shotgun.ReloadSpeed = 0.5f;
				shotgun.ReloadByOne = true;
				shotgun.Range = 2.5f;
				shotgun.BulletSpeed = 25.f;
				shotgun.BulletSize = { 0.1f, 0.1f };
				shotgun.Offset = { 0.25f, -0.15f };
				shotgun.Size = { 1.f, 0.45f };
				shotgun.TextureID = m_RenderData.LoadTexture("assets/textures/Sawed-Off.png");
			}

			{
				auto& revolver = WeaponStats[(int)WeaponType::Revolver];
				revolver.BulletType = BulletType::Revolver;
				revolver.WeaponClass = WeaponClass::Secondary;
				revolver.Damage = 25;
				revolver.FireRate = 1.f;
				revolver.AltFireRate = 1.5f;
				revolver.MaxMag = 6;
				revolver.ReloadSpeed = 0.5f;
				revolver.ReloadByOne = true;
				revolver.Range = 25.f;
				revolver.Offset = { 0.25f, -0.15f };
				revolver.Size = { 1.f, 0.45f };
				revolver.BulletSpeed = 25.f;
				revolver.BulletSize = { 0.1f, 0.1f };
				revolver.TextureID = m_RenderData.LoadTexture("assets/textures/Revolver.png");
			}

			{
				auto& redBlaster = WeaponStats[(int)WeaponType::RedBlaster];
				redBlaster.WeaponClass = WeaponClass::EnemyWeapon;
				redBlaster.BulletType = BulletType::RedCircle;
				redBlaster.Damage = 5;
				redBlaster.FireRate = 0.3f;
				redBlaster.Range = 50.f;
				redBlaster.BulletSpeed = 25.f;
				redBlaster.BulletSize = { 0.25f, 0.25f };
				redBlaster.Offset = { 0.25f, -0.15f };
				redBlaster.Size = { 1.f, 0.45f };
				redBlaster.TextureID = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			}

			{
				auto& sword = WeaponStats[(int)WeaponType::Sword];
				sword.WeaponClass = WeaponClass::Melee;
				sword.Damage = 50;
				sword.FireRate = 2.5f;
				sword.Range = 15.f;
				sword.Size = { 1.f, 0.45f };
				sword.TextureID = m_RenderData.LoadTexture("assets/textures/Sword.png");
			}

			m_Renderer.CreateScreen(renderSize, m_RenderData);
			m_ParticleEmitter.Init();
			m_Particle.ColorBegin = { 0.99f, 0.83f, 0.48f, 1.f };
			m_Particle.ColorEnd = { 0.99f, 0.42f, 0.16f, 1.f };
			m_Particle.SizeBegin = 0.5f, m_Particle.SizeVariation = 0.3f, m_Particle.SizeEnd = 0.0f;
			m_Particle.LifeTime = 0.6f;
			m_Particle.Velocity = { 0.0f, 0.0f };
			m_Particle.VelocityVariation = { 6.f, 6.f };
			m_Particle.Position = { 0.0f, 0.0f };

			m_SummonParticle.ColorBegin = glm::vec4{ 1.f, 0.37f, 0.12f, 1.f } * 2.f;
			m_SummonParticle.ColorEnd = { 1.f, 0.37f, 0.12f, 1.0f };
			m_SummonParticle.SizeBegin = 0.5f, m_SummonParticle.SizeVariation = 0.3f, m_SummonParticle.SizeEnd = 0.0f;
			m_SummonParticle.LifeTime = 0.35f;
			m_SummonParticle.Velocity = { 0.0f, 0.0f };
			m_SummonParticle.VelocityVariation = { 6.f, 6.f };
			m_SummonParticle.Position = { 0.0f, 0.0f };			

			//m_SummonParticle.VelocityVariation = glm::normalize(entity.Position - m_Map.player.Position) * 2.5f;
		}
		
		void InputGame()
		{
			m_Map.InputGame();
		}		

		void Update()
		{
			m_Map.UpdateGame();
			m_Map.RenderGame();
		}

		void UI_Data()
		{
			auto color = ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f);
			ImGui::SetWindowFontScale(0.5f);
			ImGui::SetCursorPos(ImVec2(10.f, 10.f));
			ImGui::TextColored(color, std::format("FPS: {}", int(1.f / Globals.deltaTime)).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Enemy count: {}", m_Map.EnemyCount).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Level Time: {:.2f} sec.", m_Map.LevelTime).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Current Level: {}", m_LevelID).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Accumulator: {}", m_Map.player.Weapons[(int)m_Map.player.MeleeWeapon].Timer).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Ammo: {}/{}", m_Map.player.Weapons[(int)m_Map.player.Weapon].Magazine, m_Map.player.Weapons[(int)m_Map.player.Weapon].Ammo).c_str());
			//ImGui::SetCursorPosX(10.f);
			//ImGui::TextColored(color, std::format("Steps: {}", m_Map.nSteps).c_str());
			//ImGui::SetCursorPosX(10.f);
			//ImGui::TextColored(color, std::format("Clamped steps: {}", glm::min(m_Map.nSteps, m_Map.MAX_STEPS)).c_str());

			ImGui::SetCursorPosX(10.f);
			//big hp-bar
			ImGui::SetWindowFontScale(0.2f);
			ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - 10));
			//ImGui::ProgressBar((float)m_Map.player.Health / m_Map.player.StartHealth, ImVec2(ImGui::GetWindowSize().x, 10), std::format("HP: {}", m_Map.player.Health).c_str());

			ImGui::SetWindowFontScale(1.f);
		}

		void MAIN_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f));

			ImGui::Begin("MENU", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			UI::SetScrollSpeed(50.0f);

			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::Image(MenuBackgroundTexture, ImVec2((float)Globals.window.GetSize().x, (float)MenuBackgroundTexture.GetSize().y * TextureRatio.x));

			ImVec2 PlaySize = ImVec2(484 * TextureRatio.x, 197 * TextureRatio.x);
			if (UI::ImageButton(PlayButtonTexture, PlaySize, ImVec2((ImGui::GetWindowSize().x - PlaySize.x) * 0.5f, (ImGui::GetWindowSize().y - PlaySize.y) * 0.5f + 305)))
			{
				m_Map.LoadFull("levels/level1.malen");
				Globals.gameState = GameState::PLAY;
			}

			ImVec2 SettingsSize = ImGui::CalcTextSize("Settings");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - SettingsSize.x) * 0.5f, (ImGui::GetWindowSize().y - SettingsSize.y) * 0.5f - 100));
			if (ImGui::Button("Settings")) {
				Globals.settings.Load();
				Globals.gameState = GameState::SETTINGS;
			}

			ImVec2 LoadoutSize = ImGui::CalcTextSize("Loadout");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - LoadoutSize.x) * 0.5f, (ImGui::GetWindowSize().y - LoadoutSize.y) * 0.5f + 100));
			if (ImGui::Button("Loadout")) {
				//Globals.settings.Load();
				Globals.gameState = GameState::LOADOUT;
			}

			ImVec2 QuitSize = ImGui::CalcTextSize("Quit");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - QuitSize.x) * 0.5f, (ImGui::GetWindowSize().y - QuitSize.y) * 0.5f));
			if (ImGui::Button("Quit")) Globals.window.Close();

			ImGui::PopStyleVar(6);
			ImGui::End();
		}

		void PAUSE_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.7f));

			ImGui::SetNextWindowBgAlpha(0.75f);
			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
			UI_Data();

			ImGui::SetWindowFontScale(1.25f);
			ImVec2 PausedSize = ImGui::CalcTextSize("! PAUSED !");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - PausedSize.x) * 0.5f, (ImGui::GetWindowSize().y - PausedSize.y) * 0.5f - 100));
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), "! PAUSED !");

			ImGui::SetWindowFontScale(1.f);
			ImVec2 ResumeSize = ImGui::CalcTextSize("Resume");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ResumeSize.x) * 0.5f, (ImGui::GetWindowSize().y - ResumeSize.y) * 0.5f + 100));
			if (ImGui::Button("Resume"))Globals.gameState = GameState::PLAY;
			ImVec2 SettingsSize = ImGui::CalcTextSize("Settings");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - SettingsSize.x) * 0.5f, (ImGui::GetWindowSize().y - SettingsSize.y) * 0.5f + 200));
			if (ImGui::Button("Settings")) {
				Globals.settings.Load();
				Globals.gameState = GameState::SETTINGS;
			}


			ImVec2 MainMenuSize = ImGui::CalcTextSize("Main Menu");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - MainMenuSize.x) * 0.5f, (ImGui::GetWindowSize().y - MainMenuSize.y) * 0.5f + 300));
			if (ImGui::Button("Main Menu"))Globals.gameState = GameState::MENU;

			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetRenderImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
			ImGui::End();
			ImGui::PopStyleVar(3);
		}

		void LOADOUT_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.f);

			ImGui::Begin("LOADOUT", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(0.8f);
			if (ImGui::BeginTabBar("Tab", ImGuiTabBarFlags_None))
			{
				//@TODO - optimize this
				// ? put primaries in one class, secondaries in another and melee in another
				// - make the loops shorter -

				if (ImGui::BeginTabItem("Primary"))
				{
					ImGui::SeparatorText("Primary Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Primary)UI::WeaponChoose((WeaponType)i, m_Map.player.PrimaryWeapon);
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Secondary"))
				{
					ImGui::SeparatorText("Secondary Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Secondary) {
							UI::WeaponChoose((WeaponType)i, m_Map.player.SecondaryWeapon);
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Melee"))
				{
					ImGui::SeparatorText("Melee Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Melee) {
							UI::WeaponChoose((WeaponType)i, m_Map.player.MeleeWeapon);
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImVec2 BackSize = ImGui::CalcTextSize("Save and Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f + 300, (ImGui::GetWindowSize().y + BackSize.y + 400) * 0.5f));
			if (ImGui::Button("Save and Go Back"))
			{
				Globals.gameState = GameState::MENU;
				Globals.settings.Save();
			}
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		bool buttonChecks[10] = { false };
		void SETTINGS_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.f);

			ImGui::Begin("SETTINGS", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(0.8f);
			if (ImGui::BeginTabBar("Tab", ImGuiTabBarFlags_None)) 
			{
				if (ImGui::BeginTabItem("Screen")) 
				{
					ImGui::SeparatorText("Window");
					const char* windowSizes[] = { "1920x1080", "1366x768", "1280x1024", "1024x768", "1280x720" };
					ImGui::Combo("Window Size", &Globals.settings.iWindowSize, windowSizes, IM_ARRAYSIZE(windowSizes));
					UI::Checkbox("Fullscreen", Globals.settings.Fullscreen); UI::HelpMarker("Requires Restart");
					UI::Checkbox("VSync", Globals.settings.VSync); UI::HelpMarker("Requires Restart");
					ImGui::SliderFloat("Brightness", &Globals.settings.Brighness, 1, 10);
					UI::Separator("Game");
					UI::Checkbox("Hide Particles", Globals.settings.HideParticles);
					UI::Checkbox("Screen Effects", Globals.settings.ScreenEffects);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Audio")) 
				{
					UI::Separator("Volume");					
					if (UI::Drag("Master Volume", Globals.settings.MasterVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) 
					{ 
						ma_engine_set_volume(&Globals.music_engine, Globals.settings.MasterVolume); 
						ma_engine_set_volume(&Globals.music_engine, (Globals.settings.MusicVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f));
						ma_engine_set_volume(&Globals.sfx_engine, (Globals.settings.SFXVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f));
					}
					if (UI::Drag("Music Volume", Globals.settings.MusicVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) { ma_engine_set_volume(&Globals.music_engine, (Globals.settings.MusicVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f)); }
					if (UI::Drag("SFX Volume", Globals.settings.SFXVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) { ma_engine_set_volume(&Globals.sfx_engine, (Globals.settings.SFXVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f)); }

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Controls")) 
				{
					UI::Separator("Movement");
					UI::GetKey("Move Left", Globals.settings.KeyLeft, buttonChecks[0]);
					UI::GetKey("Move Right", Globals.settings.	KeyRight, buttonChecks[1]);
					UI::GetKey("Jump", Globals.settings.KeyJump, buttonChecks[2]);
					UI::GetKey("Dash", Globals.settings.KeyDash, buttonChecks[3]);
					UI::Separator("Weapons");
					UI::GetKey("Shoot", Globals.settings.KeyShoot, buttonChecks[4]);
					UI::GetKey("Melee", Globals.settings.KeyMelee, buttonChecks[5]);
					UI::GetKey("Main Weapon", Globals.settings.KeyMainWeapon, buttonChecks[6]);
					UI::GetKey("Secondary Weapon", Globals.settings.KeySecondaryWeapon, buttonChecks[7]);
					UI::GetKey("Fast Switch", Globals.settings.KeyFastSwich, buttonChecks[8]);
					UI::Separator("Game");

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImVec2 BackSize = ImGui::CalcTextSize("Save and Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f + 300, (ImGui::GetWindowSize().y + BackSize.y + 400) * 0.5f));
			if (ImGui::Button("Save and Go Back")) 
			{
				Globals.gameState = GameState::MENU;
				Globals.settings.Save();
			}
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		void DEATH_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::Begin("DEATH", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(1.8f);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("You Died").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("You Died").y) * 0.5f));
			ImGui::TextColored(ImVec4(1.f, 0, 0, 1.f), "You Died");
			ImGui::SetWindowFontScale(1.f);

			ImVec2 BackSize = ImGui::CalcTextSize("Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f, (ImGui::GetWindowSize().y + BackSize.y + 300) * 0.5f));
			if (ImGui::Button("Go Back")) Globals.gameState = GameState::MENU;
			
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		void WIN_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);

			ImGui::Begin("WIN", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

			ImVec2 WinSize = ImGui::CalcTextSize("CONGRATS You Won!");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - WinSize.x) * 0.5f, (ImGui::GetWindowSize().y - WinSize.y) * 0.5f));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), "CONGRATS! You Won!");

			ImVec2 TimeSize = ImGui::CalcTextSize("Level Time: {} sec.");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - TimeSize.x) * 0.5f, (ImGui::GetWindowSize().y - TimeSize.y) * 0.5f - 100));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), std::format("Level Time: {:.2f} sec.", m_Map.LevelTime).c_str());

			ImVec2 LevelSize = ImGui::CalcTextSize("Current Level: {}");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - TimeSize.x) * 0.5f, (ImGui::GetWindowSize().y - TimeSize.y) * 0.5f - 200));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), std::format("Current Level: {}", m_LevelID).c_str());

			ImVec2 NextSize = ImGui::CalcTextSize("Go Next");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - NextSize.x) * 0.5f, (ImGui::GetWindowSize().y + NextSize.y + 300) * 0.5f));
			if (ImGui::Button("Go Next")) 
			{
				Globals.gameState = GameState::PLAY;
				m_Map.EnemyCount = 0;
				m_Map.LoadFull("levels/level2.malen");
				m_Map.player.Health = m_Map.player.StartHealth;
				m_LevelID++;
			}
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		void UI()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			UI_Data();

			//glm::vec2 pos = m_Renderer.WorldToScreen(m_Map.player.Position - (glm::vec2)m_Map.camera.Position - m_Map.player.Size.x);
			//ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
			//ImGui::ProgressBar(m_Map.player.DashCD / 2, ImVec2(65, 10), "");

			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetRenderImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
			ImGui::End();
			ImGui::PopStyleVar(3);
		}

		void Resize(glm::vec2 size)
		{
			m_Renderer.Resize(size, m_RenderData);
		}

		void DestroyGame()
		{
			m_Renderer.Deinit();
			m_RenderData.Destroy();
			m_Map.Free();
		}
	};
}