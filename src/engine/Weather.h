#pragma once

#include <cmath>
#include <string>

enum class WeatherType
{
	None,
	Rain,
	Fog
};

inline bool TryParseWeather(const std::string& value, WeatherType& outWeather)
{
	if (value == "none") outWeather = WeatherType::None;
	else if (value == "rain") outWeather = WeatherType::Rain;
	else if (value == "fog") outWeather = WeatherType::Fog;
	else return false;
	return true;
}

struct WeatherState
{
	WeatherType type = WeatherType::None;
	float elapsed = 0.0f;
};

inline void SetWeather(WeatherState& weather, WeatherType type)
{
	if (weather.type != type)
	{
		weather.type = type;
		weather.elapsed = 0.0f;
	}
}

inline void UpdateWeather(WeatherState& weather, float deltaTime)
{
	if (weather.type != WeatherType::None)
	{
		weather.elapsed = std::fmod(weather.elapsed + deltaTime, 3600.0f);
	}
}
