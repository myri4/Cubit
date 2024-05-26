#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <wc/Utils/CPUImage.h>
#include "../Rendering/RenderData.h"
#include "../Globals.h"

namespace wc
{
	struct Tile
	{
		bool Solid = true;
	};

	using TileID = uint8_t;

	struct Tileset
	{
		std::vector<Tile> Tiles;

		void Load()
		{
			Tiles.resize(5);

			Tiles[0] = Tile();
			Tiles[0].Solid = false;
			
			Tiles[1] = Tile();
			Tiles[1].Solid = true;
		}
	};
}