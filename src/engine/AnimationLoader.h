#pragma once

#include <SFML/Graphics.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
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

// Liest ANIM 1; frame-Rechtecke in Texturpixeln werden in Dateireihenfolge abgespielt.
// Die Ausgabe bleibt bei Fehlern unverändert. Der optionale scale-Faktor ist rein visuell.
inline bool LoadAnimationFromFile(const std::string& path, SpriteAnimation& animation)
{
	std::ifstream file(path);
	if (!file)
	{
		std::fprintf(stderr, "Failed to open animation file: %s\n", path.c_str());
		return false;
	}

	SpriteAnimation loadedAnimation;
	bool headerRead = false;
	bool texturePathRead = false;
	bool frameDurationRead = false;
	bool scaleRead = false;
	std::string line;
	std::size_t lineNumber = 0;

	while (std::getline(file, line))
	{
		++lineNumber;
		std::istringstream lineStream(line);
		std::string directive;
		if (!(lineStream >> directive) || directive.front() == '#')
		{
			continue;
		}

		if (!headerRead)
		{
			unsigned int version = 0;
			if (directive != "ANIM" || !(lineStream >> version) || version != 1)
			{
				std::fprintf(stderr, "Invalid animation header at %s:%zu (expected ANIM 1)\n", path.c_str(), lineNumber);
				return false;
			}

			headerRead = true;
			continue;
		}

		if (directive == "texture")
		{
			if (texturePathRead || !(lineStream >> loadedAnimation.texturePath) || loadedAnimation.texturePath.empty())
			{
				std::fprintf(stderr, "Invalid or duplicate texture directive at %s:%zu\n", path.c_str(), lineNumber);
				return false;
			}

			texturePathRead = true;
		}
		else if (directive == "frame_duration")
		{
			if (frameDurationRead || !(lineStream >> loadedAnimation.frameDuration) ||
				!std::isfinite(loadedAnimation.frameDuration) || loadedAnimation.frameDuration <= 0.0f)
			{
				std::fprintf(stderr, "Invalid or duplicate frame_duration directive at %s:%zu\n", path.c_str(), lineNumber);
				return false;
			}

			frameDurationRead = true;
		}
		else if (directive == "scale")
		{
			if (scaleRead || !(lineStream >> loadedAnimation.scale) ||
				!std::isfinite(loadedAnimation.scale) || loadedAnimation.scale <= 0.0f)
			{
				std::fprintf(stderr, "Invalid or duplicate scale directive at %s:%zu\n", path.c_str(), lineNumber);
				return false;
			}

			scaleRead = true;
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
	}

	if (!headerRead || !texturePathRead || !frameDurationRead || loadedAnimation.frames.empty())
	{
		std::fprintf(stderr, "Animation file is missing required data: %s\n", path.c_str());
		return false;
	}

	animation = std::move(loadedAnimation);
	return true;
}
