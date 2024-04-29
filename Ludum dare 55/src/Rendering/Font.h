#pragma once

#undef INFINITE
#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>
#include <msdfgen/atlas-gen/msdf-atlas-gen.h>
#include <msdfgen/atlas-gen/GlyphGeometry.h>
#define INFINITE            0xFFFFFFFF

#include <vector>
#include <glm/glm.hpp>
namespace wc
{
    struct RenderData;

	struct Font
	{
        void Load(const std::string filepath, RenderData& renderData);
        glm::vec2 CalculateTextSize(const std::string& text);

        float Kerning = 0.f;
        float LineSpacing = 0.f;
        uint32_t textureID = 0;
        msdf_atlas::FontGeometry FontGeometry;
    private:
        std::vector<msdf_atlas::GlyphGeometry> m_Glyphs;
	};

    struct SvgImage
    {
        void Load(const std::string& filepath, RenderData& renderData);

        uint32_t textureID = 0;
        msdfgen::Shape shape;
    };
}