#pragma once

#include <Windows.h>
#include <SFML/Graphics.hpp>

#include <cstdio>
#include <filesystem>
#include <iterator>
#include <string>

#include "SettingsStore.h"

class SettingsMenu
{
public:
	void ApplySettings(const GameSettings& settings, sf::RenderWindow& window)
	{
		verticalSync = settings.verticalSync;
		const auto found = std::find(GameSettings::fpsLimits.begin(), GameSettings::fpsLimits.end(), settings.fpsLimit);
		fpsLimitIndex = static_cast<std::size_t>(std::distance(GameSettings::fpsLimits.begin(), found));
		window.setVerticalSyncEnabled(verticalSync);
		window.setFramerateLimit(verticalSync ? 0u : settings.fpsLimit);
	}

	GameSettings GetSettings() const
	{
		return { verticalSync, GameSettings::fpsLimits[fpsLimitIndex] };
	}

	bool Open()
	{
		if (!fontLoaded)
		{
			wchar_t windowsDirectory[MAX_PATH];
			const UINT length = GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
			if (length == 0 || length >= MAX_PATH)
			{
				std::fprintf(stderr, "Failed to locate the Windows font directory.\n");
				return false;
			}

			const std::filesystem::path fontDirectory = std::filesystem::path(windowsDirectory) / L"Fonts";
			fontLoaded = font.openFromFile(fontDirectory / L"arial.ttf") ||
						 font.openFromFile(fontDirectory / L"consola.ttf");
			if (!fontLoaded)
			{
				std::fprintf(stderr, "Failed to load a font for the settings menu.\n");
				return false;
			}
		}

		open = true;
		return true;
	}

	void Close() { open = false; }
	bool IsOpen() const { return open; }

	bool HandleKey(sf::Keyboard::Key key, sf::RenderWindow& window)
	{
		if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Down)
		{
			if (!verticalSync)
			{
				fpsLimitSelected = !fpsLimitSelected;
			}
		}
		else if (key == sf::Keyboard::Key::Left || key == sf::Keyboard::Key::Right ||
			key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
		{
			if (fpsLimitSelected)
			{
				const std::size_t step = key == sf::Keyboard::Key::Left ? GameSettings::fpsLimits.size() - 1 : 1;
				fpsLimitIndex = (fpsLimitIndex + step) % GameSettings::fpsLimits.size();
			}
			else
			{
				verticalSync = !verticalSync;
				window.setVerticalSyncEnabled(verticalSync);
			}
			window.setFramerateLimit(verticalSync ? 0u : GameSettings::fpsLimits[fpsLimitIndex]);
			return true;
		}
		return false;
	}

	void Draw(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const float width = static_cast<float>(resolution.x);
		const float height = static_cast<float>(resolution.y);

		sf::RectangleShape overlay({ width, height });
		overlay.setFillColor(sf::Color(0, 0, 0, 170));
		target.draw(overlay);

		sf::RectangleShape panel({ 540.0f, 330.0f });
		panel.setOrigin({ 270.0f, 165.0f });
		panel.setPosition({ width * 0.5f, height * 0.5f });
		panel.setFillColor(sf::Color(35, 40, 35));
		panel.setOutlineThickness(2.0f);
		panel.setOutlineColor(sf::Color(190, 170, 120));
		target.draw(panel);

		sf::Text title(font, "Einstellungen", 32);
		title.setPosition({ width * 0.5f - 230.0f, height * 0.5f - 135.0f });
		target.draw(title);

		sf::Text syncSetting(font, verticalSync ? "VSync: An" : "VSync: Aus", 24);
		syncSetting.setFillColor(fpsLimitSelected ? sf::Color::White : sf::Color(255, 220, 140));
		syncSetting.setPosition({ width * 0.5f - 230.0f, height * 0.5f - 75.0f });
		target.draw(syncSetting);

		const std::string fpsLabel = GameSettings::fpsLimits[fpsLimitIndex] == 0u
			? "FPS-Limit: Unbegrenzt" : "FPS-Limit: " + std::to_string(GameSettings::fpsLimits[fpsLimitIndex]);
		sf::Text fpsSetting(font, fpsLabel, 24);
		fpsSetting.setFillColor(verticalSync ? sf::Color(135, 135, 135) :
			(fpsLimitSelected ? sf::Color(255, 220, 140) : sf::Color::White));
		fpsSetting.setPosition({ width * 0.5f - 230.0f, height * 0.5f - 35.0f });
		target.draw(fpsSetting);

		sf::Text instructions(font, "FPS-Limit gilt bei VSync aus.\nHoch / Runter: Auswahl\nLinks / Rechts / Enter / Leertaste: Aendern\nEsc: Zurueck zum Spiel", 18);
		instructions.setPosition({ width * 0.5f - 230.0f, height * 0.5f + 20.0f });
		target.draw(instructions);
	}

private:
	sf::Font font;
	std::size_t fpsLimitIndex = GameSettings::fpsLimits.size() - 1;
	bool fontLoaded = false;
	bool open = false;
	bool verticalSync = false;
	bool fpsLimitSelected = false;
};
