#pragma once

#include <glm/glm.hpp>
#include <wc/Audio/AudioEngine.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>
#include "miniaudio.h"

namespace wc 
{
	enum class GameState
	{
		MENU,
		DEATH,
		WIN,
		PLAY,
		SETTINGS,
		PAUSE
	};

	struct GlobalVariables
	{
		//sound

		const int music_fade_time_mls = 500;

		float sfx_volume = 0.6f;

		ma_sound music_menu;
		ma_sound music_main;
		ma_sound music_GameOver;
		ma_sound music_win;

		ma_engine sfx_engine;
		ma_engine music_engine;

		Window window; // @TODO: maybe rename to main window

		Clock deltaTimer;
		float deltaTime = 0.f;

		float levelTime = 0.f;
		int level = 1;

		GameState gameState = GameState::MENU;

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}
	};

	inline GlobalVariables Globals;
}