#pragma once

#include <wc/vk/Buffer.h>
#include <wc/vk/Commands.h>
#include <wc/vk/Images.h>
#include <wc/Texture.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <wc/Utils/CPUImage.h>
#include "Font.h"

#undef LoadImage
namespace wc
{
	static const uint32_t MaxQuadCount = 10'000;
	static const uint32_t MaxQuadVertexCount = MaxQuadCount * 4;
	static const uint32_t MaxQuadIndexCount = MaxQuadCount * 6;

	static const uint32_t MaxLineCount = 20'000;
	static const uint32_t MaxLineVertexCount = MaxLineCount * 2;

	struct Vertex
	{
		glm::vec3 Position;
		uint32_t TextureID = 0;
		glm::vec2 TexCoords;
		float Fade = 0.f;
		float Thickness = 0.f;

		glm::vec4 Color;
		Vertex() = default;
		Vertex(const glm::vec3& pos, glm::vec2 texCoords, uint32_t texID, const glm::vec4& color) : Position(pos), TexCoords(texCoords), TextureID(texID), Color(color) {}
		Vertex(const glm::vec3& pos, glm::vec2 texCoords, float thickness, float fade, const glm::vec4& color) : Position(pos), TexCoords(texCoords), Thickness(thickness), Fade(fade), Color(color) {}
	};

	struct LineVertex
	{
		glm::vec3 Position;
		uint32_t _pad1 = 0;
		glm::vec4 Color;

		LineVertex() = default;
		LineVertex(const glm::vec3& pos, const glm::vec4& color) : Position(pos), Color(color) {}
	};

	class RenderData
	{
		BufferManager<Vertex> m_VertexBuffer;
		BufferManager<uint32_t> m_IndexBuffer;
		BufferManager<LineVertex> m_LineVertexBuffer;

		std::unordered_map<std::string, uint32_t> m_Cache;
	public:
		std::vector<Texture> Textures;

	public:
		auto GetVertexBuffer() const { return m_VertexBuffer.GetBuffer(); }
		auto GetIndexBuffer() const { return m_IndexBuffer.GetBuffer(); }

		auto GetLineVertexBuffer() const { return m_LineVertexBuffer.GetBuffer(); }

		auto GetIndexCount() const { return m_IndexBuffer.Counter; }
		auto GetVertexCount() const { return m_VertexBuffer.Counter; }

		auto GetLineVertexCount() const { return m_LineVertexBuffer.Counter; }

		void UploadVertexData()
		{
			SyncContext::immediate_submit([=](VkCommandBuffer cmd) {
				m_IndexBuffer.Update(cmd);
				m_VertexBuffer.Update(cmd);
				});
		}

		void UploadLineVertexData()
		{
			SyncContext::immediate_submit([=](VkCommandBuffer cmd) {
				m_LineVertexBuffer.Update(cmd);
				});
		}

		void Create()
		{
			m_IndexBuffer.Allocate(MaxQuadIndexCount, INDEX_BUFFER);
			m_VertexBuffer.Allocate(MaxQuadVertexCount);

			m_LineVertexBuffer.Allocate(MaxLineVertexCount);

			m_LineVertexBuffer.GetBuffer().SetName("LineVertexBuffer");

			Texture texture;
			uint32_t white = 0xFFFFFFFF;
			texture.Load(&white, 1, 1);
			Textures.push_back(texture);
		}

		uint32_t LoadTexture(const std::string& file)
		{
			if (m_Cache.find(file) != m_Cache.end()) return m_Cache[file];  // If this texture exists
			if (std::filesystem::exists(file))
			{
				Texture texture;
				texture.Load(file);
				Textures.push_back(texture);

				m_Cache[file] = uint32_t(Textures.size() - 1);
				return uint32_t(Textures.size() - 1);
			}

			m_Cache[file] = 0;
			WC_CORE_ERROR("Cannot find file at location: {}", file);
			return 0;
		}

		Texture LoadImage(const std::string& file) { return Textures[LoadTexture(file)]; }

		uint32_t LoadTextureFromMemory(const CPUImage& image)
		{
			Texture texture;
			texture.Load(image.data, image.Width, image.Height);
			Textures.push_back(texture);

			return uint32_t(Textures.size() - 1);
		}

		// @TODO: Remake this api to support meshes not predefined shapes.

		void DrawQuad(const glm::mat4& transform, uint32_t texID, const glm::vec4& color = glm::vec4(1.f))
		{
			if (m_IndexBuffer.Counter >= MaxQuadIndexCount) WC_CORE_ERROR("Not enough memory");// Flush();

			Vertex* vertices = m_VertexBuffer;
			auto& vertCount = m_VertexBuffer.Counter;

			uint32_t* indices = m_IndexBuffer;
			auto& indexCount = m_IndexBuffer.Counter;

			vertices[vertCount + 0] = Vertex(transform * glm::vec4(0.5f, 0.5f, 0.f, 1.f), { 1.f, 0.f }, texID, color);
			vertices[vertCount + 1] = Vertex(transform * glm::vec4(-0.5f, 0.5f, 0.f, 1.f), { 0.f, 0.f }, texID, color);
			vertices[vertCount + 2] = Vertex(transform * glm::vec4(-0.5f, -0.5f, 0.f, 1.f), { 0.f, 1.f }, texID, color);
			vertices[vertCount + 3] = Vertex(transform * glm::vec4(0.5f, -0.5f, 0.f, 1.f), { 1.f, 1.f }, texID, color);

			indices[indexCount + 0] = vertCount;
			indices[indexCount + 1] = 1 + vertCount;
			indices[indexCount + 2] = 2 + vertCount;

			indices[indexCount + 3] = 2 + vertCount;
			indices[indexCount + 4] = 3 + vertCount;
			indices[indexCount + 5] = vertCount;

			vertCount += 4;
			indexCount += 6;
		}

		void DrawLineQuad(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::vec3 vertices[4];
			vertices[0] = transform * glm::vec4(0.5f, 0.5f, 0.f, 1.f);
			vertices[1] = transform * glm::vec4(-0.5f, 0.5f, 0.f, 1.f);
			vertices[2] = transform * glm::vec4(-0.5f, -0.5f, 0.f, 1.f);
			vertices[3] = transform * glm::vec4(0.5f, -0.5f, 0.f, 1.f);
			DrawLine(vertices[0], vertices[1], color);
			DrawLine(vertices[1], vertices[2], color);
			DrawLine(vertices[2], vertices[3], color);
			DrawLine(vertices[3], vertices[0], color);
		}

		void DrawQuad(const glm::vec3& position, glm::vec2 size, uint32_t texID = 0, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::scale(glm::mat4(1.f), { size.x, size.y, 1.f });
			DrawQuad(transform, texID, color);
		}

		// Note: Rotation should be in radians
		void DrawQuad(const glm::vec3& position, glm::vec2 size, float rotation, uint32_t texID = 0, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { size.x, size.y, 1.f });
			DrawQuad(transform, texID, color);
		}

		void DrawTriangle(const glm::mat4& transform, uint32_t texID, const glm::vec4& color = glm::vec4(1.f))
		{
			if (m_IndexBuffer.Counter >= MaxQuadIndexCount) WC_CORE_ERROR("Not enough memory");// Flush();

			Vertex* vertices = m_VertexBuffer;
			auto& vertCount = m_VertexBuffer.Counter;

			uint32_t* indices = m_IndexBuffer;
			auto& indexCount = m_IndexBuffer.Counter;

			vertices[vertCount + 0] = Vertex(transform * glm::vec4(-0.5f, -0.5f, 0.f, 1.f), { 1.f, 0.f }, texID, color);
			vertices[vertCount + 1] = Vertex(transform * glm::vec4(0.5f, -0.5f, 0.f, 1.f), { 0.f, 0.f }, texID, color);
			vertices[vertCount + 2] = Vertex(transform * glm::vec4(0.f, 0.5f, 0.f, 1.f), { 0.f, 1.f }, texID, color);

			indices[indexCount + 0] = vertCount;
			indices[indexCount + 1] = 1 + vertCount;
			indices[indexCount + 2] = 2 + vertCount;

			vertCount += 3;
			indexCount += 3;
		}

		void DrawTriangle(const glm::vec3& position, glm::vec2 size, float rotation, uint32_t texID = 0, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { size.x, size.y, 1.f });
			DrawTriangle(transform, texID, color);
		}

		void DrawCircle(const glm::vec3& position, float radius, float thickness = 1.f, float fade = 0.05f, const glm::vec4& color = glm::vec4(1.f))
		{
			Vertex* vertices = m_VertexBuffer;
			auto& vertCount = m_VertexBuffer.Counter;

			uint32_t* indices = m_IndexBuffer;
			auto& indexCount = m_IndexBuffer.Counter;

			vertices[vertCount + 0] = Vertex(position + glm::vec3(radius, radius, 0.f), { 1.f, 1.f }, thickness, fade, color);
			vertices[vertCount + 1] = Vertex(position + glm::vec3(-radius, radius, 0.f), { -1.f, 1.f }, thickness, fade, color);
			vertices[vertCount + 2] = Vertex(position + glm::vec3(-radius, -radius, 0.f), { -1.f,-1.f }, thickness, fade, color);
			vertices[vertCount + 3] = Vertex(position + glm::vec3(radius, -radius, 0.f), { 1.f,-1.f }, thickness, fade, color);

			indices[indexCount + 0] = vertCount;
			indices[indexCount + 1] = 1 + vertCount;
			indices[indexCount + 2] = 2 + vertCount;

			indices[indexCount + 3] = 2 + vertCount;
			indices[indexCount + 4] = 3 + vertCount;
			indices[indexCount + 5] = vertCount;

			vertCount += 4;
			indexCount += 6;
		}


		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& startColor, const glm::vec4& endColor)
		{
			//if (m_LineVertexCount >= MaxLineVertexCount) Flush();

			m_LineVertexBuffer[m_LineVertexBuffer.Counter + 0] = LineVertex(start, startColor);
			m_LineVertexBuffer[m_LineVertexBuffer.Counter + 1] = LineVertex(end, endColor);

			m_LineVertexBuffer.Counter += 2;
		}

		void DrawLines(const LineVertex* vertices, uint32_t count)
		{
			memcpy(m_LineVertexBuffer + m_LineVertexBuffer.Counter * sizeof(LineVertex), vertices, count * sizeof(LineVertex));
			m_LineVertexBuffer.Counter += count;
		}

		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& startColor, const glm::vec3& endColor) { DrawLine(start, end, glm::vec4(startColor, 1.f), glm::vec4(endColor, 1.f)); }
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color = glm::vec4(1.f)) { DrawLine(start, end, color, color); }
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) { DrawLine(start, end, color, color); }


		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec4& color = glm::vec4(1.f)) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), color, color); }
		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec3& startColor, const glm::vec3& endColor) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), glm::vec4(startColor, 1.f), glm::vec4(endColor, 1.f)); }
		void DrawLine(glm::vec2 start, glm::vec2 end, const glm::vec3& color) { DrawLine(glm::vec3(start, 0.f), glm::vec3(end, 0.f), color, color); }

		void DrawString(const std::string& string, const Font& font, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.f))
		{
			const auto& fontGeometry = font.FontGeometry;
			const auto& metrics = fontGeometry.getMetrics();
			uint32_t texID = font.textureID;
			auto& fontAtlas = Textures[texID];

			double x = 0.0;
			double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
			double y = 0.0;

			Vertex* vertices = m_VertexBuffer;
			auto& vertCount = m_VertexBuffer.Counter;

			uint32_t* indices = m_IndexBuffer;
			auto& indexCount = m_IndexBuffer.Counter;
						
			const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

			for (uint32_t i = 0; i < string.size(); i++)
			{
				char character = string[i];

				if (character == '\r')
					continue;

				if (character == '\n')
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + font.LineSpacing;
					continue;
				}

				if (character == ' ')
				{
					float advance = spaceGlyphAdvance;
					if (i < string.size() - 1)
					{
						char nextCharacter = string[i + 1];
						double dAdvance;
						fontGeometry.getAdvance(dAdvance, character, nextCharacter);
						advance = (float)dAdvance;
					}

					x += fsScale * advance + font.Kerning;
					continue;
				}

				if (character == '\t')
				{
					x += 4.f * (fsScale * spaceGlyphAdvance + font.Kerning);
					continue;
				}

				auto glyph = fontGeometry.getGlyph(character);

				if (!glyph)
					glyph = fontGeometry.getGlyph('?');

				if (!glyph) return;

				double al, ab, ar, at;
				glyph->getQuadAtlasBounds(al, ab, ar, at);
				glm::vec2 texCoordMin((float)al, (float)ab);
				glm::vec2 texCoordMax((float)ar, (float)at);

				double pl, pb, pr, pt;
				glyph->getQuadPlaneBounds(pl, pb, pr, pt);
				glm::vec2 quadMin((float)pl, (float)pb);
				glm::vec2 quadMax((float)pr, (float)pt);

				quadMin *= fsScale, quadMax *= fsScale;
				quadMin += glm::vec2(x, y);
				quadMax += glm::vec2(x, y);

				float texelWidth = 1.f / fontAtlas.GetImage().GetSize().x;
				float texelHeight = 1.f / fontAtlas.GetImage().GetSize().y;
				texCoordMin *= glm::vec2(texelWidth, texelHeight);
				texCoordMax *= glm::vec2(texelWidth, texelHeight);

				vertices[vertCount + 0] = Vertex(transform * glm::vec4(quadMax, 0.f, 1.f), texCoordMax, texID, color);
				vertices[vertCount + 1] = Vertex(transform * glm::vec4(quadMin.x, quadMax.y, 0.f, 1.f), { texCoordMin.x, texCoordMax.y }, texID, color);
				vertices[vertCount + 2] = Vertex(transform * glm::vec4(quadMin, 0.f, 1.f), texCoordMin, texID, color);
				vertices[vertCount + 3] = Vertex(transform * glm::vec4(quadMax.x, quadMin.y, 0.f, 1.f), { texCoordMax.x, texCoordMin.y }, texID, color);

				for (uint32_t i = 0; i < 4; i++) vertices[vertCount + i].Thickness = -1.f;

				indices[indexCount + 0] = vertCount;
				indices[indexCount + 1] = 1 + vertCount;
				indices[indexCount + 2] = 2 + vertCount;

				indices[indexCount + 3] = 2 + vertCount;
				indices[indexCount + 4] = 3 + vertCount;
				indices[indexCount + 5] = vertCount;

				vertCount += 4;
				indexCount += 6;

				if (i < string.size() - 1)
				{
					double advance = glyph->getAdvance();
					char nextCharacter = string[i + 1];
					fontGeometry.getAdvance(advance, character, nextCharacter);

					x += fsScale * advance + font.Kerning;
				}
			}
		}

		void DrawString(const std::string& string, const Font& font, const glm::vec3& position, glm::vec2 scale, float rotation, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position) * glm::rotate(glm::mat4(1.f), rotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f), { scale.x, scale.y, 1.f });
			DrawString(string, font, transform, color);
		}

		void DrawString(const std::string& string, const Font& font, const glm::vec3& position, const glm::vec4& color = glm::vec4(1.f))
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), position);
			DrawString(string, font, transform, color);
		}

		void Reset()
		{
			m_IndexBuffer.Counter = 0;
			m_VertexBuffer.Counter = 0;

			m_LineVertexBuffer.Counter = 0;
		}

		void Destroy()
		{
			m_VertexBuffer.Free();
			m_IndexBuffer.Free();
			m_LineVertexBuffer.Free();

			for (auto& texture : Textures) texture.Destroy();
		}
	};
}