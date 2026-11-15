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
 * @brief Rundet eine Koordinaten auf das nächste Grid-Gitter.
 * 
 * Wird verwendet, um Positionsdaten von NPCs oder Scenery-Objekten
 * an das 64px-Grid des Spiels zu schnappen (Snap-to-Grid).
 * 
 * @param value Die Roh-Koordinate, die gerundet werden soll.
 * @param gridSize Die Größe eines Grid-Zells in Pixeln (Standard: 64).
 * @return float Die auf das nächste Grid gerundete Koordinate.
 */
inline float SnapToGrid(float value, float gridSize)
{
	return std::round(value / gridSize) * gridSize;
}

/**
 * @brief Lädt eine Map-Datei aus einer Textdatei und parsst sie in ein MapData-Objekt.
 * 
 * Unterstützt folgende Keywords im Dateiformat:
 * - MAP ID         : Setzt die Map-ID
 * - GROUND path    : Setzt den Pfad zum Boden-Sprite
 * - WEATHER none|rain|fog : Setzt das Standardwetter
 * - OBJECT id type x y spritePath solid=0|1 : Lädt ein Objekt (wird auf Grid gesnapped)
 * - TRIGGER id x y width height eventName [glow=0|1] [spawn_x=x spawn_y=y] : Definiert eine Trigger-Zone
 * 
 * Zeilen, die leer sind oder mit '#' beginnen, werden ignoriert.
 * 
 * @param filePath Der Pfad zur Map-Textdatei.
 * @param outMap Referenz auf das MapData-Objekt zum Ausfüllen.
 * @return true Wenn die Map erfolgreich geladen wurde (MAP ID vorhanden).
 * @return false Wenn die Datei nicht gefunden, leer oder ohne MAP ID ist.
 * 
 * Beispiel Dateiformat:
 * # This is a comment
 * MAP my_level_01
 * GROUND grass.png
 * OBJECT 1 npc 128 64 player_sprite.png solid=1
 * OBJECT 2 wall 320 480 stone_wall.png solid=1
 * OBJECT 3 untergrund 0 0 ground_tile.png solid=0
 * TRIGGER 5 100 200 150 on_level_complete
 */
inline bool TryLoadMapFromFile(const std::string& filePath, MapData& outMap)
{
	std::ifstream input(filePath);
	if (!input.is_open())
	{
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
		return false;
	}

	outMap = std::move(loadedMap);
	return true;
}
