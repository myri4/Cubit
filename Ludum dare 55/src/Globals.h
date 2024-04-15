#pragma once

#include <glm/glm.hpp>
#include <wc/Audio/AudioEngine.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/Time.h>

namespace wc 
{
	enum class GameState
	{
		MENU,
		PLAY,
		PAUSE
	};

	enum class Level
	{
		LEVEL1,
		LEVEL2,
		LEVEL3
	};
	struct GlobalVariables
	{
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