#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class MagicEffectMode
{
	World,
	Screen
};

struct MagicEffectDefinition
{
	std::string id;
	std::string fragmentPath;
	float quadSize = 224.0f;
	bool screenShake = false;
	MagicEffectMode mode = MagicEffectMode::World;
};

// Discovers magic_<id>.frag files once; magic_effects.cfg overrides existing definitions only.
inline const std::vector<MagicEffectDefinition>& GetMagicEffectDefinitions()
{
	static const std::vector<MagicEffectDefinition> definitions = []
	{
		std::vector<MagicEffectDefinition> found;
		const std::filesystem::path shaderDirectory = "Assets/Shaders/Effects";
		std::error_code error;
		std::filesystem::directory_iterator iterator(shaderDirectory, error);
		if (error)
		{
			std::fprintf(stderr, "Failed to scan magic effect shaders in %s: %s\n",
				shaderDirectory.string().c_str(), error.message().c_str());
			return found;
		}

		const std::filesystem::directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::path& path = iterator->path();
			const std::string stem = path.stem().string();
			if (iterator->is_regular_file(error) && !error && path.extension() == ".frag" &&
				stem.starts_with("magic_") && stem.size() > 6)
			{
				found.push_back({ stem.substr(6), path.string() });
			}
			if (error)
			{
				std::fprintf(stderr, "Failed to inspect magic effect shader: %s\n", error.message().c_str());
				error.clear();
			}

			iterator.increment(error);
			if (error)
			{
				std::fprintf(stderr, "Failed to continue scanning magic effect shaders: %s\n",
					error.message().c_str());
				break;
			}
		}

		std::sort(found.begin(), found.end(), [](const MagicEffectDefinition& left, const MagicEffectDefinition& right)
		{
			return left.id < right.id;
		});

		std::ifstream settings("Assets/Shaders/Effects/magic_effects.cfg");
		if (!settings)
		{
			std::fprintf(stderr, "Failed to open magic effect settings: Assets/Shaders/Effects/magic_effects.cfg\n");
			return found;
		}

		std::string line;
		while (std::getline(settings, line))
		{
			if (line.empty() || line[0] == '#')
			{
				continue;
			}

			std::istringstream row(line);
			std::string id;
			float quadSize = 0.0f;
			if (!(row >> id >> quadSize) || !std::isfinite(quadSize) || quadSize <= 0.0f)
			{
				std::fprintf(stderr, "Invalid magic effect settings row: %s\n", line.c_str());
				continue;
			}

			bool screenShake = false;
			MagicEffectMode mode = MagicEffectMode::World;
			bool hasScreenShake = false;
			bool hasMode = false;
			bool valid = true;
			std::string option;
			while (row >> option)
			{
				if (!hasScreenShake && (option == "screen_shake=0" || option == "screen_shake=1"))
				{
					screenShake = option == "screen_shake=1";
					hasScreenShake = true;
				}
				else if (!hasMode && (option == "mode=world" || option == "mode=screen"))
				{
					mode = option == "mode=screen" ? MagicEffectMode::Screen : MagicEffectMode::World;
					hasMode = true;
				}
				else
				{
					valid = false;
					break;
				}
			}
			if (!valid)
			{
				std::fprintf(stderr, "Invalid magic effect settings row: %s\n", line.c_str());
				continue;
			}

			const auto definition = std::find_if(found.begin(), found.end(), [&id](const MagicEffectDefinition& item)
			{
				return item.id == id;
			});
			if (definition == found.end())
			{
				std::fprintf(stderr, "Magic effect settings reference unknown shader: %s\n", id.c_str());
				continue;
			}
			definition->quadSize = quadSize;
			definition->screenShake = screenShake;
			definition->mode = mode;
		}

		return found;
	}();

	return definitions;
}

inline const MagicEffectDefinition* FindMagicEffectDefinition(const std::string& id)
{
	const auto& definitions = GetMagicEffectDefinitions();
	const auto definition = std::find_if(definitions.begin(), definitions.end(), [&id](const MagicEffectDefinition& item)
	{
		return item.id == id;
	});
	return definition == definitions.end() ? nullptr : &*definition;
}

// Call only with an active render context. Failed shader-load attempts are not repeated.
inline sf::Shader* GetMagicShader(const std::string& id)
{
	static std::unordered_map<std::string, std::unique_ptr<sf::Shader>> shaders;
	static std::unordered_set<std::string> loadAttempted;
	const MagicEffectDefinition* definition = FindMagicEffectDefinition(id);
	if (definition == nullptr)
	{
		return nullptr;
	}

	if (loadAttempted.insert(id).second)
	{
		if (!sf::Shader::isAvailable())
		{
			std::fprintf(stderr, "Shaders are not available on this system.\n");
		}
		else
		{
			auto loaded = std::make_unique<sf::Shader>();
			if (loaded->loadFromFile("Assets/Shaders/Effects/magic_effect.vert", definition->fragmentPath))
			{
				shaders.emplace(id, std::move(loaded));
			}
			else
			{
				std::fprintf(stderr, "Failed to load magic shader: %s\n", definition->fragmentPath.c_str());
			}
		}
	}

	const auto shader = shaders.find(id);
	return shader == shaders.end() ? nullptr : shader->second.get();
}

inline void PreloadMagicShaders()
{
	for (const MagicEffectDefinition& definition : GetMagicEffectDefinitions())
	{
		GetMagicShader(definition.id);
	}
}
