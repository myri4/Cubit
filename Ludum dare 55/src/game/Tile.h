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
		uint32_t StartFrame = 0;
		uint32_t TextureID = 0;

		bool Solid = true;
	};

	using TileID = uint8_t;

	struct Tileset
	{
		uint32_t StartIndex = 0;
		uint32_t TextureCount = 0;
		std::vector<Tile> Tiles;

		void Create(const std::string& filepath, RenderData& renderData)
		{
			CPUImage image;
			image.Load(filepath, 4);

			const uint32_t TEX_WIDTH = 32;
			const uint32_t TEX_HEIGHT = 32;

			CPUImage output;
			output.Allocate(TEX_WIDTH, TEX_HEIGHT, 4); // Here 4 components are forced from Vulkan

			uint32_t numTilesX = image.Width / TEX_WIDTH;
			uint32_t numTilesY = image.Height / TEX_HEIGHT;
			TextureCount = numTilesX * numTilesY;

			for (uint32_t tileY = 0; tileY < numTilesY; tileY++)
				for (uint32_t tileX = 0; tileX < numTilesX; tileX++)
				{
					for (uint32_t x = 0; x < TEX_WIDTH; x++)
						for (uint32_t y = 0; y < TEX_HEIGHT; y++)
						{
							glm::vec4 color = image.Color(x + tileX * TEX_WIDTH, y + tileY * TEX_HEIGHT);
							output.Set(x, y, color);
						}
					uint32_t id = renderData.LoadTextureFromMemory(output);
					if (StartIndex == 0) StartIndex = id;
				}

			output.Free();
			image.Free();

			Tiles.resize(101);

			Tiles[0] = Tile();
			Tiles[0].Solid = false;
		}

		void Free()
		{
			Tiles.clear();
		}

		void Load(const std::string& filepath, RenderData& RenderData)
		{
			if (std::filesystem::exists(filepath))
			{
				YAML::Node node = YAML::LoadFile(filepath);

				Create(node["SourceImage"].as<std::string>(), RenderData);

				YAML::Node tiles = node["Tiles"];
				for (int i = 0; i < tiles.size(); i++)
				{
					auto metaData = tiles[i];

					uint32_t id = 0;
					if (metaData["ID"]) id = metaData["ID"].as<TileID>();

					Tile& tile = Tiles[id];

					if (metaData["StartFrame"]) tile.StartFrame = metaData["StartFrame"].as<uint32_t>();

					if (metaData["Solid"]) tile.Solid = metaData["Solid"].as<bool>();

					tile.TextureID = StartIndex + tile.StartFrame;
				}
			}
		}
	};
}