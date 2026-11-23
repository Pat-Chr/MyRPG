#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct SpriteAnimation
{
	std::string texturePath;
	float frameDuration = 0.0f;
	float scale = 1.0f;
	std::vector<sf::IntRect> frames;
};

struct AnimationParseState
{
	SpriteAnimation animation;
	bool texturePathRead = false;
	bool frameDurationRead = false;
	bool scaleRead = false;
};

inline bool ParseAnimationDirective(const std::string& path, std::size_t lineNumber,
	const std::string& directive, std::istringstream& lineStream, AnimationParseState& state)
{
	SpriteAnimation& loadedAnimation = state.animation;
	if (directive == "texture")
	{
		if (state.texturePathRead || !(lineStream >> loadedAnimation.texturePath) || loadedAnimation.texturePath.empty())
		{
			std::fprintf(stderr, "Invalid or duplicate texture directive at %s:%zu\n", path.c_str(), lineNumber);
			return false;
		}
		state.texturePathRead = true;
	}
	else if (directive == "frame_duration")
	{
		if (state.frameDurationRead || !(lineStream >> loadedAnimation.frameDuration) ||
			!std::isfinite(loadedAnimation.frameDuration) || loadedAnimation.frameDuration <= 0.0f)
		{
			std::fprintf(stderr, "Invalid or duplicate frame_duration directive at %s:%zu\n", path.c_str(), lineNumber);
			return false;
		}
		state.frameDurationRead = true;
	}
	else if (directive == "scale")
	{
		if (state.scaleRead || !(lineStream >> loadedAnimation.scale) ||
			!std::isfinite(loadedAnimation.scale) || loadedAnimation.scale <= 0.0f)
		{
			std::fprintf(stderr, "Invalid or duplicate scale directive at %s:%zu\n", path.c_str(), lineNumber);
			return false;
		}
		state.scaleRead = true;
	}
	else if (directive == "frame")
	{
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
		if (!(lineStream >> x >> y >> width >> height) || x < 0 || y < 0 || width <= 0 || height <= 0)
		{
			std::fprintf(stderr, "Invalid frame rectangle at %s:%zu\n", path.c_str(), lineNumber);
			return false;
		}
		loadedAnimation.frames.push_back({ { x, y }, { width, height } });
	}
	else
	{
		std::fprintf(stderr, "Unknown animation directive '%s' at %s:%zu\n", directive.c_str(), path.c_str(), lineNumber);
		return false;
	}

	std::string trailingToken;
	if (lineStream >> trailingToken)
	{
		std::fprintf(stderr, "Unexpected value '%s' at %s:%zu\n", trailingToken.c_str(), path.c_str(), lineNumber);
		return false;
	}
	return true;
}

inline bool IsCompleteAnimation(const AnimationParseState& state)
{
	return state.texturePathRead && state.frameDurationRead && !state.animation.frames.empty();
}

// ANIM 1 remains the format for individual player animations.
inline bool LoadAnimationFromFile(const std::string& path, SpriteAnimation& animation)
{
	std::ifstream file(path);
	if (!file)
	{
		std::fprintf(stderr, "Failed to open animation file: %s\n", path.c_str());
		return false;
	}
	AnimationParseState state;
	bool headerRead = false;
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(file, line))
	{
		++lineNumber;
		std::istringstream lineStream(line);
		std::string directive;
		if (!(lineStream >> directive) || directive.front() == '#') continue;
		if (!headerRead)
		{
			unsigned int version = 0;
			std::string extra;
			if (directive != "ANIM" || !(lineStream >> version) || version != 1 || (lineStream >> extra))
			{
				std::fprintf(stderr, "Invalid animation header at %s:%zu (expected ANIM 1)\n", path.c_str(), lineNumber);
				return false;
			}
			headerRead = true;
		}
		else if (!ParseAnimationDirective(path, lineNumber, directive, lineStream, state)) return false;
	}
	if (file.bad() || !headerRead || !IsCompleteAnimation(state))
	{
		std::fprintf(stderr, "Animation file is unreadable or missing required data: %s\n", path.c_str());
		return false;
	}
	animation = std::move(state.animation);
	return true;
}

// Indices 0-3 are idle directions, 4-7 are optional walk directions.
using NpcAnimations = std::array<std::optional<SpriteAnimation>, 8>;

inline bool LoadNpcAnimationsFromFile(const std::string& path, NpcAnimations& animations)
{
	std::ifstream file(path);
	if (!file)
	{
		std::fprintf(stderr, "Failed to open NPC animation file: %s\n", path.c_str());
		return false;
	}
	const std::array<std::string, 8> names{ "idle_down", "idle_up", "idle_left", "idle_right",
		"walk_down", "walk_up", "walk_left", "walk_right" };
	NpcAnimations loaded;
	AnimationParseState state;
	std::size_t current = names.size();
	bool headerRead = false;
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(file, line))
	{
		++lineNumber;
		std::istringstream lineStream(line);
		std::string directive;
		if (!(lineStream >> directive) || directive.front() == '#') continue;
		if (!headerRead)
		{
			unsigned int version = 0;
			std::string extra;
			if (directive != "ANIM" || !(lineStream >> version) || version != 2 || (lineStream >> extra))
			{
				std::fprintf(stderr, "Invalid NPC animation header at %s:%zu (expected ANIM 2)\n", path.c_str(), lineNumber);
				return false;
			}
			headerRead = true;
		}
		else if (directive == "animation")
		{
			std::string name, extra;
			if (!(lineStream >> name) || (lineStream >> extra))
			{
				std::fprintf(stderr, "Invalid NPC animation name at %s:%zu\n", path.c_str(), lineNumber);
				return false;
			}
			if (current != names.size())
			{
				if (!IsCompleteAnimation(state))
				{
					std::fprintf(stderr, "Incomplete NPC animation '%s' at %s:%zu\n", names[current].c_str(), path.c_str(), lineNumber);
					return false;
				}
				loaded[current] = std::move(state.animation);
				state = AnimationParseState{};
			}
			current = names.size();
			for (std::size_t i = 0; i < names.size(); ++i)
				if (names[i] == name) { current = i; break; }
			if (current == names.size() || loaded[current])
			{
				std::fprintf(stderr, "Unknown or duplicate NPC animation '%s' at %s:%zu\n", name.c_str(), path.c_str(), lineNumber);
				return false;
			}
		}
		else if (current == names.size())
		{
			std::fprintf(stderr, "NPC animation directive outside a section at %s:%zu\n", path.c_str(), lineNumber);
			return false;
		}
		else if (!ParseAnimationDirective(path, lineNumber, directive, lineStream, state)) return false;
	}
	if (file.bad() || !headerRead || current == names.size() || !IsCompleteAnimation(state))
	{
		std::fprintf(stderr, "NPC animation file is unreadable or missing required data: %s\n", path.c_str());
		return false;
	}
	loaded[current] = std::move(state.animation);
	for (std::size_t i = 0; i < 4; ++i)
	{
		if (!loaded[i])
		{
			std::fprintf(stderr, "Missing NPC animation '%s': %s\n", names[i].c_str(), path.c_str());
			return false;
		}
	}
	animations = std::move(loaded);
	return true;
}
