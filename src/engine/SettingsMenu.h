#pragma once

#include <Windows.h>
#include <SFML/Graphics.hpp>

#include <iterator>
#include <string>

#include "MenuFont.h"
#include "SettingsStore.h"

// Applies window options directly; the caller is responsible for saving them.
class SettingsMenu
{
public:
	enum class Action { None, SettingsChanged, SaveAndMainMenu };

	void ApplySettings(const GameSettings& settings, sf::RenderWindow& window)
	{
		verticalSync = settings.verticalSync;
		showFps = settings.showFps;
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
		mapEditorEnabled = settings.mapEditorEnabled;
#endif
		const auto found = std::find(GameSettings::fpsLimits.begin(), GameSettings::fpsLimits.end(), settings.fpsLimit);
		fpsLimitIndex = static_cast<std::size_t>(std::distance(GameSettings::fpsLimits.begin(), found));
		window.setVerticalSyncEnabled(verticalSync);
		// VSync disables only the active frame limit; the selected level remains saved.
		window.setFramerateLimit(verticalSync ? 0u : settings.fpsLimit);
	}

	GameSettings GetSettings() const
	{
		GameSettings settings{ verticalSync, GameSettings::fpsLimits[fpsLimitIndex], showFps };
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
		settings.mapEditorEnabled = mapEditorEnabled;
#endif
		return settings;
	}

	bool Open()
	{
		if (!fontLoaded)
		{
			fontLoaded = LoadMenuFont(font);
			if (!fontLoaded)
			{
				return false;
			}
		}

		open = true;
		return true;
	}

	void Close() { open = false; }
	bool IsOpen() const { return open; }

	Action HandleKey(sf::Keyboard::Key key, sf::RenderWindow& window)
	{
		if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Down)
		{
			if (key == sf::Keyboard::Key::Down)
			{
				selected = selected == Selection::VSync ? (verticalSync ? Selection::ShowFps : Selection::FpsLimit) :
					(selected == Selection::FpsLimit ? Selection::ShowFps :
					(selected == Selection::ShowFps ? Selection::MainMenu : Selection::VSync));
			}
			else
			{
				selected = selected == Selection::VSync ? Selection::MainMenu :
					(selected == Selection::MainMenu ? Selection::ShowFps :
					(selected == Selection::ShowFps ? (verticalSync ? Selection::VSync : Selection::FpsLimit) : Selection::VSync));
			}
		}
		else if (key == sf::Keyboard::Key::Left || key == sf::Keyboard::Key::Right ||
			key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
		{
			if (selected == Selection::MainMenu)
			{
				return (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space) ? Action::SaveAndMainMenu : Action::None;
			}
			if (selected == Selection::FpsLimit)
			{
				const std::size_t step = key == sf::Keyboard::Key::Left ? GameSettings::fpsLimits.size() - 1 : 1;
				fpsLimitIndex = (fpsLimitIndex + step) % GameSettings::fpsLimits.size();
				window.setFramerateLimit(GameSettings::fpsLimits[fpsLimitIndex]);
			}
			else if (selected == Selection::VSync)
			{
				verticalSync = !verticalSync;
				window.setVerticalSyncEnabled(verticalSync);
				window.setFramerateLimit(verticalSync ? 0u : GameSettings::fpsLimits[fpsLimitIndex]);
			}
			else
			{
				showFps = !showFps;
			}
			return Action::SettingsChanged;
		}
		return Action::None;
	}

	Action HandleClick(sf::Vector2f position, sf::Vector2u resolution, sf::RenderWindow& window)
	{
		if (MainMenuButtonBounds(resolution).contains(position))
		{
			return Action::SaveAndMainMenu;
		}

		for (int index = 0; index < SettingCount; ++index)
		{
			if (SettingBounds(index, resolution).contains(position))
			{
				if (index == 1 && verticalSync)
				{
					return Action::None;
				}
				selected = index == 0 ? Selection::VSync : (index == 1 ? Selection::FpsLimit : Selection::ShowFps);
				return HandleKey(sf::Keyboard::Key::Enter, window);
			}
		}
		return Action::None;
	}

	void Draw(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const float width = static_cast<float>(resolution.x);
		const float height = static_cast<float>(resolution.y);

		sf::RectangleShape overlay({ width, height });
		overlay.setFillColor(sf::Color(0, 0, 0, 170));
		target.draw(overlay);

		const float panelHeight = 390.0f;
		sf::RectangleShape panel({ 540.0f, panelHeight });
		panel.setOrigin({ 270.0f, panelHeight * 0.5f });
		panel.setPosition({ width * 0.5f, height * 0.5f });
		panel.setFillColor(sf::Color(35, 40, 35));
		panel.setOutlineThickness(2.0f);
		panel.setOutlineColor(sf::Color(190, 170, 120));
		target.draw(panel);

		sf::Text title(font, "Einstellungen", 32);
		title.setPosition({ width * 0.5f - 230.0f, height * 0.5f - 165.0f });
		target.draw(title);

		for (int index = 0; index < SettingCount; ++index)
		{
			const sf::FloatRect bounds = SettingBounds(index, resolution);
			sf::RectangleShape row(bounds.size);
			row.setPosition(bounds.position);
			row.setFillColor(sf::Color(50, 55, 45));
			target.draw(row);
		}

		sf::Text syncSetting(font, verticalSync ? "VSync: An" : "VSync: Aus", 24);
		syncSetting.setFillColor(selected == Selection::VSync ? sf::Color(255, 220, 140) : sf::Color::White);
		syncSetting.setPosition(SettingBounds(0, resolution).position);
		target.draw(syncSetting);

		const std::string fpsLabel = GameSettings::fpsLimits[fpsLimitIndex] == 0u
			? "FPS-Limit: Unbegrenzt" : "FPS-Limit: " + std::to_string(GameSettings::fpsLimits[fpsLimitIndex]);
		sf::Text fpsSetting(font, fpsLabel, 24);
		fpsSetting.setFillColor(verticalSync ? sf::Color(135, 135, 135) :
			(selected == Selection::FpsLimit ? sf::Color(255, 220, 140) : sf::Color::White));
		fpsSetting.setPosition(SettingBounds(1, resolution).position);
		target.draw(fpsSetting);

		sf::Text showFpsSetting(font, showFps ? "FPS-Anzeige: An" : "FPS-Anzeige: Aus", 24);
		showFpsSetting.setFillColor(selected == Selection::ShowFps ? sf::Color(255, 220, 140) : sf::Color::White);
		showFpsSetting.setPosition(SettingBounds(2, resolution).position);
		target.draw(showFpsSetting);

		const sf::FloatRect buttonBounds = MainMenuButtonBounds(resolution);
		sf::RectangleShape button(buttonBounds.size);
		button.setPosition(buttonBounds.position);
		button.setFillColor(sf::Color(50, 55, 45));
		button.setOutlineThickness(1.0f);
		button.setOutlineColor(selected == Selection::MainMenu ? sf::Color(255, 220, 140) : sf::Color(160, 150, 115));
		target.draw(button);

		sf::Text buttonText(font, sf::String(L"Speichern und zum Hauptmenü"), 22);
		buttonText.setFillColor(selected == Selection::MainMenu ? sf::Color(255, 220, 140) : sf::Color::White);
		buttonText.setPosition({ buttonBounds.position.x + 12.0f, buttonBounds.position.y + 7.0f });
		target.draw(buttonText);

		sf::Text instructions(font, "FPS-Limit gilt bei VSync aus.\nHoch / Runter: Auswahl\nLinks / Rechts / Enter / Leertaste / Klick: Aendern\nEsc: Zurueck zum Spiel", 18);
		instructions.setPosition({ width * 0.5f - 230.0f, height * 0.5f + 75.0f });
		target.draw(instructions);
	}

private:
	static constexpr int SettingCount = 3;
	static sf::FloatRect SettingBounds(int index, sf::Vector2u resolution)
	{
		return { { static_cast<float>(resolution.x) * 0.5f - 230.0f,
			static_cast<float>(resolution.y) * 0.5f - 105.0f + index * 40.0f }, { 460.0f, 35.0f } };
	}

	static sf::FloatRect MainMenuButtonBounds(sf::Vector2u resolution)
	{
		return { { static_cast<float>(resolution.x) * 0.5f - 230.0f,
			static_cast<float>(resolution.y) * 0.5f + 20.0f }, { 460.0f, 42.0f } };
	}

	enum class Selection { VSync, FpsLimit, ShowFps, MainMenu };
	sf::Font font;
	std::size_t fpsLimitIndex = GameSettings::fpsLimits.size() - 1;
	Selection selected = Selection::VSync;
	bool fontLoaded = false;
	bool open = false;
	bool verticalSync = false;
	bool showFps = true;
#if defined(MYRPG_MAP_EDITOR) && !defined(_DEBUG)
	bool mapEditorEnabled = false;
#endif
};
