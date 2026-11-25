#pragma once

#include "../BuildConfig.h"
#include "UserDataFiles.h"

#include <nlohmann/json.hpp>

#include <array>
#include <algorithm>

struct GameSettings
{
	inline static constexpr std::array<unsigned int, 4> fpsLimits{ 60u, 120u, 200u, 0u };
	bool verticalSync = false;
	unsigned int fpsLimit = 0u;
	bool showFps = false;
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
	bool mapEditorEnabled = false;
#endif
};

class SettingsStore
{
public:
	// A missing file leaves defaults unchanged; an invalid file does not enable saving.
	bool Load(GameSettings& settings, std::string& error)
	{
		if (!UserDataFiles::Resolve(L"settings.json", path, error))
		{
			return false;
		}

		std::error_code ec;
		const bool exists = std::filesystem::exists(path, ec);
		if (ec)
		{
			error = "Die Einstellungsdatei konnte nicht geprueft werden: " + ec.message();
			return false;
		}
		if (!exists)
		{
			writable = true;
			return true;
		}

		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			error = "Die Einstellungsdatei konnte nicht geoeffnet werden.";
			return false;
		}

		try
		{
			input >> document;
		}
		catch (const nlohmann::json::parse_error& exception)
		{
			error = std::string("Ungueltige Einstellungsdatei: ") + exception.what();
			return false;
		}
		if (input.bad())
		{
			error = "Die Einstellungsdatei konnte nicht vollstaendig gelesen werden.";
			return false;
		}
		if (!document.is_object() || !document.contains("version") ||
			!document["version"].is_number_integer() || document["version"] != 1 ||
			!document.contains("vsync") || !document["vsync"].is_boolean() ||
			!document.contains("fps_limit") || !document["fps_limit"].is_number_integer() ||
			(document.contains("show_fps") && !document["show_fps"].is_boolean())
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
			|| (document.contains("map_editor_enabled") && !document["map_editor_enabled"].is_boolean())
#endif
			)
		{
			error = "Die Einstellungsdatei hat ein ungueltiges oder nicht unterstuetztes Format.";
			return false;
		}
		const auto& fps = document["fps_limit"];
		const auto found = std::find_if(GameSettings::fpsLimits.begin(), GameSettings::fpsLimits.end(),
			[&fps](unsigned int value) { return fps == value; });
		if (found == GameSettings::fpsLimits.end())
		{
			error = "Die Einstellungsdatei enthaelt ein ungueltiges FPS-Limit.";
			return false;
		}
		settings.verticalSync = document["vsync"].get<bool>();
		settings.fpsLimit = *found;
		settings.showFps = document.contains("show_fps") ? document["show_fps"].get<bool>() : true;
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
		settings.mapEditorEnabled = document.contains("map_editor_enabled") ? document["map_editor_enabled"].get<bool>() : false;
#endif
		writable = true;
		return true;
	}

	bool Save(const GameSettings& settings, std::string& error)
	{
		if (!writable)
		{
			error = "Einstellungen werden nicht gespeichert, da die Einstellungsdatei nicht geladen werden konnte.";
			return false;
		}

		// Update only known fields so other JSON entries remain intact.
		document["version"] = 1;
		document["vsync"] = settings.verticalSync;
		document["fps_limit"] = settings.fpsLimit;
		document["show_fps"] = settings.showFps;
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
		document["map_editor_enabled"] = settings.mapEditorEnabled;
#endif

		return UserDataFiles::WriteAtomically(path, document.dump(2) + '\n', true, error);
	}

private:
	std::filesystem::path path;
	nlohmann::json document = nlohmann::json::object();
	bool writable = false;
};
