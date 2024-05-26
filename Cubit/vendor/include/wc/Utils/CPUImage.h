#pragma once

#include <glm/glm.hpp>
#include <string>

#include <stb_image/stb_image.h>
#include <stb_image/stb_write.h>

namespace wc
{
	struct CPUImage
	{
		uint8_t* data = nullptr;
		uint32_t Width = 0, Height = 0, Channels = 0, bytes_per_scanline = 0;

		void Allocate(uint32_t width, uint32_t height, uint32_t channels)
		{
			Width = width;
			Height = height;
			Channels = channels;
			bytes_per_scanline = Width * channels;
			data = new uint8_t[Width * Height * Channels];
		}

		void Load(const std::string& file, int reqComp = 0)
		{
			int width, height, channels;
			data = stbi_load(file.c_str(), &width, &height, &channels, reqComp);
			Width = width;
			Height = height;
			Channels = reqComp != 0 ? reqComp : channels;
			bytes_per_scanline = Width * Channels;
		}

		void Save(const std::string& file)
		{
			stbi_write_png(file.c_str(), Width, Height, Channels, data, bytes_per_scanline);
		}

		glm::vec4 Color(uint32_t x, uint32_t y)
		{
			glm::vec4 color;
			color.r = data[y * bytes_per_scanline + x * Channels + 0];
			color.g = data[y * bytes_per_scanline + x * Channels + 1];
			color.b = data[y * bytes_per_scanline + x * Channels + 2];
			color.a = data[y * bytes_per_scanline + x * Channels + 3];
			return color;
		}

		void Set(uint32_t x, uint32_t y, const glm::vec4& color)
		{
			data[y * bytes_per_scanline + x * Channels + 0] = (uint8_t)color.r;
			data[y * bytes_per_scanline + x * Channels + 1] = (uint8_t)color.g;
			data[y * bytes_per_scanline + x * Channels + 2] = (uint8_t)color.b;
			data[y * bytes_per_scanline + x * Channels + 3] = (uint8_t)color.a;
		}

		void Free() { free(data); }
	};
}