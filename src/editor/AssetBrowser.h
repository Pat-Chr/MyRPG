#pragma once
#include "../BuildConfig.h"
#ifdef MYRPG_MAP_EDITOR
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <set>
#include <string>
#include <system_error>
#include <vector>

class AssetBrowser
{
public:
	struct Entry { std::string path; std::string label; int depth; bool folder; };
	std::vector<Entry> visible;
	std::string selected;
	int offset = 0;

	bool Refresh(const std::filesystem::path& root, std::string& error)
	{
		std::vector<Entry> entries;
		std::error_code ec;
		const auto sprites = root / L"Assets" / L"Sprites";
		std::filesystem::recursive_directory_iterator it(sprites, ec), end;
		if (ec) { error = "Sprite-Ordner konnte nicht gelesen werden: " + ec.message(); return false; }
		while (it != end)
		{
			const auto path = it->path();
			const bool folder = it->is_directory(ec);
			if (ec) { error = "Asset konnte nicht geprueft werden: " + ec.message(); return false; }
			const bool regular = !folder && it->is_regular_file(ec);
			if (ec) { error = "Asset konnte nicht geprueft werden: " + ec.message(); return false; }
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (folder || (regular && extension == ".png"))
			{
				const std::string relative = path.lexically_relative(root).generic_string();
				if (relative.find_first_of(" \t\r\n") == std::string::npos)
				{
					const auto local = path.lexically_relative(sprites);
					const int depth = static_cast<int>(std::distance(local.begin(), local.end())) - 1;
					entries.push_back({ relative, path.filename().string(), depth, folder });
				}
				else std::fprintf(stderr, "Map-Editor: Asset-Pfad enthaelt Leerzeichen: %s\n", relative.c_str());
			}
			it.increment(ec);
			if (ec) { error = "Sprite-Ordner konnte nicht vollstaendig gelesen werden: " + ec.message(); return false; }
		}
		std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.path < b.path; });
		all = std::move(entries);
		if (!initialized)
		{
			for (const auto& entry : all)
				if (entry.folder && (entry.path == "Assets/Sprites/Environment" || entry.path.starts_with("Assets/Sprites/Environment/"))) expanded.insert(entry.path);
			initialized = true;
		}
		if (!selected.empty() && std::none_of(all.begin(), all.end(), [&](const Entry& entry) { return !entry.folder && entry.path == selected; })) selected.clear();
		Rebuild();
		if (selected.empty())
			for (const auto& entry : visible) if (!entry.folder) { selected = entry.path; break; }
		return true;
	}
	void Toggle(const std::string& path)
	{
		if (expanded.contains(path)) expanded.erase(path); else expanded.insert(path);
		Rebuild();
	}
	bool IsExpanded(const std::string& path) const { return expanded.contains(path); }
private:
	std::vector<Entry> all;
	std::set<std::string> expanded;
	bool initialized = false;
	void Rebuild()
	{
		visible.clear();
		for (const auto& entry : all)
		{
			std::filesystem::path parent = std::filesystem::path(entry.path).parent_path();
			bool show = true;
			while (parent.generic_string() != "Assets/Sprites" && !parent.empty())
			{
				if (!expanded.contains(parent.generic_string())) { show = false; break; }
				parent = parent.parent_path();
			}
			if (show) visible.push_back(entry);
		}
		offset = std::clamp(offset, 0, (std::max)(0, static_cast<int>(visible.size()) - 17));
	}
};
#endif
