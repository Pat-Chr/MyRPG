#pragma once

#include "../BuildConfig.h"

#ifdef MYRPG_MAP_EDITOR
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../engine/MapLoader.h"

class MapDocument
{
public:
	MapData map;
	std::filesystem::path path;
	bool dirty = false;

	bool HasUsedId(const std::string& id) const
	{
		return Find(map.objects, id) != map.objects.end() || Find(originalObjects, id) != originalObjects.end();
	}

	std::optional<std::size_t> FindObjectAt(float x, float y) const
	{
		for (std::size_t i = map.objects.size(); i > 0; --i)
		{
			const MapObject& object = map.objects[i - 1];
			if (std::abs(object.x - x) <= 32.0f && std::abs(object.y - y) <= 32.0f)
				return i - 1;
		}
		return std::nullopt;
	}

	void RemoveObject(std::size_t index)
	{
		map.objects.erase(map.objects.begin() + index);
		dirty = true;
	}

	bool HasUsedTriggerId(const std::string& id) const
	{
		return FindTrigger(map.triggers, id) != map.triggers.end() || FindTrigger(originalTriggers, id) != originalTriggers.end();
	}

	std::optional<std::size_t> FindTriggerAt(float x, float y) const
	{
		for (std::size_t i = map.triggers.size(); i > 0; --i)
		{
			const auto& trigger = map.triggers[i - 1];
			if (x >= trigger.x && x <= trigger.x + trigger.width && y >= trigger.y && y <= trigger.y + trigger.height)
				return i - 1;
		}
		return std::nullopt;
	}

	bool IsCurrent(std::string& error) const
	{
		std::string contents;
		if (!existsOnDisk || !Read(path, contents, error)) return false;
		if (contents != original)
		{
			error = "Die Kartendatei wurde extern geaendert. Bitte erneut oeffnen.";
			return false;
		}
		return true;
	}

	bool Load(const std::filesystem::path& file, std::string& error)
	{
		std::string contents;
		if (!Read(file, contents, error)) return false;
		MapData loaded;
		if (!TryLoadMapFromFile(file.string(), loaded))
		{
			error = "Die Karte konnte nicht geladen werden: " + file.string();
			return false;
		}
		std::vector<Line> lines;
		std::set<std::string> ids, triggerIds;
		std::istringstream input(contents);
		std::string text;
		std::size_t objects = 0, triggers = 0;
		while (std::getline(input, text))
		{
			const bool cr = !text.empty() && text.back() == '\r';
			if (cr) text.pop_back();
			std::istringstream fields(text);
			std::string keyword;
			fields >> keyword;
			Line line{ text, cr ? "\r\n" : "\n", {} };
			if (keyword == "OBJECT")
			{
				std::string id, type, sprite, solid, extra;
				float x, y;
				if (!(fields >> id >> type >> x >> y >> sprite >> solid) || (fields >> extra) ||
					!std::isfinite(x) || !std::isfinite(y) ||
					(type != "scenery" && type != "untergrund" && type != "wall" && type != "npc") ||
					(solid != "solid=0" && solid != "solid=1") || !ids.insert(id).second)
				{
					error = "Ungueltige oder nicht unterstuetzte OBJECT-Zeile; die Karte wird nicht bearbeitet.";
					return false;
				}
				line.objectId = id;
				++objects;
			}
			else if (keyword == "TRIGGER")
			{
				std::string id, event;
				float x, y, width, height;
				if (!(fields >> id >> x >> y >> width >> height >> event) ||
					!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) ||
					!triggerIds.insert(id).second)
				{
					error = "Ungueltige TRIGGER-Zeile oder doppelte Trigger-ID; die Karte wird nicht bearbeitet.";
					return false;
				}
				line.triggerId = id;
				++triggers;
			}
			lines.push_back(std::move(line));
		}
		if (!contents.empty() && contents.back() != '\n' && !lines.empty()) lines.back().ending.clear();
		if (objects != loaded.objects.size() || triggers != loaded.triggers.size())
		{
			error = "Objekte oder Trigger konnten nicht vollstaendig geladen werden.";
			return false;
		}
		map = std::move(loaded);
		path = file;
		original = std::move(contents);
		sourceLines = std::move(lines);
		originalObjects = map.objects;
		originalTriggers = map.triggers;
		originalGround = map.defaultGroundPath;
		originalWeather = map.defaultWeather;
		existsOnDisk = true;
		dirty = false;
		return true;
	}

	void New(const std::filesystem::path& file, const std::string& id)
	{
		map = MapData{};
		map.id = id;
		path = file;
		original.clear();
		sourceLines.clear();
		originalObjects.clear();
		originalTriggers.clear();
		originalGround.clear();
		originalWeather = "none";
		existsOnDisk = false;
		dirty = true;
	}

	bool Save(std::string& error)
	{
		if (path.empty() || map.id.empty() || map.defaultGroundPath.empty())
		{
			error = "Bitte zuerst eine Karte und einen Standardboden auswaehlen.";
			return false;
		}
		std::string current;
		std::error_code ec;
		const bool exists = std::filesystem::exists(path, ec);
		if (ec) { error = "Kartendatei kann nicht geprueft werden: " + ec.message(); return false; }
		if (exists != existsOnDisk || (exists && (!Read(path, current, error) || current != original)))
		{
			if (error.empty()) error = "Die Kartendatei wurde extern geaendert. Bitte erneut oeffnen.";
			return false;
		}
		std::string output;
		std::set<std::string> emitted, emittedTriggers;
		bool groundWritten = false, weatherWritten = false;
		const std::string ending = sourceLines.empty() ? "\n" : (sourceLines.front().ending == "\r\n" ? "\r\n" : "\n");
		auto append = [&](const std::string& text) {
			if (!output.empty() && output.back() != '\n') output += ending;
			output += text + ending;
		};
		if (sourceLines.empty())
		{
			append("MAP " + map.id);
			append("GROUND " + map.defaultGroundPath);
			append("WEATHER " + map.defaultWeather);
			groundWritten = weatherWritten = true;
		}
		for (const Line& line : sourceLines)
		{
			if (!line.objectId.empty())
			{
				const auto item = Find(map.objects, line.objectId);
				if (item == map.objects.end()) continue;
				emitted.insert(item->id);
				const auto previous = Find(originalObjects, line.objectId);
				output += previous != originalObjects.end() && Equal(*item, *previous) ? line.text : Format(*item);
				output += line.ending;
				continue;
			}
			if (!line.triggerId.empty())
			{
				const auto item = FindTrigger(map.triggers, line.triggerId);
				if (item == map.triggers.end()) continue;
				emittedTriggers.insert(item->id);
				const auto previous = FindTrigger(originalTriggers, line.triggerId);
				output += previous != originalTriggers.end() ? UpdateTriggerLine(line.text, *previous, *item) : FormatTrigger(*item);
				output += line.ending;
				continue;
			}
			std::istringstream fields(line.text);
			std::string keyword;
			fields >> keyword;
			if ((keyword == "GROUND" && map.defaultGroundPath != originalGround) ||
				(keyword == "WEATHER" && map.defaultWeather != originalWeather))
			{
				const std::size_t keywordStart = line.text.find_first_not_of(" \t");
				const std::size_t valueStart = line.text.find_first_not_of(" \t", keywordStart + keyword.size());
				if (valueStart == std::string::npos)
				{
					error = "Eine Kartenzeile ohne Wert kann nicht sicher bearbeitet werden.";
					return false;
				}
				const std::size_t valueEnd = line.text.find_first_of(" \t", valueStart);
				output += line.text.substr(0, valueStart) + (keyword == "GROUND" ? map.defaultGroundPath : map.defaultWeather);
				if (valueEnd != std::string::npos) output += line.text.substr(valueEnd);
				output += line.ending;
				if (keyword == "GROUND") groundWritten = true;
				else weatherWritten = true;
			}
			else
			{
				output += line.text + line.ending;
				if (keyword == "GROUND") groundWritten = true;
				if (keyword == "WEATHER") weatherWritten = true;
			}
		}
		if (!groundWritten) append("GROUND " + map.defaultGroundPath);
		if (!weatherWritten && map.defaultWeather != "none") append("WEATHER " + map.defaultWeather);
		for (const MapObject& item : map.objects)
			if (!emitted.contains(item.id)) append(Format(item));
		for (const TriggerZone& item : map.triggers)
			if (!emittedTriggers.contains(item.id)) append(FormatTrigger(item));

		if (exists)
		{
			const std::filesystem::path backup = path.wstring() + L".original.bak";
			if (!CopyFileW(path.c_str(), backup.c_str(), TRUE))
			{
				const DWORD code = GetLastError();
				if (code != ERROR_FILE_EXISTS && code != ERROR_ALREADY_EXISTS)
				{
					error = "Originalsicherung fehlgeschlagen (Windows-Fehler " + std::to_string(code) + ").";
					return false;
				}
				if (!std::filesystem::is_regular_file(backup, ec) || ec)
				{
					error = "Die vorhandene Originalsicherung ist keine lesbare Datei.";
					return false;
				}
			}
		}
		wchar_t temporary[MAX_PATH];
		if (!GetTempFileNameW(path.parent_path().c_str(), L"map", 0, temporary))
		{
			error = "Temporaere Kartendatei konnte nicht angelegt werden (Windows-Fehler " + std::to_string(GetLastError()) + ").";
			return false;
		}
		std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
		file.write(output.data(), static_cast<std::streamsize>(output.size()));
		file.close();
		const DWORD flags = MOVEFILE_WRITE_THROUGH | (exists ? MOVEFILE_REPLACE_EXISTING : 0);
		if (!file || !MoveFileExW(temporary, path.c_str(), flags))
		{
			const DWORD code = GetLastError();
			DeleteFileW(temporary);
			error = "Karte konnte nicht gespeichert werden (Windows-Fehler " + std::to_string(code) + ").";
			return false;
		}
		if (!Load(path, error)) return false;
		return true;
	}

private:
	struct Line { std::string text, ending, objectId, triggerId; };
	std::string original, originalGround, originalWeather;
	std::vector<Line> sourceLines;
	std::vector<MapObject> originalObjects;
	std::vector<TriggerZone> originalTriggers;
	bool existsOnDisk = false;

	static bool Read(const std::filesystem::path& file, std::string& result, std::string& error)
	{
		std::ifstream input(file, std::ios::binary);
		if (!input) { error = "Datei konnte nicht geoeffnet werden: " + file.string(); return false; }
		result.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		if (input.bad()) { error = "Datei konnte nicht vollstaendig gelesen werden: " + file.string(); return false; }
		return true;
	}
	static std::vector<MapObject>::const_iterator Find(const std::vector<MapObject>& objects, const std::string& id)
	{
		return std::find_if(objects.begin(), objects.end(), [&](const MapObject& item) { return item.id == id; });
	}
	static std::vector<TriggerZone>::const_iterator FindTrigger(const std::vector<TriggerZone>& triggers, const std::string& id)
	{
		return std::find_if(triggers.begin(), triggers.end(), [&](const TriggerZone& trigger) { return trigger.id == id; });
	}
	static std::string Number(float value)
	{
		std::ostringstream stream;
		stream << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
		return stream.str();
	}
	static std::string FormatTrigger(const TriggerZone& trigger)
	{
		return "TRIGGER " + trigger.id + " " + Number(trigger.x) + " " + Number(trigger.y) + " " +
			Number(trigger.width) + " " + Number(trigger.height) + " " + trigger.eventName +
			" glow=" + (trigger.glow ? "1" : "0") + " spawn_x=" + Number(trigger.spawnX) + " spawn_y=" + Number(trigger.spawnY);
	}
	static std::string UpdateTriggerLine(const std::string& text, const TriggerZone& previous, const TriggerZone& trigger)
	{
		std::string result;
		std::size_t pos = 0, index = 0;
		bool glowFound = false;
		while (pos < text.size())
		{
			const auto start = text.find_first_not_of(" \t\r\n", pos);
			if (start == std::string::npos) { result += text.substr(pos); break; }
			result += text.substr(pos, start - pos);
			const auto end = text.find_first_of(" \t\r\n", start);
			const auto length = end == std::string::npos ? text.size() - start : end - start;
			const std::string token = text.substr(start, length);
			if (index == 2 && trigger.x != previous.x) result += Number(trigger.x);
			else if (index == 3 && trigger.y != previous.y) result += Number(trigger.y);
			else if (index == 4 && trigger.width != previous.width) result += Number(trigger.width);
			else if (index == 5 && trigger.height != previous.height) result += Number(trigger.height);
			else if (index == 6 && trigger.eventName != previous.eventName) result += trigger.eventName;
			else if (index >= 7 && token.starts_with("glow=")) { glowFound = true; result += trigger.glow != previous.glow ? (trigger.glow ? "glow=1" : "glow=0") : token; }
			else result += token;
			pos = start + length;
			++index;
		}
		if (!glowFound && trigger.glow != previous.glow) result += trigger.glow ? " glow=1" : " glow=0";
		return result;
	}
	static bool Equal(const MapObject& a, const MapObject& b)
	{
		return a.id == b.id && a.type == b.type && a.x == b.x && a.y == b.y && a.spritePath == b.spritePath && a.solid == b.solid;
	}
	static std::string Format(const MapObject& object)
	{
		const char* type = object.type == MapObjectType::Untergrund ? "untergrund" : object.type == MapObjectType::Wall ? "wall" : object.type == MapObjectType::Npc ? "npc" : "scenery";
		std::ostringstream stream;
		stream << "OBJECT " << object.id << ' ' << type << ' ' << std::setprecision(std::numeric_limits<float>::max_digits10)
			   << object.x << ' ' << object.y << ' ' << object.spritePath << " solid=" << (object.solid ? 1 : 0);
		return stream.str();
	}
};
#endif
