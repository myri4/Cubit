#pragma once

#include <glm/glm.hpp>
#include <filesystem>

// Gui
#include <imgui/imgui.h>
#include <wc/Utils/YAML.h>

namespace wc
{
	using KeyType = int32_t; // @TODO: Fix the entire input system and change this to uint

	enum class GraphicsOption : uint8_t { OFF, PERF, QUALITY };

	struct Settings
	{
		// Screen
		int iWindowSize = 0;
		glm::vec2 WindowSize = { 1920, 1080 };
		float WindowResolution = 0;
		bool Fullscreen = true;
		bool VSync = false;

		// Graphics
		GraphicsOption ChromatiAbberation = GraphicsOption::PERF;
		bool CRTEffect = true;
		bool Bloom = true;
		bool Vignete = true;
		float Brighness = 1.f;
		bool Background = false;

		// Audio
		int MasterVolume = 50;
		int MusicVolume = 50;
		int SFXVolume = 50;

		// Controls
		KeyType KeyLeft = ImGuiKey_A;
		KeyType KeyRight = ImGuiKey_D;
		KeyType KeyJump = ImGuiKey_Space;
		KeyType KeyDash = ImGuiKey_LeftShift;
		KeyType KeyMelee = ImGuiKey_E;
		KeyType KeyShoot = ImGuiKey_MouseLeft;
		KeyType KeyMainWeapon = ImGuiKey_1;
		KeyType KeySecondaryWeapon = ImGuiKey_2;
		KeyType KeyFastSwich = ImGuiKey_Q; // @TODO: Make it so it changes to the last weapon used

		void Save()
		{
			YAML::Node settings;

			// Window
			YAML_SAVE_VAR(settings, iWindowSize);
			YAML_SAVE_VAR(settings, WindowSize);
			YAML_SAVE_VAR(settings, WindowResolution);
			YAML_SAVE_VAR(settings, Fullscreen);
			YAML_SAVE_VAR(settings, VSync);

			// Screen effects
			//YAML_SAVE_VAR(settings, HideParticles);
			YAML_SAVE_VAR(settings, CRTEffect);
			YAML_SAVE_VAR(settings, Bloom);
			YAML_SAVE_VAR(settings, Vignete);
			YAML_SAVE_VAR(settings, Brighness);
			YAML_SAVE_VAR(settings, Background);

			YAML_SAVE_VAR(settings, MasterVolume);
			YAML_SAVE_VAR(settings, MusicVolume);
			YAML_SAVE_VAR(settings, SFXVolume);

			// Key binds
			YAML_SAVE_VAR(settings, KeyLeft);
			YAML_SAVE_VAR(settings, KeyRight);
			YAML_SAVE_VAR(settings, KeyJump);
			YAML_SAVE_VAR(settings, KeyDash);
			YAML_SAVE_VAR(settings, KeyMelee);
			YAML_SAVE_VAR(settings, KeyShoot);
			YAML_SAVE_VAR(settings, KeyMainWeapon);
			YAML_SAVE_VAR(settings, KeySecondaryWeapon);
			YAML_SAVE_VAR(settings, KeyFastSwich);

			YAMLUtils::SaveFile("settings.txt", settings);
		}

		void Load()
		{
			if (!std::filesystem::exists("settings.txt")) 
			{ 
				Save();
				return;
			}

			YAML::Node settings = YAML::LoadFile("settings.txt");

			// Window
			YAML_LOAD_VAR(settings, iWindowSize);
			YAML_LOAD_VAR(settings, WindowSize);
			YAML_LOAD_VAR(settings, WindowResolution);
			YAML_LOAD_VAR(settings, Fullscreen);
			YAML_LOAD_VAR(settings, VSync);

			//YAML_LOAD_VAR(settings, HideParticles);
			YAML_LOAD_VAR(settings, CRTEffect);
			YAML_LOAD_VAR(settings, Bloom);
			YAML_LOAD_VAR(settings, Vignete);
			YAML_LOAD_VAR(settings, Brighness);
			YAML_LOAD_VAR(settings, Background);

			YAML_LOAD_VAR(settings, MasterVolume);
			YAML_LOAD_VAR(settings, MusicVolume);
			YAML_LOAD_VAR(settings, SFXVolume);

			// Key binds
			YAML_LOAD_VAR(settings, KeyLeft);
			YAML_LOAD_VAR(settings, KeyRight);
			YAML_LOAD_VAR(settings, KeyJump);
			YAML_LOAD_VAR(settings, KeyDash);
			YAML_LOAD_VAR(settings, KeyMelee);
			YAML_LOAD_VAR(settings, KeyShoot);
			YAML_LOAD_VAR(settings, KeyMainWeapon);
			YAML_LOAD_VAR(settings, KeySecondaryWeapon);
			YAML_LOAD_VAR(settings, KeyFastSwich);

			if (iWindowSize == 0)
				WindowSize = { 1920, 1080 };
			else if (iWindowSize == 1)
				WindowSize = { 1366, 768 };
			else if (iWindowSize == 2)
				WindowSize = { 1280, 1024 };
			else if (iWindowSize == 3)
				WindowSize = { 1024, 768 };
			else if (iWindowSize == 4)
				WindowSize = { 1280, 720 };
		}
	};
}