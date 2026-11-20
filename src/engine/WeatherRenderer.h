#pragma once

#include <SFML/Graphics.hpp>

#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Weather.h"

// Erst mit aktivem Renderkontext aufrufen. Jeder Shader wird höchstens einmal zu laden versucht.
inline sf::Shader* GetWeatherShader(const std::string& effectId)
{
	if (effectId == "none")
	{
		return nullptr;
	}

	const WeatherEffectDefinition* definition = FindWeatherEffectDefinition(effectId);
	if (definition == nullptr)
	{
		return nullptr;
	}

	static std::unordered_map<std::string, std::unique_ptr<sf::Shader>> shaders;
	static std::unordered_set<std::string> loadAttempted;
	if (loadAttempted.insert(effectId).second)
	{
		if (!sf::Shader::isAvailable())
		{
			std::fprintf(stderr, "Weather shaders are not available on this system.\n");
		}
		else
		{
			auto loaded = std::make_unique<sf::Shader>();
			if (loaded->loadFromFile("Assets/Shaders/Weather/weather.vert", definition->fragmentPath))
			{
				shaders.emplace(effectId, std::move(loaded));
			}
			else
			{
				std::fprintf(stderr, "Failed to load weather shader: %s\n", definition->fragmentPath.c_str());
			}
		}
	}

	const auto shader = shaders.find(effectId);
	return shader == shaders.end() ? nullptr : shader->second.get();
}

inline void PreloadWeatherShaders()
{
	for (const WeatherEffectDefinition& definition : GetWeatherEffectDefinitions())
	{
		GetWeatherShader(definition.id);
	}
}

// Hitze verzerrt die fertige Welttextur, statt wie die übrigen Effekte ein Overlay zu zeichnen.
inline void RenderHeat(sf::RenderTarget& target, const sf::RenderTexture& scene, const WeatherState& weather,
	float width, float height, sf::Vector2f worldOrigin)
{
	sf::Shader* shader = GetWeatherShader(weather.effectId);
	if (shader == nullptr) return;

	shader->setUniform("time", weather.elapsed);
	shader->setUniform("resolution", sf::Glsl::Vec2(width, height));
	shader->setUniform("worldOrigin", sf::Glsl::Vec2(worldOrigin));
	shader->setUniform("scene", sf::Shader::CurrentTexture);
	sf::Sprite sprite(scene.getTexture());
	target.draw(sprite, shader);
}

inline void RenderWeather(sf::RenderTarget& target, const WeatherState& weather, float width, float height,
	sf::Vector2f worldOrigin)
{
	sf::Shader* shader = GetWeatherShader(weather.effectId);
	if (shader == nullptr) return;

	const sf::Color white = sf::Color::White;
	const sf::Vertex quad[6] = {
		{ { 0.0f, 0.0f }, white, { 0.0f, 0.0f } },
		{ { width, 0.0f }, white, { 1.0f, 0.0f } },
		{ { 0.0f, height }, white, { 0.0f, 1.0f } },
		{ { width, 0.0f }, white, { 1.0f, 0.0f } },
		{ { width, height }, white, { 1.0f, 1.0f } },
		{ { 0.0f, height }, white, { 0.0f, 1.0f } }
	};
	shader->setUniform("time", weather.elapsed);
	shader->setUniform("resolution", sf::Glsl::Vec2(width, height));
	// Die Wolkenschatten sind weltfest; die Regentropfen bleiben bildschirmfest.
	if (weather.effectId == "rain")
	{
		shader->setUniform("worldOrigin", sf::Glsl::Vec2(worldOrigin));
	}
	sf::RenderStates states;
	states.shader = shader;
	target.draw(quad, 6, sf::PrimitiveType::Triangles, states);
}
