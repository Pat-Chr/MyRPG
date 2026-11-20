#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>

#include "MagicEffectRegistry.h"
#include "MagicEffects.h"

// Welt-Effekte folgen der Kamera; Bildschirm-Effekte füllen den logischen Viewport unabhängig vom Auslöseort.
inline void RenderMagicEffects(sf::RenderTarget& target, const MagicEffects& effects,
							   float cameraX, float cameraY, float viewportWidth, float viewportHeight)
{
	for (const MagicEffect& effect : effects.active)
	{
		const MagicEffectDefinition* definition = FindMagicEffectDefinition(effect.type);
		if (definition == nullptr)
		{
			continue;
		}

		sf::Shader* shader = GetMagicShader(effect.type);
		if (shader == nullptr)
		{
			continue;
		}

		const bool screenMode = definition->mode == MagicEffectMode::Screen;
		const float half = definition->quadSize * 0.5f;
		const float cx = effect.x - cameraX + viewportWidth * 0.5f;
		const float cy = effect.y - cameraY + viewportHeight * 0.5f;
		const float left = screenMode ? 0.0f : cx - half;
		const float top = screenMode ? 0.0f : cy - half;
		const float right = screenMode ? viewportWidth : cx + half;
		const float bottom = screenMode ? viewportHeight : cy + half;
		const float progress = std::clamp(effect.elapsed / effect.duration, 0.0f, 1.0f);

		sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
		quad[0] = sf::Vertex({ left, top }, sf::Color::White, { 0.0f, 0.0f });
		quad[1] = sf::Vertex({ right, top }, sf::Color::White, { 1.0f, 0.0f });
		quad[2] = sf::Vertex({ left, bottom }, sf::Color::White, { 0.0f, 1.0f });
		quad[3] = sf::Vertex({ right, bottom }, sf::Color::White, { 1.0f, 1.0f });

		shader->setUniform("progress", progress);
		if (screenMode)
		{
			shader->setUniform("resolution", sf::Glsl::Vec2(viewportWidth, viewportHeight));
		}

		sf::RenderStates states;
		states.shader = shader;
		states.blendMode = sf::BlendAdd;
		target.draw(quad, states);
	}
}
