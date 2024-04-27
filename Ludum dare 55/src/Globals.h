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
		CREDITS,
		PAUSE
	};

	struct GlobalVariables
	{
		//sound

		float sfx_volume = 0.6f;

		ma_engine sfx_engine;

		Window window; // @TODO: maybe rename to main window

		Clock deltaTimer;
		float deltaTime = 0.f;

		GameState gameState = GameState::MENU;

		void UpdateTime()
		{
			deltaTime = deltaTimer.restart();
		}
	};

	inline GlobalVariables Globals;
}