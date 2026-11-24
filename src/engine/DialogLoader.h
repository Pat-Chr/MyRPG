#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>
#include <SFML/System/String.hpp>

inline bool LoadNpcGreeting(const std::string& name, std::vector<std::string>& paragraphs, std::string& error)
{
	paragraphs = { "..." };
	error.clear();
	if (name.empty()) return true;

	const std::wstring filename = sf::String::fromUtf8(name.begin(), name.end()).toWideString();
	if (filename.find_first_of(L"<>:\"/\\|?*") != std::wstring::npos ||
		filename.find_first_of(L"\r\n\t") != std::wstring::npos || filename == L"." || filename == L"..")
	{
		error = "Der NPC-Name ist kein gueltiger Dialog-Dateiname.";
		return false;
	}

	const auto path = std::filesystem::path(L"Assets\\Dialogs") / (filename + L".json");
	std::error_code ec;
	const auto status = std::filesystem::status(path, ec);
	if (ec == std::errc::no_such_file_or_directory || (!ec && !std::filesystem::exists(status))) return true;
	if (ec)
	{
		error = "Die Dialogdatei konnte nicht geprueft werden: " + ec.message();
		return false;
	}

	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		error = "Die Dialogdatei konnte nicht geoeffnet werden.";
		return false;
	}
	const auto document = nlohmann::json::parse(input, nullptr, false);
	if (input.bad() || document.is_discarded() || !document.is_object() ||
		!document.contains("greeting") || !document["greeting"].is_array() || document["greeting"].empty())
	{
		error = "Die Dialogdatei muss ein nicht leeres JSON-Array 'greeting' enthalten.";
		return false;
	}

	std::vector<std::string> loaded;
	for (const auto& entry : document["greeting"])
	{
		if (!entry.is_string() || entry.get_ref<const std::string&>().empty())
		{
			error = "Jeder Begruessungsabschnitt muss ein nicht leerer Text sein.";
			return false;
		}
		loaded.push_back(entry.get<std::string>());
	}
	paragraphs = std::move(loaded);
	return true;
}
