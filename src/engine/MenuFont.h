#pragma once

#include <Windows.h>
#include <SFML/Graphics.hpp>

#include <cstdio>
#include <filesystem>

// Menus use Windows system fonts (Arial, with Consolas as fallback) instead of a font asset.
inline bool LoadMenuFont(sf::Font& font)
{
	wchar_t windowsDirectory[MAX_PATH];
	const UINT length = GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
	if (length == 0 || length >= MAX_PATH)
	{
		std::fprintf(stderr, "Failed to locate the Windows font directory.\n");
		return false;
	}

	const std::filesystem::path fontDirectory = std::filesystem::path(windowsDirectory) / L"Fonts";
	if (!font.openFromFile(fontDirectory / L"arial.ttf") && !font.openFromFile(fontDirectory / L"consola.ttf"))
	{
		std::fprintf(stderr, "Failed to load a menu font.\n");
		return false;
	}
	return true;
}
