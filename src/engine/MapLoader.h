// file: src\engine\MapLoader.h
#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "GameTypes.h"

inline std::string EncodeObjectName(const std::string& name)
{
	constexpr char hex[] = "0123456789ABCDEF";
	std::string encoded;
	for (unsigned char c : name)
	{
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') encoded += static_cast<char>(c);
		else { encoded += '%'; encoded += hex[c >> 4]; encoded += hex[c & 15]; }
	}
	return encoded;
}

inline bool DecodeObjectName(const std::string& encoded, std::string& name)
{
	auto digit = [](char c) -> int {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		return -1;
	};
	std::string result;
	for (std::size_t i = 0; i < encoded.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(encoded[i]);
		if (c == '%')
		{
			if (i + 2 >= encoded.size()) return false;
			const int high = digit(encoded[i + 1]), low = digit(encoded[i + 2]);
			if (high < 0 || low < 0) return false;
			c = static_cast<unsigned char>((high << 4) | low);
			i += 2;
		}
		if (c < 32 || c == 127) return false;
		result += static_cast<char>(c);
	}
	name = std::move(result);
	return true;
}

inline bool TryParseWalkingCells(const std::string& value, std::vector<WalkingCell>& cells)
{
	if (value.empty()) return false;
	std::vector<WalkingCell> parsed;
	std::size_t start = 0;
	while (start < value.size())
	{
		const std::size_t comma = value.find(',', start);
		const std::size_t end = value.find(';', start);
		const std::size_t limit = end == std::string::npos ? value.size() : end;
		if (comma == std::string::npos || comma >= limit || comma == start || comma + 1 == limit) return false;
		WalkingCell cell;
		const char* base = value.data();
		const auto x = std::from_chars(base + start, base + comma, cell.x);
		const auto y = std::from_chars(base + comma + 1, base + limit, cell.y);
		if (x.ec != std::errc{} || x.ptr != base + comma || y.ec != std::errc{} || y.ptr != base + limit ||
			std::abs(static_cast<long long>(cell.x)) > 1000000 || std::abs(static_cast<long long>(cell.y)) > 1000000 ||
			parsed.size() >= 4096 || std::find(parsed.begin(), parsed.end(), cell) != parsed.end()) return false;
		parsed.push_back(cell);
		if (end == std::string::npos) break;
		start = end + 1;
		if (start == value.size()) return false;
	}
	cells = std::move(parsed);
	return true;
}

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
 * - "scenery"    -> MapObjectType::Scenery (Dekoration)
 * 
 * @param value Der String-Wert, der interpretiert werden soll.
 * @return MapObjectType Das zugehörige Enum-Wert; unbekannte Typen werden als Dekoration behandelt.
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

				std::string token;
				bool hasName = false, hasWalkingArea = false, hasWalkingCells = false;
				while (lineStream >> token)
				{
					if (token.starts_with("tile="))
					{
						char comma1 = 0, comma2 = 0, comma3 = 0;
						std::istringstream tileStream(token.substr(5));
						tileStream >> object.tileX >> comma1 >> object.tileY >> comma2 >> object.tileWidth >> comma3 >> object.tileHeight;
						object.hasTile = !tileStream.fail() && comma1 == ',' && comma2 == ',' && comma3 == ',' &&
							object.tileX >= 0 && object.tileY >= 0 && object.tileWidth > 0 && object.tileHeight > 0;
					}
					else if (token.starts_with("name="))
					{
						if (hasName || !DecodeObjectName(token.substr(5), object.npcName))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid object name: " << token << '\n';
							return false;
						}
						hasName = true;
					}
					else if (token.starts_with("walking_area="))
					{
						int flag = 0;
						if (object.type != MapObjectType::Npc || hasWalkingArea || !TryParseFlag(token, "walking_area", flag))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid walking area: " << token << '\n';
							return false;
						}
						hasWalkingArea = true;
						object.walkingAreaEnabled = flag != 0;
					}
					else if (token.starts_with("walk_cells="))
					{
						if (object.type != MapObjectType::Npc || hasWalkingCells || !TryParseWalkingCells(token.substr(11), object.walkingCells))
						{
							std::cerr << filePath << ':' << lineNumber << ": Invalid walking cells: " << token << '\n';
							return false;
						}
						hasWalkingCells = true;
					}
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
