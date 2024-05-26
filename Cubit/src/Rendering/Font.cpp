#include "Font.h"
#include "RenderData.h"

namespace wc
{
	void Font::Load(const std::string filepath, RenderData& renderData)
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();

        if (!ft) return; // @TODO: Handle errors

        msdfgen::FontHandle* font = loadFont(ft, filepath.c_str());
        if (!font) return;

        struct CharsetRange
        {
            uint32_t Begin, End;
        };

        // From imgui_draw.cpp
        static const CharsetRange charsetRanges[] =
        {
            { 0x0020, 0x00FF }
        };

        msdf_atlas::Charset charset;
        for (CharsetRange range : charsetRanges)
        {
            for (uint32_t c = range.Begin; c <= range.End; c++)
                charset.add(c);
        }

        double fontScale = 1.f;
        FontGeometry = msdf_atlas::FontGeometry(&m_Glyphs);
        int glyphsLoaded = FontGeometry.loadCharset(font, fontScale, charset);


        double emSize = 40.0;

        msdf_atlas::TightAtlasPacker atlasPacker;
        // atlasPacker.setDimensionsConstraint()
        atlasPacker.setPixelRange(2.0);
        atlasPacker.setMiterLimit(1.0);
        atlasPacker.setPadding(0);
        atlasPacker.setScale(emSize);
        int remaining = atlasPacker.pack(m_Glyphs.data(), (int)m_Glyphs.size());


        int width, height;
        atlasPacker.getDimensions(width, height);
        emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
        // if MSDF || MTSDF

        uint64_t coloringSeed = 0;
        bool expensiveColoring = false;
        if (expensiveColoring)
        {
            msdf_atlas::Workload([&glyphs = m_Glyphs, &coloringSeed](int i, int threadNo) -> bool {
                unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
                glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
                return true;
                }, m_Glyphs.size()).finish(THREAD_COUNT);
        }
        else {
            unsigned long long glyphSeed = coloringSeed;
            for (msdf_atlas::GlyphGeometry& glyph : m_Glyphs)
            {
                glyphSeed *= LCG_MULTIPLIER;
                glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
            }
        }

        msdf_atlas::GeneratorAttributes attributes;
        attributes.config.overlapSupport = true;
        attributes.scanlinePass = true;

        msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<uint8_t, 3>> generator(width, height);
        generator.setAttributes(attributes);
        generator.setThreadCount(8);
        generator.generate(m_Glyphs.data(), (int)m_Glyphs.size());

        msdfgen::BitmapConstRef<uint8_t, 3> bitmap = (msdfgen::BitmapConstRef<uint8_t, 3>)generator.atlasStorage();
        auto bytes_per_scanline = bitmap.width * 3;
        CPUImage newBitmap;
        newBitmap.Allocate(bitmap.width, bitmap.height, 4);

        for (uint32_t x = 0; x < bitmap.width; x++)
            for (uint32_t y = 0; y < bitmap.height; y++)
            {
                glm::vec3 col;
                col.r = bitmap.pixels[y * bytes_per_scanline + x * 3 + 0];
                col.g = bitmap.pixels[y * bytes_per_scanline + x * 3 + 1];
                col.b = bitmap.pixels[y * bytes_per_scanline + x * 3 + 2];
                newBitmap.Set(x, y, glm::vec4(col, 255.f));
            }

        textureID = renderData.LoadTextureFromMemory(newBitmap);
        newBitmap.Free();

        destroyFont(font);
        deinitializeFreetype(ft);
    }

    glm::vec2 Font::CalculateTextSize(const std::string& string)
    {
		const auto& fontGeometry = FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		double x = 0.0;
		double y = 0.0;
        glm::vec2 end;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

		const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

		for (uint32_t i = 0; i < string.size(); i++)
		{
			char character = string[i];

			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0;
				y -= fsScale * metrics.lineHeight + LineSpacing;
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

				x += fsScale * advance + Kerning;
                end.x += fsScale * advance + Kerning;
				continue;
			}

			if (character == '\t')
			{
				x += 4.f * (fsScale * spaceGlyphAdvance + Kerning);
                end.x += 4.f * (fsScale * spaceGlyphAdvance + Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);

			if (!glyph)
				glyph = fontGeometry.getGlyph('?');

			if (!glyph) return glm::vec2(0.f);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin((float)pl, (float)pb);
			glm::vec2 quadMax((float)pr, (float)pt);

			quadMin *= fsScale, quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			if (i < string.size() - 1)
			{
				double advance = glyph->getAdvance();
				char nextCharacter = string[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + Kerning;
                end.x += fsScale * advance + Kerning;
			}
		}

        return {x, y};
    }

    void SvgImage::Load(const std::string& filepath, RenderData& renderData)
    {
        bool loaded = msdfgen::loadSvgShape(shape, filepath.c_str());
        if (loaded)
        {
            shape.normalize();
            //                      max. angle
            edgeColoringSimple(shape, 3.0);
            //           image width, height
            msdfgen::Bitmap<float, 3> bitmap(800, 400);
            //                     range, scale, translation
            msdfgen::generateMSDF(bitmap, shape, 4.0, 1.0, 0.0);


            auto bytes_per_scanline = bitmap.width() * 3;
            CPUImage newBitmap;
            newBitmap.Allocate(bitmap.width(), bitmap.height(), 4);

            for (uint32_t x = 0; x < bitmap.width(); x++)
                for (uint32_t y = 0; y < bitmap.height(); y++)
                {
                    glm::vec3 col;
                    col.r = bitmap[y * bytes_per_scanline + x * 3 + 0];
                    col.g = bitmap[y * bytes_per_scanline + x * 3 + 1];
                    col.b = bitmap[y * bytes_per_scanline + x * 3 + 2];
                    newBitmap.Set(x, y, glm::vec4(col, 255.f));
                }

            textureID = renderData.LoadTextureFromMemory(newBitmap);
            renderData.Textures[textureID].SetName("svg");
            newBitmap.Free();
        }
    }
}