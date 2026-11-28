#pragma once

#include <SFML/Graphics.hpp>

#include <cstdio>
#include <iterator>

#include "MenuFont.h"
#include "SaveGameStore.h"

// Manages the title screen; the main loop executes the selected action.
class MainMenu
{
public:
	enum class Action { None, OpenSaves, LoadSelected, DeleteSelected, OpenNewGame, StartNewGame, ExitGame,
		SaveSelected, CreateSave, CloseSaveList };

	bool Load()
	{
		if (!LoadMenuFont(font))
		{
			return false;
		}
		if (!background.loadFromFile("Assets/UI/title_screen.png"))
		{
			std::fprintf(stderr, "Failed to load Assets/UI/title_screen.png.\n");
			return false;
		}
		return true;
	}

	void Reset()
	{
		CloseSaves();
		showNewGame = false;
		creatingSaveName = false;
		ignoreNewSaveShortcutText = false;
		newGameName.clear();
		selected = 0;
	}

	void OpenNewGame()
	{
		newGameName.clear();
		creatingSaveName = false;
		showNewGame = true;
	}

	std::string NewGameName() const
	{
		const auto utf8 = newGameName.toUtf8();
		std::string name(utf8.begin(), utf8.end());
		if (!name.empty()) name.erase(name.find_last_not_of(' ') + 1);
		return name;
	}

	void HandleText(char32_t character)
	{
		if (!showNewGame) return;
		if (ignoreNewSaveShortcutText)
		{
			ignoreNewSaveShortcutText = false;
			if (character == U'n' || character == U'N') return;
		}
		if (character == U'\b')
		{
			if (!newGameName.isEmpty()) newGameName.erase(newGameName.getSize() - 1);
		}
		else if (newGameName.getSize() < MaxNameLength && character >= U' ' &&
			(character < 0x7f || character >= 0xa0) && character != 0x2028 && character != 0x2029 &&
			(character != U' ' || !newGameName.isEmpty()))
		{
			newGameName += sf::String(character);
		}
	}

	void OpenSaves(std::vector<SaveGameSlot> slots, bool forSaving = false,
		std::optional<SaveGameSlot::Id> active = std::nullopt)
	{
		saveSlots = std::move(slots);
		selectedSlot = 0;
		if (active)
		{
			for (std::size_t i = 0; i < saveSlots.size(); ++i)
				if (saveSlots[i].id == *active) { selectedSlot = i; break; }
		}
		saving = forSaving;
		showSaves = true;
	}

	void CloseSaveList() { CloseSaves(); }
	void DrawSaveList(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		DrawSaves(target, resolution);
		if (showNewGame && creatingSaveName) DrawNewGame(target, resolution);
	}

	const SaveGameSlot* SelectedSave() const
	{
		return showSaves && !saveSlots.empty() ? &saveSlots[selectedSlot] : nullptr;
	}

	void RemoveSelectedSave()
	{
		if (!SelectedSave()) return;
		saveSlots.erase(saveSlots.begin() + selectedSlot);
		if (selectedSlot > 0 && selectedSlot == saveSlots.size()) --selectedSlot;
	}

	Action HandleKey(sf::Keyboard::Key key)
	{
		ignoreNewSaveShortcutText = false;
		if (showNewGame)
		{
			if (key == sf::Keyboard::Key::Escape) { showNewGame = false; ignoreNewSaveShortcutText = false; }
			if (key == sf::Keyboard::Key::Enter && !NewGameName().empty())
				return creatingSaveName ? Action::CreateSave : Action::StartNewGame;
			return Action::None;
		}
		if (showSaves) return HandleSaveKey(key);
		if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Down)
		{
			const int step = key == sf::Keyboard::Key::Up ? ButtonCount - 1 : 1;
			selected = (selected + step) % ButtonCount;
		}
		else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
		{
			return Buttons[selected].action;
		}
		return Action::None;
	}

	Action HandleClick(sf::Vector2f position, sf::Vector2u resolution)
	{
		if (showNewGame)
		{
			if (NewGameButtonBounds(true, resolution).contains(position)) { showNewGame = false; ignoreNewSaveShortcutText = false; }
			else if (NewGameButtonBounds(false, resolution).contains(position) && !NewGameName().empty())
				return creatingSaveName ? Action::CreateSave : Action::StartNewGame;
			return Action::None;
		}
		if (showSaves) return HandleSaveClick(position, resolution);
		for (int index = 0; index < ButtonCount; ++index)
		{
			if (ButtonBounds(index, resolution).contains(position))
			{
				selected = index;
				return Buttons[index].action;
			}
		}
		return Action::None;
	}

	void Draw(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const sf::Vector2u textureSize = background.getSize();
		sf::Sprite sprite(background);
		sprite.setScale({ static_cast<float>(resolution.x) / textureSize.x,
						  static_cast<float>(resolution.y) / textureSize.y });
		target.draw(sprite);

		if (showNewGame)
		{
			DrawNewGame(target, resolution);
			return;
		}
		if (showSaves)
		{
			DrawSaves(target, resolution);
			return;
		}
		for (int index = 0; index < ButtonCount; ++index)
		{
			DrawButton(target, ButtonBounds(index, resolution), Buttons[index].label, 27, index == selected);
		}
	}

private:
	static constexpr std::size_t RowsPerPage = 6;
	static constexpr std::size_t MaxNameLength = 32;

	void DrawNewGame(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const float left = static_cast<float>(resolution.x) * 0.5f - 280.0f;
		const float top = static_cast<float>(resolution.y) * 0.5f - 130.0f;
		sf::RectangleShape panel({ 560.0f, 260.0f });
		panel.setPosition({ left, top });
		panel.setFillColor(sf::Color(28, 35, 32, 245));
		panel.setOutlineThickness(2.0f);
		panel.setOutlineColor(sf::Color(160, 150, 115));
		target.draw(panel);

		sf::Text title(font, creatingSaveName ? "Neuer Spielstand" : "Neues Spiel", 32);
		title.setPosition({ left + 24.0f, top + 14.0f });
		target.draw(title);
		sf::Text prompt(font, creatingSaveName ? "Speichername (max. 32 Zeichen)" : "Spielname (max. 32 Zeichen)", 20);
		prompt.setPosition({ left + 24.0f, top + 70.0f });
		target.draw(prompt);
		DrawButton(target, { { left + 24.0f, top + 104.0f }, { 512.0f, 46.0f } },
			newGameName + L"|", 24, true);

		const bool hasName = !NewGameName().empty();
		DrawButton(target, NewGameButtonBounds(false, resolution), creatingSaveName ? "Speichern" : "Spiel starten", 22, hasName, hasName);
		DrawButton(target, NewGameButtonBounds(true, resolution), "Abbrechen", 22, false);
	}

	static sf::FloatRect NewGameButtonBounds(bool cancel, sf::Vector2u resolution)
	{
		return { { static_cast<float>(resolution.x) * 0.5f + (cancel ? 12.0f : -212.0f),
			static_cast<float>(resolution.y) * 0.5f + 52.0f }, { 200.0f, 46.0f } };
	}

	void CloseSaves()
	{
		showSaves = false;
		saving = false;
		showNewGame = false;
		creatingSaveName = false;
		ignoreNewSaveShortcutText = false;
		saveSlots.clear();
		selectedSlot = 0;
	}

	void OpenSaveName(bool fromShortcut)
	{
		newGameName.clear();
		creatingSaveName = true;
		showNewGame = true;
		ignoreNewSaveShortcutText = fromShortcut;
	}

	std::size_t FirstVisibleSlot() const { return selectedSlot / RowsPerPage * RowsPerPage; }
	bool HasPreviousPage() const { return FirstVisibleSlot() != 0; }
	bool HasNextPage() const { return FirstVisibleSlot() + RowsPerPage < saveSlots.size(); }

	Action HandleSaveKey(sf::Keyboard::Key key)
	{
		using Key = sf::Keyboard::Key;
		if (key == Key::Escape)
		{
			if (saving) return Action::CloseSaveList;
			CloseSaves();
		}
		else if (saving && key == Key::N) OpenSaveName(true);
		else if (SelectedSave())
		{
			if (key == Key::Up && selectedSlot > 0) --selectedSlot;
			else if (key == Key::Down && selectedSlot + 1 < saveSlots.size()) ++selectedSlot;
			else if ((key == Key::Left || key == Key::PageUp) && HasPreviousPage())
				selectedSlot = FirstVisibleSlot() - RowsPerPage;
			else if ((key == Key::Right || key == Key::PageDown) && HasNextPage())
				selectedSlot = FirstVisibleSlot() + RowsPerPage;
			else if (key == Key::Enter || key == Key::Space) return saving ? Action::SaveSelected : Action::LoadSelected;
			else if (saving && key == Key::L) return Action::LoadSelected;
			else if (key == Key::Delete) return Action::DeleteSelected;
		}
		return Action::None;
	}

	Action HandleSaveClick(sf::Vector2f position, sf::Vector2u resolution)
	{
		for (std::size_t row = 0; row < RowsPerPage && FirstVisibleSlot() + row < saveSlots.size(); ++row)
		{
			if (SaveRowBounds(row, resolution).contains(position))
			{
				selectedSlot = FirstVisibleSlot() + row;
				return Action::None;
			}
		}
		if (SaveControlBounds(2, resolution).contains(position))
		{
			if (saving) return Action::CloseSaveList;
			CloseSaves();
		}
		else if (saving && NewSaveBounds(resolution).contains(position)) OpenSaveName(false);
		else if (SelectedSave())
		{
			if (saving && LoadSaveBounds(resolution).contains(position)) return Action::LoadSelected;
			if (SaveControlBounds(0, resolution).contains(position)) return saving ? Action::SaveSelected : Action::LoadSelected;
			if (SaveControlBounds(1, resolution).contains(position)) return Action::DeleteSelected;
			if (HasPreviousPage() && PageButtonBounds(false, resolution).contains(position))
				selectedSlot = FirstVisibleSlot() - RowsPerPage;
			else if (HasNextPage() && PageButtonBounds(true, resolution).contains(position))
				selectedSlot = FirstVisibleSlot() + RowsPerPage;
		}
		return Action::None;
	}

	void DrawSaves(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const float left = SavePanelLeft(resolution);
		sf::RectangleShape panel({ 680.0f, 536.0f });
		panel.setPosition({ left, 106.0f });
		panel.setFillColor(sf::Color(28, 35, 32, 245));
		panel.setOutlineThickness(2.0f);
		panel.setOutlineColor(sf::Color(160, 150, 115));
		target.draw(panel);

		sf::Text title(font, saving ? "Spiel speichern" : "Spiel laden", 32);
		title.setPosition({ left + 24.0f, 118.0f });
		target.draw(title);

		if (saveSlots.empty())
		{
			sf::Text empty(font, "Noch keine Spielstaende vorhanden.", 22);
			empty.setPosition({ left + 24.0f, 180.0f });
			target.draw(empty);
		}
		for (std::size_t row = 0; row < RowsPerPage && FirstVisibleSlot() + row < saveSlots.size(); ++row)
		{
			const auto index = FirstVisibleSlot() + row;
			const SaveGameSlot& slot = saveSlots[index];
			const auto map = slot.mapFile.empty() ? "Nicht lesbar" :
				(slot.mapFile.size() > 32 ? slot.mapFile.substr(0, 29) + "..." : slot.mapFile);
			const auto rowText = slot.Label() + "  |  " + map;
			const auto bounds = SaveRowBounds(row, resolution);
			DrawButton(target, { bounds.position, { 460.0f, bounds.size.y } },
				sf::String::fromUtf8(rowText.begin(), rowText.end()), 20, index == selectedSlot);
			DrawButton(target, { bounds.position + sf::Vector2f(460.0f, 0.0f), { bounds.size.x - 460.0f, bounds.size.y } },
				slot.PlaytimeLabel(), 18, index == selectedSlot);
		}

		if (SelectedSave())
		{
			const auto pages = (saveSlots.size() - 1) / RowsPerPage + 1;
			sf::Text page(font, "Seite " + std::to_string(selectedSlot / RowsPerPage + 1) + " / " + std::to_string(pages), 18);
			page.setPosition({ left + 84.0f, 490.0f });
			target.draw(page);
			DrawButton(target, PageButtonBounds(false, resolution), "<", 20, false, HasPreviousPage());
			DrawButton(target, PageButtonBounds(true, resolution), ">", 20, false, HasNextPage());
		}
		DrawButton(target, SaveControlBounds(0, resolution), saving ? "Ueberschreiben" : "Laden", 22, false, SelectedSave() != nullptr);
		DrawButton(target, SaveControlBounds(1, resolution), L"Löschen", 22, false, SelectedSave() != nullptr);
		DrawButton(target, SaveControlBounds(2, resolution), L"Zurück", 22, false);
		if (saving)
		{
			DrawButton(target, NewSaveBounds(resolution), "Neuer Spielstand", 18, false);
			DrawButton(target, LoadSaveBounds(resolution), "Laden", 18, false, SelectedSave() != nullptr);
		}
		sf::Text help(font, saving ? "Enter: Ueberschreiben | N: Neu | L: Laden | Entf: Loeschen | Esc: Zurueck" :
			"Enter: Laden   |   Entf: Loeschen   |   Esc: Zurueck", 17);
		help.setPosition({ left + 24.0f, 603.0f });
		target.draw(help);
	}

	void DrawButton(sf::RenderTarget& target, sf::FloatRect bounds, const sf::String& text,
		unsigned int size, bool highlighted, bool enabled = true) const
	{
		sf::RectangleShape button(bounds.size);
		button.setPosition(bounds.position);
		button.setFillColor(highlighted ? sf::Color(72, 81, 65, 235) : sf::Color(28, 35, 32, 235));
		button.setOutlineThickness(2.0f);
		button.setOutlineColor(highlighted ? sf::Color(255, 220, 140) : sf::Color(160, 150, 115));
		target.draw(button);

		sf::Text label(font, text, size);
		if (!enabled) label.setFillColor(sf::Color(135, 135, 135));
		const sf::FloatRect textBounds = label.getLocalBounds();
		if (textBounds.size.x > bounds.size.x - 24.0f)
			label.setScale({ (bounds.size.x - 24.0f) / textBounds.size.x, 1.0f });
		label.setOrigin({ textBounds.position.x + textBounds.size.x * 0.5f,
						  textBounds.position.y + textBounds.size.y * 0.5f });
		label.setPosition({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		target.draw(label);
	}

	static float SavePanelLeft(sf::Vector2u resolution) { return static_cast<float>(resolution.x) * 0.5f - 340.0f; }

	static sf::FloatRect SaveRowBounds(std::size_t row, sf::Vector2u resolution)
	{
		return { { SavePanelLeft(resolution) + 24.0f, 174.0f + static_cast<float>(row) * 50.0f }, { 632.0f, 42.0f } };
	}

	static sf::FloatRect SaveControlBounds(int index, sf::Vector2u resolution)
	{
		return { { SavePanelLeft(resolution) + 24.0f + index * 216.0f, 541.0f }, { 200.0f, 46.0f } };
	}

	static sf::FloatRect PageButtonBounds(bool next, sf::Vector2u resolution)
	{
		return { { SavePanelLeft(resolution) + (next ? 196.0f : 24.0f), 484.0f }, { 48.0f, 36.0f } };
	}

	static sf::FloatRect NewSaveBounds(sf::Vector2u resolution)
	{
		return { { SavePanelLeft(resolution) + 444.0f, 484.0f }, { 212.0f, 36.0f } };
	}

	static sf::FloatRect LoadSaveBounds(sf::Vector2u resolution)
	{
		return { { SavePanelLeft(resolution) + 272.0f, 484.0f }, { 154.0f, 36.0f } };
	}

	struct MenuButton { const char* label; Action action; };
	static constexpr MenuButton Buttons[] = {
		{ "Spiel laden", Action::OpenSaves },
		{ "Neues Spiel", Action::OpenNewGame },
		{ "Spiel beenden", Action::ExitGame }
	};
	static constexpr int ButtonCount = std::size(Buttons);

	static sf::FloatRect ButtonBounds(int index, sf::Vector2u resolution)
	{
		return { { static_cast<float>(resolution.x) * 0.5f - 150.0f,
				   static_cast<float>(resolution.y) * 0.55f + index * 75.0f }, { 300.0f, 54.0f } };
	}

	sf::Font font;
	sf::Texture background;
	int selected = 0;
	bool showNewGame = false;
	bool creatingSaveName = false;
	bool ignoreNewSaveShortcutText = false;
	sf::String newGameName;
	bool showSaves = false;
	bool saving = false;
	std::vector<SaveGameSlot> saveSlots;
	std::size_t selectedSlot = 0;
};
