#pragma once

#include <glm/glm.hpp>
#include <wc/Audio/AudioEngine.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>
//#include <wc/Utils/spdLog.h>
#include "miniaudio.h"
#include "game/Events.h"

#include "Settings.h"
#include <iostream>

namespace wc 
{
	
	enum class GameState
	{
		MENU,
		DEATH,
		WIN,
		PLAY,
		CREDITS,
		SETTINGS,
		LOADOUT,
		PAUSE
	};

	struct GlobalVariables
	{
		Settings settings;

		std::string levelName = "";

		std::vector<ServerEvent> ServerEvents;

		//sound
		const int music_fade_time_mls = 500;

		float sfx_volume = 0.6f;

		ma_sound music_menu;
		ma_sound music_main;
		ma_sound music_GameOver;
		ma_sound music_win;

		std::string shotgun = "assets/sound/sfx/shotgun.wav";
		std::string gun = "assets/sound/sfx/gun.wav";
		std::string swordSwing = "assets/sound/sfx/sword_swing.wav";
		std::string damageEnemy = "assets/sound/sfx/damage_enemy.wav";

		ma_engine sfx_engine;
		ma_engine music_engine;

		Window window; // @TODO: maybe rename to main window

		Clock deltaTimer;
		float deltaTime = 0.f;

		float serverTime = 0.f;

		GameState gameState = GameState::MENU;

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}
	};

	inline GlobalVariables Globals;
}