// file: src\engine\MapLoader.h
#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "GameTypes.h"

// Parst ein Flag der Form "<key>=0" oder "<key>=1".
inline bool TryParseFlag(const std::string& token, const std::string& key, int& outValue)
{
	const std::string prefix = key + "=";
	if (token.size() != prefix.size() + 1 || token.compare(0, prefix.size(), prefix) != 0)
	{
		return false;
	}

	const char value = token.back();
	if (value != '0' && value != '1')
	{
		return false;
	}

	outValue = value - '0';
	return true;
}

inline bool TryParseCoordinate(const std::string& token, const std::string& key, float& outValue)
{
	const std::string prefix = key + "=";
	if (!token.starts_with(prefix))
	{
		return false;
	}

	std::istringstream valueStream(token.substr(prefix.size()));
	float value = 0.0f;
	if (!(valueStream >> value) || !valueStream.eof() || !std::isfinite(value))
	{
		return false;
	}

	outValue = value;
	return true;
}

/**
 * @brief Wandelt einen String-Wert in ein MapObjectType-Enum um.
 * 
 * Unterstützt die folgenden Typen:
 * - "untergrund" -> MapObjectType::Untergrund (Bodenobjekte)
 * - "wall"       -> MapObjectType::Wall (Wände/Barrieren)
 * - "npc"        -> MapObjectType::Npc (NPC-Charaktere)
 * 
 * @param value Der String-Wert, der interpretiert werden soll.
 * @return MapObjectType Das zugehörige Enum-Wert, falls bekannt.
 * @return MapObjectType::Scenery als Default für unbekannte Typen.
 */
inline MapObjectType ParseMapObjectType(const std::string& value)
{
	if (value == "untergrund")
	{
		return MapObjectType::Untergrund;
	}

	if (value == "wall")
	{
		return MapObjectType::Wall;
	}

	if (value == "npc")
	{
		return MapObjectType::Npc;
	}

	return MapObjectType::Scenery; // Default für unbekannte Typen
}

/**
 * @brief Rundet eine Koordinate auf das vorgegebene Raster.
 *
 * Der Kartenlader nutzt das 64-Pixel-Raster nur für Untergrund und Scenery.
 *
 * @param value Koordinate vor dem Runden.
 * @param gridSize Rasterabstand in Pixeln.
 * @return Gerundete Koordinate.
 */
inline float SnapToGrid(float value, float gridSize)
{
	return std::round(value / gridSize) * gridSize;
}

/**
 * @brief Lädt eine Karte im Textformat (siehe DesignDocument.md).
 *
 * Eine MAP-ID ist Pflicht. Unlesbare OBJECT- und TRIGGER-Grunddaten werden
 * übersprungen; ungültige WEATHER-, Glow- oder Spawn-Angaben brechen das Laden ab.
 * Spawn-Koordinaten müssen paarweise angegeben werden.
 *
 * @param filePath Pfad zur Kartendatei.
 * @param outMap Wird nur nach erfolgreichem Laden ersetzt.
 * @return true, wenn die Karte geladen wurde.
 */
inline bool TryLoadMapFromFile(const std::string& filePath, MapData& outMap)
{
#ifdef _DEBUG
	std::cout << "[Map] Loading '" << filePath << "'...\n";
#endif
	std::ifstream input(filePath);
	if (!input.is_open())
	{
#ifdef _DEBUG
		std::cout << "[Map] Failed to open '" << filePath << "'.\n";
#endif
		return false; // Datei konnte nicht geöffnet werden
	}

	MapData loadedMap;
	std::string line;
	std::size_t lineNumber = 0;
	bool hasWeather = false;

	while (std::getline(input, line))
	{
		++lineNumber;
		// Leerzeilen und Kommentare ignorieren
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		std::istringstream lineStream(line);
		std::string keyword;
		lineStream >> keyword;

		if (keyword == "MAP")
		{
			lineStream >> loadedMap.id; // Map-ID ist erforderlich für eine gültige Map
			continue;
		}

		if (keyword == "GROUND")
		{
			lineStream >> loadedMap.defaultGroundPath; // Pfad zum Boden-Sprite
			continue;
		}

		if (keyword == "WEATHER")
		{
			std::string value;
			std::string extra;
			if (hasWeather || !(lineStream >> value) || !TryParseWeather(value, loadedMap.defaultWeather) || (lineStream >> extra))
			{
				std::cerr << filePath << ':' << lineNumber << ": Invalid WEATHER directive\n";
				return false;
			}
			hasWeather = true;
			continue;
		}

		if (keyword == "OBJECT")
		{
			MapObject object;
			std::string type;
			std::string solidToken;

			lineStream >> object.id >> type >> object.x >> object.y >> object.spritePath >> solidToken;
			int solidFlag = 0;
			if (!lineStream.fail() && !TryParseFlag(solidToken, "solid", solidFlag))
			{
				lineStream.setstate(std::ios::failbit);
			}
			if (!lineStream.fail()) // Nur parsen, wenn alle Werte erfolgreich gelesen wurden
			{
				object.type = ParseMapObjectType(type);
				
				// Bodenobjekte und Scenery auf 64px-Grid snapen
				if (object.type == MapObjectType::Scenery || object.type == MapObjectType::Untergrund)
				{
					constexpr float gridSize = 64.0f;
					object.x = SnapToGrid(object.x, gridSize);
					object.y = SnapToGrid(object.y, gridSize);
				}
				
				object.solid = solidFlag != 0; // Festkörper-Objekte wenn Flag != 0

				std::string tileToken;
				if (lineStream >> tileToken && tileToken.starts_with("tile="))
				{
					char comma1 = 0, comma2 = 0, comma3 = 0;
					std::istringstream tileStream(tileToken.substr(5));
					tileStream >> object.tileX >> comma1 >> object.tileY >> comma2 >> object.tileWidth >> comma3 >> object.tileHeight;
					object.hasTile = !tileStream.fail() && comma1 == ',' && comma2 == ',' && comma3 == ',' &&
						object.tileX >= 0 && object.tileY >= 0 && object.tileWidth > 0 && object.tileHeight > 0;
				}
				loadedMap.objects.push_back(object);
			}

			continue;
		}

		if (keyword == "TRIGGER")
		{
			TriggerZone trigger;
			lineStream >> trigger.id >> trigger.x >> trigger.y >> trigger.width >> trigger.height >> trigger.eventName;

			if (!lineStream.fail()) // Nur hinzufügen, wenn alle Werte erfolgreich gelesen wurden
			{
				std::string token;
				bool hasSpawnX = false;
				bool hasSpawnY = false;
				while (lineStream >> token)
				{
					int glowFlag = 0;
					if (token.starts_with("glow="))
					{
						if (!TryParseFlag(token, "glow", glowFlag))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid trigger option: " << token << '\n';
							return false;
						}
						trigger.glow = glowFlag != 0;
					}
					else if (token.starts_with("spawn_x="))
					{
						if (hasSpawnX || !TryParseCoordinate(token, "spawn_x", trigger.spawnX))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid trigger option: " << token << '\n';
							return false;
						}
						hasSpawnX = true;
					}
					else if (token.starts_with("spawn_y="))
					{
						if (hasSpawnY || !TryParseCoordinate(token, "spawn_y", trigger.spawnY))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid trigger option: " << token << '\n';
							return false;
						}
						hasSpawnY = true;
					}
				}
				if (hasSpawnX != hasSpawnY)
				{
					std::cerr << filePath << ':' << lineNumber << ": Both spawn_x and spawn_y are required\n";
					return false;
				}

				loadedMap.triggers.push_back(trigger);
			}
		}
	}

	// Map ohne ID ist ungültig - keine leeren Maps zurückgeben
	if (loadedMap.id.empty())
	{
#ifdef _DEBUG
		std::cout << "[Map] Failed to load '" << filePath << "': map ID is missing.\n";
#endif
		return false;
	}

	outMap = std::move(loadedMap);
#ifdef _DEBUG
	std::cout << "[Map] Loaded '" << outMap.id << "' from '" << filePath << "' ("
		<< outMap.objects.size() << " objects, " << outMap.triggers.size() << " triggers).\n";
#endif
	return true;
}
