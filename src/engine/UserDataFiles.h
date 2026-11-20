#pragma once

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace UserDataFiles
{
	inline bool Resolve(const wchar_t* name, std::filesystem::path& path, std::string& error)
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
		path = std::filesystem::path(buffer.data()) / L"MyRPG" / name;
		return true;
	}

	// Schreibt zuerst in eine temporaere Datei und ersetzt das Ziel erst nach erfolgreichem Flush.
	// Damit bleibt ein bestehender Spielstand oder eine bestehende Einstellung bei Schreibfehlern erhalten.
	inline bool WriteAtomically(const std::filesystem::path& path, std::string_view contents,
		bool replaceExisting, std::string& error)
	{
		const auto directory = path.parent_path();
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		if (ec)
		{
			error = "Das Datenverzeichnis konnte nicht erstellt werden: " + ec.message();
			return false;
		}

		wchar_t temporaryName[MAX_PATH];
		if (!GetTempFileNameW(directory.c_str(), L"rpg", 0, temporaryName))
		{
			error = "Eine temporaere Datei konnte nicht erstellt werden (Windows-Fehler " + std::to_string(GetLastError()) + ").";
			return false;
		}
		const std::filesystem::path temporaryPath(temporaryName);
		std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
		output << contents;
		output.close();
		if (!output)
		{
			std::filesystem::remove(temporaryPath, ec);
			error = "Die Datei konnte nicht vollstaendig geschrieben werden.";
			return false;
		}
		const DWORD flags = MOVEFILE_WRITE_THROUGH | (replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0);
		if (!MoveFileExW(temporaryPath.c_str(), path.c_str(), flags))
		{
			const DWORD code = GetLastError();
			std::filesystem::remove(temporaryPath, ec);
			error = "Die Datei konnte nicht gespeichert werden (Windows-Fehler " + std::to_string(code) + ").";
			return false;
		}
		return true;
	}
}
