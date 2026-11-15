#pragma once

#include <SFML/Graphics.hpp>

#include <cstdio>

#include "Weather.h"

inline sf::Shader* GetWeatherShader(WeatherType type)
{
	if (type == WeatherType::None)
	{
		return nullptr;
	}

	struct WeatherShaders
	{
		sf::Shader rain;
		sf::Shader fog;
		bool rainReady = false;
		bool fogReady = false;

		WeatherShaders()
		{
			if (!sf::Shader::isAvailable())
			{
				std::fprintf(stderr, "Weather shaders are not available on this system.\n");
				return;
			}
			rainReady = rain.loadFromFile("Assets/Shaders/Weather/weather.vert", "Assets/Shaders/Weather/rain.frag");
			fogReady = fog.loadFromFile("Assets/Shaders/Weather/weather.vert", "Assets/Shaders/Weather/fog.frag");
			if (!rainReady) std::fprintf(stderr, "Failed to load rain shader.\n");
			if (!fogReady) std::fprintf(stderr, "Failed to load fog shader.\n");
		}
	};
	static WeatherShaders shaders;
	if (type == WeatherType::Rain) return shaders.rainReady ? &shaders.rain : nullptr;
	if (type == WeatherType::Fog) return shaders.fogReady ? &shaders.fog : nullptr;
	return nullptr;
}

inline void PreloadWeatherShaders()
{
	GetWeatherShader(WeatherType::Rain);
}

inline void RenderWeather(sf::RenderTarget& target, const WeatherState& weather, float width, float height)
{
	sf::Shader* shader = GetWeatherShader(weather.type);
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
	sf::RenderStates states;
	states.shader = shader;
	target.draw(quad, 6, sf::PrimitiveType::Triangles, states);
}
