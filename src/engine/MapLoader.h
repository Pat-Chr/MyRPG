#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "GameTypes.h"

inline MapObjectType ParseMapObjectType(const std::string& value)
{
	if (value == "wall")
	{
		return MapObjectType::Wall;
	}

	if (value == "npc")
	{
		return MapObjectType::Npc;
	}

	return MapObjectType::Scenery;
}

inline float SnapToGrid(float value, float gridSize)
{
	return std::round(value / gridSize) * gridSize;
}

inline bool TryLoadMapFromFile(const std::string& filePath, MapData& outMap)
{
	std::ifstream input(filePath);
	if (!input.is_open())
	{
		return false;
	}

	MapData loadedMap;
	std::string line;

	while (std::getline(input, line))
	{
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		std::istringstream lineStream(line);
		std::string keyword;
		lineStream >> keyword;

		if (keyword == "MAP")
		{
			lineStream >> loadedMap.id;
			continue;
		}

		if (keyword == "OBJECT")
		{
			MapObject object;
			std::string type;
			int solidFlag = 0;

			lineStream >> object.id >> type >> object.x >> object.y >> object.spritePath >> solidFlag;
			if (!lineStream.fail())
			{
				object.type = ParseMapObjectType(type);
				if (object.type == MapObjectType::Scenery)
				{
					constexpr float gridSize = 64.0f;
					object.x = SnapToGrid(object.x, gridSize);
					object.y = SnapToGrid(object.y, gridSize);
				}
				object.solid = solidFlag != 0;
				loadedMap.objects.push_back(object);
			}

			continue;
		}

		if (keyword == "TRIGGER")
		{
			TriggerZone trigger;
			lineStream >> trigger.id >> trigger.x >> trigger.y >> trigger.width >> trigger.height >> trigger.eventName;

			if (!lineStream.fail())
			{
				loadedMap.triggers.push_back(trigger);
			}
		}
	}

	if (loadedMap.id.empty())
	{
		return false;
	}

	outMap = std::move(loadedMap);
	return true;
}
