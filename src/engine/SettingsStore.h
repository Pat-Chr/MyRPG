#pragma once

#include <Windows.h>
#include <nlohmann/json.hpp>

#include <array>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

struct GameSettings
{
	inline static constexpr std::array<unsigned int, 4> fpsLimits{ 60u, 120u, 200u, 0u };
	bool verticalSync = false;
	unsigned int fpsLimit = 0u;
	bool showFps = false;
};

class SettingsStore
{
public:
	bool Load(GameSettings& settings, std::string& error)
	{
		const DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
		if (size == 0)
		{
			error = "LOCALAPPDATA konnte nicht ermittelt werden.";
			return false;
		}
		std::vector<wchar_t> buffer(size);
		const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(), size);
		if (length == 0 || length >= size)
		{
			error = "LOCALAPPDATA konnte nicht ermittelt werden.";
			return false;
		}
		path = std::filesystem::path(buffer.data()) / L"MyRPG" / L"settings.json";

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
			(document.contains("show_fps") && !document["show_fps"].is_boolean()))
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
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec)
		{
			error = "Das Einstellungsverzeichnis konnte nicht erstellt werden: " + ec.message();
			return false;
		}

		document["version"] = 1;
		document["vsync"] = settings.verticalSync;
		document["fps_limit"] = settings.fpsLimit;
		document["show_fps"] = settings.showFps;

		wchar_t temporaryName[MAX_PATH];
		if (!GetTempFileNameW(path.parent_path().c_str(), L"rpg", 0, temporaryName))
		{
			error = "Eine temporaere Einstellungsdatei konnte nicht erstellt werden (Windows-Fehler " + std::to_string(GetLastError()) + ").";
			return false;
		}
		const std::filesystem::path temporaryPath(temporaryName);
		std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
		output << document.dump(2) << '\n';
		output.close();
		if (!output)
		{
			std::filesystem::remove(temporaryPath, ec);
			error = "Die Einstellungsdatei konnte nicht geschrieben werden.";
			return false;
		}
		if (!MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			const DWORD code = GetLastError();
			std::filesystem::remove(temporaryPath, ec);
			error = "Die Einstellungsdatei konnte nicht ersetzt werden (Windows-Fehler " + std::to_string(code) + ").";
			return false;
		}
		return true;
	}

private:
	std::filesystem::path path;
	nlohmann::json document = nlohmann::json::object();
	bool writable = false;
};
