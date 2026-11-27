#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "DamageNumbers.h"
#include "MenuFont.h"

struct DamageNumberRenderResources
{
	static constexpr unsigned int cellWidth = 24;
	static constexpr unsigned int cellHeight = 36;
	static constexpr unsigned int characterSize = 24;
	sf::RenderTexture atlas;
	sf::Shader shader;
	float advance = 0.0f;
	bool ready = false;
	std::vector<sf::Vertex> vertices;

	DamageNumberRenderResources()
	{
		if (!sf::Shader::isAvailable())
		{
			std::fprintf(stderr, "Damage numbers: shaders are not supported.\n");
			return;
		}
		if (!shader.loadFromFile("Assets/Shaders/Combat/damage_numbers.vert", "Assets/Shaders/Combat/damage_numbers.frag"))
		{
			std::fprintf(stderr, "Damage numbers: failed to load shaders.\n");
			return;
		}
		sf::Font font;
		if (!LoadMenuFont(font))
		{
			std::fprintf(stderr, "Damage numbers: failed to load digit font.\n");
			return;
		}
		if (!atlas.resize({ cellWidth * 10, cellHeight }))
		{
			std::fprintf(stderr, "Damage numbers: failed to create digit atlas.\n");
			return;
		}
		atlas.clear(sf::Color::Transparent);
		for (unsigned int digit = 0; digit < 10; ++digit)
		{
			sf::Text text(font, std::to_string(digit), characterSize);
			text.setStyle(sf::Text::Bold);
			text.setFillColor(sf::Color(255, 224, 110));
			text.setOutlineColor(sf::Color(40, 22, 10));
			text.setOutlineThickness(2.0f);
			const sf::FloatRect bounds = text.getLocalBounds();
			text.setPosition({ digit * static_cast<float>(cellWidth) + (cellWidth - bounds.size.x) * 0.5f - bounds.position.x,
				(cellHeight - bounds.size.y) * 0.5f - bounds.position.y });
			atlas.draw(text);
		}
		advance = font.getGlyph(U'0', characterSize, true).advance + 1.0f;
		atlas.display();
		shader.setUniform("texture", sf::Shader::CurrentTexture);
		vertices.reserve(256);
		ready = true;
	}
};

inline DamageNumberRenderResources& GetDamageNumberRenderResources()
{
	static DamageNumberRenderResources resources;
	return resources;
}

inline void PreloadDamageNumberRenderer()
{
	GetDamageNumberRenderResources();
}

inline void RenderDamageNumbers(sf::RenderTarget& target, const std::vector<DamageNumber>& numbers,
	float cameraX, float cameraY, float viewportWidth, float viewportHeight)
{
	if (numbers.empty()) return;
	DamageNumberRenderResources& resources = GetDamageNumberRenderResources();
	if (!resources.ready) return;
	auto& vertices = resources.vertices;
	vertices.clear();
	for (const DamageNumber& number : numbers)
	{
		if (number.text.empty() || number.elapsed >= DamageNumber::duration) continue;
		const float width = DamageNumberRenderResources::cellWidth;
		const float height = DamageNumberRenderResources::cellHeight;
		const float centerX = number.x - cameraX + viewportWidth * 0.5f;
		const float centerY = number.y - cameraY + viewportHeight * 0.5f;
		const float textWidth = static_cast<float>(number.text.size() - 1) * resources.advance + width;
		if (centerX + textWidth * 0.5f < -8.0f || centerX - textWidth * 0.5f > viewportWidth ||
			centerY + height * 0.5f < 0.0f || centerY - height * 0.5f > viewportHeight + 48.0f) continue;
		const float remaining = std::clamp(1.0f - number.elapsed / DamageNumber::duration, 0.0f, 1.0f);
		const sf::Color timing(255, 255, 255, static_cast<std::uint8_t>(std::lround(remaining * 255.0f)));
		float x = centerX - textWidth * 0.5f;
		const float y = centerY - height * 0.5f;
		for (char digit : number.text)
		{
			const float u = (digit - '0') * width;
			const sf::Vertex topLeft({ x, y }, timing, { u, 0 });
			const sf::Vertex topRight({ x + width, y }, timing, { u + width, 0 });
			const sf::Vertex bottomLeft({ x, y + height }, timing, { u, height });
			const sf::Vertex bottomRight({ x + width, y + height }, timing, { u + width, height });
			vertices.insert(vertices.end(), { topLeft, bottomLeft, topRight, topRight, bottomLeft, bottomRight });
			x += resources.advance;
		}
	}
	if (vertices.empty()) return;
	sf::RenderStates states;
	states.texture = &resources.atlas.getTexture();
	states.shader = &resources.shader;
	target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles, states);
}
