#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "MagicEffects.h"

inline void RenderMagicEffects(sf::RenderTarget& target, const MagicEffects& effects,
							   float cameraX, float cameraY, float viewportWidth, float viewportHeight)
{
	constexpr float pi = 3.14159265359f;
	for (const MagicEffect& effect : effects.active)
	{
		if (effect.type != MagicEffectType::Sparkle)
		{
			continue;
		}

		const float progress = std::clamp(effect.elapsed / effect.duration, 0.0f, 1.0f);
		const auto alpha = static_cast<std::uint8_t>(255.0f * (1.0f - progress));
		const sf::Vector2f center{ effect.x - cameraX + viewportWidth * 0.5f,
								   effect.y - cameraY + viewportHeight * 0.5f };
		const float radius = 12.0f + 32.0f * progress;
		sf::CircleShape ring(radius, 32);
		ring.setOrigin({ radius, radius });
		ring.setScale({ 1.0f, 0.45f });
		ring.setPosition(center);
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(2.0f);
		ring.setOutlineColor(sf::Color(120, 200, 255, alpha));
		target.draw(ring);

		for (int i = 0; i < 12; ++i)
		{
			const float angle = 2.0f * pi * static_cast<float>(i) / 12.0f + progress * 2.0f;
			const float distance = 12.0f + 24.0f * progress;
			sf::RectangleShape particle({ 4.0f, 4.0f });
			particle.setOrigin({ 2.0f, 2.0f });
			particle.setPosition({ center.x + std::cos(angle) * distance,
								   center.y + std::sin(angle) * distance * 0.4f - progress * 55.0f });
			particle.setFillColor(sf::Color(190, 225, 255, alpha));
			target.draw(particle);
		}
	}
}
