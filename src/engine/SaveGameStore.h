#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "UserDataFiles.h"

struct SaveGamePosition
{
	std::string mapFile;
	float x = 0.0f;
	float y = 0.0f;
	std::string name;
	std::optional<double> playtimeSeconds;
	bool playtimeComplete = false;
};

struct SaveGameSlot
{
	using Id = std::uint64_t;
	static constexpr Id LegacyId = 0;

	Id id;
	std::string mapFile;
	std::string name;
	std::optional<double> playtimeSeconds;
	bool playtimeComplete = false;

	std::string PlaytimeLabel() const
	{
		if (!playtimeSeconds) return "Spielzeit unbekannt";
		const auto seconds = static_cast<std::uint64_t>(*playtimeSeconds);
		const auto twoDigits = [](std::uint64_t value) {
			return (value < 10 ? "0" : "") + std::to_string(value);
		};
		return twoDigits(seconds / 3600) + ":" +
			twoDigits((seconds / 60) % 60) + ":" + twoDigits(seconds % 60);
	}

	std::string Label() const
	{
		if (!name.empty()) return name + " (#" + std::to_string(id) + ")";
		return id == LegacyId ? "Bisheriger Spielstand" : "Spielstand " + std::to_string(id);
	}
};

class SaveGameStore
{
public:
	bool List(std::vector<SaveGameSlot>& slots, std::string& error) const
	{
		std::vector<SaveGameSlot::Id> ids;
		if (!ListIds(ids, error)) return false;

		std::sort(ids.rbegin(), ids.rend());
		std::vector<SaveGameSlot> result;
		result.reserve(ids.size());
		for (const auto id : ids)
		{
			SaveGamePosition position;
			std::string loadError;
			// Unreadable files remain visible and can be deleted explicitly.
			if (!Load(id, position, loadError)) position = {};
			result.push_back({ id, std::move(position.mapFile), std::move(position.name),
				position.playtimeSeconds, position.playtimeComplete });
		}
		slots = std::move(result);
		return true;
	}

	bool Load(SaveGameSlot::Id id, SaveGamePosition& position, std::string& error) const
	{
		std::filesystem::path path;
		if (!ResolveSlot(id, path, error)) return false;
		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			error = "Der Spielstand konnte nicht geoeffnet werden.";
			return false;
		}

		const auto document = nlohmann::json::parse(input, nullptr, false);
		SaveGamePosition loaded;
		if (input.bad() || !document.is_object() || !document.contains("version") ||
			!document["version"].is_number_integer() || document["version"] != Version ||
			!document.contains("map") || !document["map"].is_string() ||
			!document.contains("x") || !ReadCoordinate(document["x"], loaded.x) ||
			!document.contains("y") || !ReadCoordinate(document["y"], loaded.y) ||
			(document.contains("name") && !document["name"].is_string()))
		{
			error = "Der Spielstand ist beschaedigt oder hat ein nicht unterstuetztes Format.";
			return false;
		}
		if (document.contains("playtime_seconds"))
		{
			const auto& field = document["playtime_seconds"];
			if (!field.is_number() || !std::isfinite(field.get<double>()) ||
				field.get<double>() < 0.0 || field.get<double>() > MaxPlaytimeSeconds ||
				(document.contains("playtime_complete") && !document["playtime_complete"].is_boolean()))
			{
				error = "Der Spielstand enthaelt eine ungueltige Spielzeit.";
				return false;
			}
			loaded.playtimeSeconds = field.get<double>();
			loaded.playtimeComplete = document.value("playtime_complete", true);
		}
		else if (document.contains("playtime_complete"))
		{
			error = "Der Spielstand enthaelt eine ungueltige Spielzeit.";
			return false;
		}
		loaded.mapFile = document["map"].get<std::string>();
		if (document.contains("name")) loaded.name = document["name"].get<std::string>();
		if (!IsMapFileName(loaded.mapFile))
		{
			error = "Der Spielstand enthaelt einen ungueltigen Kartendateinamen.";
			return false;
		}
		position = std::move(loaded);
		return true;
	}

	bool Create(const SaveGamePosition& position, SaveGameSlot::Id& id, std::string& error) const
	{
		std::vector<SaveGameSlot::Id> ids;
		if (!ListIds(ids, error)) return false;
		const auto last = ids.empty() ? SaveGameSlot::LegacyId : *std::max_element(ids.begin(), ids.end());
		if (last == (std::numeric_limits<SaveGameSlot::Id>::max)())
		{
			error = "Es kann kein weiterer Spielstand angelegt werden.";
			return false;
		}
		const auto next = last + 1;
		if (!Write(next, position, false, error)) return false;
		id = next;
		return true;
	}

	bool Save(SaveGameSlot::Id id, const SaveGamePosition& position, std::string& error) const
	{
		return Write(id, position, true, error);
	}

	bool Delete(SaveGameSlot::Id id, std::string& error) const
	{
		std::filesystem::path path;
		if (!ResolveSlot(id, path, error)) return false;
		if (!DeleteFileW(path.c_str()))
		{
			const DWORD code = GetLastError();
			error = "Der Spielstand konnte nicht geloescht werden (Windows-Fehler " + std::to_string(code) + ").";
			return false;
		}
		return true;
	}

private:
	bool Write(SaveGameSlot::Id id, const SaveGamePosition& position, bool replaceExisting, std::string& error) const
	{
		if (!IsMapFileName(position.mapFile) || !std::isfinite(position.x) || !std::isfinite(position.y) ||
			(position.playtimeSeconds && (!std::isfinite(*position.playtimeSeconds) ||
				*position.playtimeSeconds < 0.0 || *position.playtimeSeconds > MaxPlaytimeSeconds)))
		{
			error = "Die aktuelle Karte oder Spielerposition kann nicht gespeichert werden.";
			return false;
		}
		std::filesystem::path path;
		if (!ResolveSlot(id, path, error)) return false;
		nlohmann::json document{ { "version", Version }, { "map", position.mapFile },
			{ "x", position.x }, { "y", position.y } };
		if (!position.name.empty()) document["name"] = position.name;
		if (position.playtimeSeconds)
		{
			document["playtime_seconds"] = *position.playtimeSeconds;
			document["playtime_complete"] = position.playtimeComplete;
		}
		return UserDataFiles::WriteAtomically(path, document.dump(2) + '\n', replaceExisting, error);
	}

	static constexpr const wchar_t* LegacyFileName = L"savegame.json";
	static constexpr const wchar_t* SlotDirectory = L"Saves";
	static constexpr std::wstring_view Prefix = L"save_";
	static constexpr std::wstring_view Extension = L".json";
	static constexpr int Version = 1;
	static constexpr double MaxPlaytimeSeconds = 1000000000000.0;

	static bool ResolveSlot(SaveGameSlot::Id id, std::filesystem::path& path, std::string& error)
	{
		if (id == SaveGameSlot::LegacyId) return UserDataFiles::Resolve(LegacyFileName, path, error);
		if (!UserDataFiles::Resolve(SlotDirectory, path, error)) return false;
		path /= std::wstring(Prefix) + std::to_wstring(id) + std::wstring(Extension);
		return true;
	}

	static std::optional<SaveGameSlot::Id> ParseSlotName(std::wstring_view name)
	{
		if (!name.starts_with(Prefix) || !name.ends_with(Extension)) return std::nullopt;
		name.remove_prefix(Prefix.size());
		name.remove_suffix(Extension.size());
		if (name.empty() || name.front() == L'0') return std::nullopt;
		SaveGameSlot::Id id = 0;
		for (const wchar_t ch : name)
		{
			if (ch < L'0' || ch > L'9') return std::nullopt;
			const auto digit = static_cast<SaveGameSlot::Id>(ch - L'0');
			if (id > ((std::numeric_limits<SaveGameSlot::Id>::max)() - digit) / 10) return std::nullopt;
			id = id * 10 + digit;
		}
		return id;
	}

	static bool ListIds(std::vector<SaveGameSlot::Id>& ids, std::string& error)
	{
		std::filesystem::path legacyPath, directory;
		if (!ResolveSlot(SaveGameSlot::LegacyId, legacyPath, error) ||
			!UserDataFiles::Resolve(SlotDirectory, directory, error)) return false;

		std::error_code ec;
		if (std::filesystem::exists(legacyPath, ec)) ids.push_back(SaveGameSlot::LegacyId);
		if (!ec && std::filesystem::exists(directory, ec))
		{
			for (std::filesystem::directory_iterator entry(directory, ec), end; !ec && entry != end; entry.increment(ec))
			{
				const auto id = ParseSlotName(entry->path().filename().native());
				if (id && entry->is_regular_file(ec)) ids.push_back(*id);
				if (ec) break;
			}
		}
		if (ec)
		{
			error = "Die Spielstandliste konnte nicht gelesen werden: " + ec.message();
			return false;
		}
		return true;
	}

	static bool IsMapFileName(const std::string& name)
	{
		return name.size() > 4 && name.ends_with(".map") &&
			name.find_first_of("\\/:*?\"<>|") == std::string::npos &&
			std::none_of(name.begin(), name.end(), [](unsigned char ch) { return ch < 32 || ch >= 127; });
	}

	static bool ReadCoordinate(const nlohmann::json& field, float& coordinate)
	{
		if (!field.is_number()) return false;
		const double value = field.get<double>();
		if (!std::isfinite(value) || std::abs(value) > (std::numeric_limits<float>::max)()) return false;
		coordinate = static_cast<float>(value);
		return true;
	}
};
