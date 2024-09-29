#pragma once
#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <spdlog/fmt/bundled/format.h>

// Specialize the fmt::formatter template for glm::vec2
template <>
struct fmt::formatter<glm::vec2> {
	// Parses format specifications of the form 'x: {}  y: {}'
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
		return ctx.end();
	}

	// Formats the glm::vec2 instance
	template <typename FormatContext>
	auto format(const glm::vec2& vec, FormatContext& ctx) -> decltype(ctx.out()) {
		return fmt::format_to(ctx.out(), "x:{:.2f}  y:{:.2f}", vec.x, vec.y);
	}
};
