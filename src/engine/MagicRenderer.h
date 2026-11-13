#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <memory>

#include "MagicEffects.h"

// Darstellung eines Effekts: Fragment-Shader und Kantenlänge des Quads, auf dem er berechnet wird.
// Die Reihenfolge entspricht MagicEffectType.
struct MagicEffectVisual
{
	const char* fragmentPath;
	float quadSize;
};

inline const MagicEffectVisual& GetMagicEffectVisual(MagicEffectType type)
{
	static const std::array<MagicEffectVisual, static_cast<std::size_t>(MagicEffectType::Count)> visuals = { {
		{ "Assets/Shaders/Effects/magic_sparkle.frag", 160.0f },
		{ "Assets/Shaders/Effects/magic_fire.frag", 224.0f },
		{ "Assets/Shaders/Effects/magic_lightning.frag", 288.0f },
		{ "Assets/Shaders/Effects/magic_frost.frag", 256.0f },
		{ "Assets/Shaders/Effects/magic_vortex.frag", 224.0f },
		{ "Assets/Shaders/Effects/magic_heal.frag", 224.0f },
		{ "Assets/Shaders/Effects/magic_shockwave.frag", 288.0f }
	} };

	return visuals[static_cast<std::size_t>(type)];
}

// Lädt den Shader eines Effekts einmalig. Gibt nullptr zurück, wenn Shader nicht unterstützt werden
// oder das Laden fehlschlägt.
inline sf::Shader* GetMagicShader(MagicEffectType type)
{
	constexpr std::size_t count = static_cast<std::size_t>(MagicEffectType::Count);
	static std::array<std::unique_ptr<sf::Shader>, count> shaders;
	static std::array<bool, count> loadAttempted{};

	const std::size_t index = static_cast<std::size_t>(type);
	if (!loadAttempted[index])
	{
		loadAttempted[index] = true;
		const MagicEffectVisual& visual = GetMagicEffectVisual(type);

		if (!sf::Shader::isAvailable())
		{
			std::fprintf(stderr, "Shaders are not available on this system.\n");
		}
		else
		{
			auto loaded = std::make_unique<sf::Shader>();
			if (loaded->loadFromFile("Assets/Shaders/Effects/magic_effect.vert", visual.fragmentPath))
			{
				shaders[index] = std::move(loaded);
			}
			else
			{
				std::fprintf(stderr, "Failed to load magic shader: %s\n", visual.fragmentPath);
			}
		}
	}

	return shaders[index].get();
}

inline void RenderMagicEffects(sf::RenderTarget& target, const MagicEffects& effects,
							   float cameraX, float cameraY, float viewportWidth, float viewportHeight)
{
	for (const MagicEffect& effect : effects.active)
	{
		sf::Shader* shader = GetMagicShader(effect.type);
		if (shader == nullptr)
		{
			continue;
		}

		const float half = GetMagicEffectVisual(effect.type).quadSize * 0.5f;
		const float progress = std::clamp(effect.elapsed / effect.duration, 0.0f, 1.0f);
		const float cx = effect.x - cameraX + viewportWidth * 0.5f;
		const float cy = effect.y - cameraY + viewportHeight * 0.5f;

		sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
		quad[0] = sf::Vertex({ cx - half, cy - half }, sf::Color::White, { 0.0f, 0.0f });
		quad[1] = sf::Vertex({ cx + half, cy - half }, sf::Color::White, { 1.0f, 0.0f });
		quad[2] = sf::Vertex({ cx - half, cy + half }, sf::Color::White, { 0.0f, 1.0f });
		quad[3] = sf::Vertex({ cx + half, cy + half }, sf::Color::White, { 1.0f, 1.0f });

		shader->setUniform("progress", progress);

		sf::RenderStates states;
		states.shader = shader;
		states.blendMode = sf::BlendAdd;
		target.draw(quad, states);
	}
}
