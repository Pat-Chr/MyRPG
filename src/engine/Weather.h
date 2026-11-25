#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

struct WeatherEffectDefinition
{
	std::string id;
	std::string fragmentPath;
};

// The ID comes from the fragment-shader filename; "none" is reserved for disabled weather.
inline const std::vector<WeatherEffectDefinition>& GetWeatherEffectDefinitions()
{
	static const std::vector<WeatherEffectDefinition> definitions = []
	{
		std::vector<WeatherEffectDefinition> found;
		const std::filesystem::path shaderDirectory = "Assets/Shaders/Weather";
		std::error_code error;
		std::filesystem::directory_iterator iterator(shaderDirectory, error);
		if (error)
		{
			std::fprintf(stderr, "Failed to scan weather shaders in %s: %s\n",
				shaderDirectory.string().c_str(), error.message().c_str());
			return found;
		}

		const std::filesystem::directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::path& path = iterator->path();
			const std::string id = path.stem().string();
			if (iterator->is_regular_file(error) && !error && path.extension() == ".frag" &&
				!id.empty() && id != "none")
			{
				found.push_back({ id, path.string() });
			}
			if (error)
			{
				std::fprintf(stderr, "Failed to inspect weather shader: %s\n", error.message().c_str());
				error.clear();
			}

			iterator.increment(error);
			if (error)
			{
				std::fprintf(stderr, "Failed to continue scanning weather shaders: %s\n",
					error.message().c_str());
				break;
			}
		}

		std::sort(found.begin(), found.end(), [](const WeatherEffectDefinition& left, const WeatherEffectDefinition& right)
		{
			return left.id < right.id;
		});
		return found;
	}();

	return definitions;
}

inline const WeatherEffectDefinition* FindWeatherEffectDefinition(const std::string& id)
{
	const auto& definitions = GetWeatherEffectDefinitions();
	const auto definition = std::find_if(definitions.begin(), definitions.end(), [&id](const WeatherEffectDefinition& item)
	{
		return item.id == id;
	});
	return definition == definitions.end() ? nullptr : &*definition;
}

inline bool TryParseWeather(const std::string& value, std::string& outWeather)
{
	if (value == "none")
	{
		outWeather = value;
		return true;
	}

	if (FindWeatherEffectDefinition(value) == nullptr)
	{
		return false;
	}

	outWeather = value;
	return true;
}

struct WeatherState
{
	std::string effectId = "none";
	float elapsed = 0.0f;
};

// The same weather trigger may fire again without restarting the active animation.
inline void SetWeather(WeatherState& weather, const std::string& effectId)
{
	if (weather.effectId != effectId)
	{
		weather.effectId = effectId;
		weather.elapsed = 0.0f;
	}
}

inline void UpdateWeather(WeatherState& weather, float deltaTime)
{
	if (weather.effectId != "none")
	{
		weather.elapsed = std::fmod(weather.elapsed + deltaTime, 3600.0f);
	}
}
