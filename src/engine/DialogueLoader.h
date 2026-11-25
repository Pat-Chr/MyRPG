#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <SFML/System/String.hpp>

inline bool ResolveDialoguePath(const std::string& name, std::filesystem::path& path, std::string& error)
{
	error.clear();
	if (name.empty())
	{
		error = "Der Dialogue-Dateiname darf nicht leer sein.";
		return false;
	}
	const std::wstring filename = sf::String::fromUtf8(name.begin(), name.end()).toWideString();
	if (filename.find_first_of(L"<>:\"/\\|?*") != std::wstring::npos ||
		filename.find_first_of(L"\r\n\t") != std::wstring::npos || filename == L"." || filename == L"..")
	{
		error = "Der Name ist kein gueltiger Dialogue-Dateiname.";
		return false;
	}
	path = std::filesystem::path(L"Assets\\Dialogues") / (filename + L".json");
	return true;
}

inline bool LoadDialogueParagraphs(const std::string& name, const std::string& section, bool allowMissing,
	std::vector<std::string>& paragraphs, std::string& error)
{
	paragraphs.clear();
	std::filesystem::path path;
	if (!ResolveDialoguePath(name, path, error)) return false;
	std::error_code ec;
	const auto status = std::filesystem::status(path, ec);
	if (ec == std::errc::no_such_file_or_directory || (!ec && !std::filesystem::exists(status)))
	{
		if (allowMissing) { paragraphs = { "..." }; return true; }
		error = "Die Dialoguedatei fehlt: " + path.string();
		return false;
	}
	if (ec)
	{
		error = "Die Dialoguedatei konnte nicht geprueft werden: " + ec.message();
		return false;
	}

	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		error = "Die Dialoguedatei konnte nicht geoeffnet werden.";
		return false;
	}
	const auto document = nlohmann::json::parse(input, nullptr, false);
	if (input.bad() || document.is_discarded() || !document.is_object() ||
		!document.contains(section) || !document[section].is_array() || document[section].empty())
	{
		error = "Die Dialoguedatei muss ein nicht leeres JSON-Array '" + section + "' enthalten.";
		return false;
	}

	std::vector<std::string> loaded;
	for (const auto& entry : document[section])
	{
		if (!entry.is_string() || entry.get_ref<const std::string&>().empty())
		{
			error = "Jeder Dialogueabschnitt muss ein nicht leerer Text sein.";
			return false;
		}
		loaded.push_back(entry.get<std::string>());
	}
	paragraphs = std::move(loaded);
	return true;
}

inline bool LoadNpcGreeting(const std::string& name, std::vector<std::string>& paragraphs, std::string& error)
{
	if (name.empty()) { paragraphs = { "..." }; error.clear(); return true; }
	return LoadDialogueParagraphs(name, "greeting", true, paragraphs, error);
}

inline bool LoadTriggerMonologue(const std::string& name, std::vector<std::string>& paragraphs, std::string& error)
{
	return LoadDialogueParagraphs(name, "monologue", false, paragraphs, error);
}
