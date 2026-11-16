#pragma once

#include "../BuildConfig.h"

#ifdef MYRPG_MAP_EDITOR
#include <Windows.h>
#include <commdlg.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "AssetBrowser.h"
#include "MapDocument.h"
#include "../engine/GameEngine.h"
#include "../engine/GameRenderer.h"
#include "../engine/MenuFont.h"

#pragma comment(lib, "Comdlg32.lib")

class MapEditor
{
public:
	void Run(sf::RenderWindow& gameWindow, GameState& game)
	{
		sf::Font font;
		if (!LoadMenuFont(font))
		{
			MessageBoxW(gameWindow.getNativeHandle(), L"Die Editorschrift konnte nicht geladen werden.", L"Map-Editor", MB_OK | MB_ICONERROR);
			return;
		}
		sf::RenderWindow window(sf::VideoMode({ 1280u, 800u }), "MyRPG Map-Editor", sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close);
		window.setFramerateLimit(60);
		window.setKeyRepeatEnabled(false);
		owner = window.getNativeHandle();
		std::error_code ec;
		root = std::filesystem::current_path(ec);
		if (ec)
		{
			Error("Spielordner konnte nicht ermittelt werden: " + ec.message());
			return;
		}
		RefreshAssets();
		MessageBoxW(owner, L"Karten und Originalsicherungen liegen im Build-Ausgabeordner. Ein Build oder Bereinigen kann sie ueberschreiben oder entfernen. Bitte wichtige Aenderungen rechtzeitig ins Projekt uebernehmen oder extern sichern.", L"Map-Editor", MB_OK | MB_ICONWARNING);
		spawn = { game.player.x, game.player.y };
		camera = spawn;
		if (!game.activeMapPath.empty())
		{
			std::string error;
			if (!document.Load(root / game.activeMapPath, error)) Error(error);
		}
		else Error("Die aktuelle Ersatzkarte ist keine gespeicherte Kartendatei. Bitte eine Karte oeffnen oder neu anlegen.");
		sf::Clock panClock;
		while (window.isOpen() && gameWindow.isOpen())
		{
			while (const std::optional event = gameWindow.pollEvent())
				if (event->is<sf::Event::Closed>() && ConfirmDiscard()) gameWindow.close();
			while (const std::optional event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					if (ConfirmDiscard()) window.close();
				}
				else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) HandleKey(key->code, window);
				else if (const auto* button = event->getIf<sf::Event::MouseButtonPressed>()) HandlePress(*button, window, game);
				else if (const auto* button = event->getIf<sf::Event::MouseButtonReleased>())
				{
					if (button->button == sf::Mouse::Button::Left) { draggingObject = false; draggingTrigger = false; creatingTrigger = false; resizeEdges = 0; placementDragActive = false; hasLastPlacementPosition = false; }
					if (button->button == sf::Mouse::Button::Middle) panning = false;
				}
				else if (const auto* move = event->getIf<sf::Event::MouseMoved>())
				{
					const sf::Vector2f position = EditorPosition(window, move->position);
					if (panning)
					{
						camera.x -= (position.x - lastMouse.x) / zoom;
						camera.y -= (position.y - lastMouse.y) / zoom;
						lastMouse = position;
					}
					else if (draggingObject && selectedObject < document.map.objects.size())
					{
						const sf::Vector2f pos = WorldPosition(window, move->position);
						MapObject& object = document.map.objects[selectedObject];
						object.x = object.type == MapObjectType::Scenery || object.type == MapObjectType::Untergrund ? SnapToGrid(pos.x, 64.0f) : pos.x;
						object.y = object.type == MapObjectType::Scenery || object.type == MapObjectType::Untergrund ? SnapToGrid(pos.y, 64.0f) : pos.y;
						document.dirty = true;
					}
					else if (resizeEdges && triggerMode && !spawnMode && selectedTrigger < document.map.triggers.size())
					{
						ResizeTrigger(WorldPosition(window, move->position));
					}
					else if ((draggingTrigger || creatingTrigger) && triggerMode && !spawnMode && selectedTrigger < document.map.triggers.size())
					{
						const sf::Vector2f pos = WorldPosition(window, move->position);
						auto& trigger = document.map.triggers[selectedTrigger];
						if (creatingTrigger)
						{
							trigger.x = (std::min)(triggerAnchor.x, pos.x);
							trigger.y = (std::min)(triggerAnchor.y, pos.y);
							trigger.width = (std::max)(1.0f, std::abs(pos.x - triggerAnchor.x));
							trigger.height = (std::max)(1.0f, std::abs(pos.y - triggerAnchor.y));
						}
						else { trigger.x = pos.x - triggerOffset.x; trigger.y = pos.y - triggerOffset.y; }
						document.dirty = true;
					}
					else if (placementDragActive && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
						!triggerMode && !selectMode && !spawnMode && IsMapArea(position))
					{
						PlaceDraggedSprites(WorldPosition(window, move->position));
					}
				}
				else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
				{
					const sf::Vector2f position = EditorPosition(window, wheel->position);
					if (mapPropertiesOpen && position.x >= 440 && position.x < 820 && position.y >= 330 && position.y < 600)
					{
						const int weatherCount = static_cast<int>(GetWeatherEffectDefinitions().size()) + 1;
						mapWeatherOffset = std::clamp(mapWeatherOffset - (wheel->delta > 0 ? 1 : -1), 0, (std::max)(0, weatherCount - 9));
					}
					else if (mapPropertiesOpen)
					{
						// The map properties dialog handles scrolling only over its weather list.
					}
					else if (TriggerPanelVisible() && position.x >= 960 && position.y >= 70 && position.y < 580)
					{
						if (triggerDropdown != TriggerDropdown::None)
						{
							const int maxOffset = (std::max)(0, static_cast<int>(dropdownEntries.size()) - 8);
							dropdownOffset = std::clamp(dropdownOffset - (wheel->delta > 0 ? 3 : -3), 0, maxOffset);
						}
					}
					else if (ObjectPanelVisible() && position.x >= 960 && position.y >= 70 && position.y < 270)
					{
						// Scrolling over the properties panel must not zoom the map.
					}
					else if (position.x < 280)
					{
						const int maxOffset = (std::max)(0, static_cast<int>(browser.visible.size()) - 17);
						browser.offset = std::clamp(browser.offset - (wheel->delta > 0 ? 3 : -3), 0, maxOffset);
					}
					else zoom = std::clamp(zoom * (wheel->delta > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 4.0f);
				}
			}
			const float panDelta = panClock.restart().asSeconds();
			if (window.hasFocus() && !mapPropertiesOpen)
			{
				float panX = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) -
					static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left));
				float panY = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) -
					static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up));
				const float panLength = std::sqrt(panX * panX + panY * panY);
				if (panLength > 0.0f)
				{
					const float distance = 500.0f * panDelta / (zoom * panLength);
					camera.x += panX * distance;
					camera.y += panY * distance;
				}
			}
			if (window.isOpen()) Draw(window, font);
		}
	}

private:
	MapDocument document;
	std::filesystem::path root;
	HWND owner = nullptr;
	AssetBrowser browser;
	std::size_t selectedObject = static_cast<std::size_t>(-1), selectedTrigger = static_cast<std::size_t>(-1);
	sf::Vector2f camera{ 0.0f, 0.0f }, spawn{ 0.0f, 0.0f }, triggerAnchor{}, triggerOffset{};
	sf::Vector2f resizeStart{}, resizePosition{}, resizeSize{};
	float zoom = 1.0f;
	sf::Vector2f lastMouse{}, lastPlacementPosition{};
	bool placementDragActive = false, hasLastPlacementPosition = false;
	bool mapPropertiesOpen = false;
	int mapWeatherOffset = 0;
	bool selectMode = false, spawnMode = false, triggerMode = false, panning = false, draggingObject = false, draggingTrigger = false, creatingTrigger = false;
	enum TriggerEdge { Left = 1, Right = 2, Top = 4, Bottom = 8 };
	int resizeEdges = 0;
	MapObjectType newType = MapObjectType::Scenery;
	bool newSolid = false;
	enum class TriggerDropdown { None, Maps, Effects };
	TriggerDropdown triggerDropdown = TriggerDropdown::None;
	std::vector<std::string> dropdownEntries;
	int dropdownOffset = 0;

	bool TriggerPanelVisible() const
	{
		return triggerMode && !spawnMode && selectedTrigger < document.map.triggers.size();
	}
	bool ObjectPanelVisible() const
	{
		return !triggerMode && !spawnMode && (!selectMode || selectedObject < document.map.objects.size());
	}

	int TriggerEdgesAt(const TriggerZone& trigger, sf::Vector2f pos) const
	{
		const float tolerance = 6.0f / zoom;
		const float left = std::abs(pos.x - trigger.x), right = std::abs(pos.x - (trigger.x + trigger.width));
		const float top = std::abs(pos.y - trigger.y), bottom = std::abs(pos.y - (trigger.y + trigger.height));
		int edges = 0;
		if (pos.y >= trigger.y - tolerance && pos.y <= trigger.y + trigger.height + tolerance)
		{
			if (left <= tolerance && left <= right) edges |= Left;
			else if (right <= tolerance) edges |= Right;
		}
		if (pos.x >= trigger.x - tolerance && pos.x <= trigger.x + trigger.width + tolerance)
		{
			if (top <= tolerance && top <= bottom) edges |= Top;
			else if (bottom <= tolerance) edges |= Bottom;
		}
		return edges;
	}
	void ResizeTrigger(sf::Vector2f pos)
	{
		auto& trigger = document.map.triggers[selectedTrigger];
		const sf::Vector2f delta = pos - resizeStart;
		float left = resizePosition.x, top = resizePosition.y;
		float right = left + resizeSize.x, bottom = top + resizeSize.y;
		if (resizeEdges & Left) left = (std::min)(left + delta.x, right - 1.0f);
		if (resizeEdges & Right) right = (std::max)(right + delta.x, left + 1.0f);
		if (resizeEdges & Top) top = (std::min)(top + delta.y, bottom - 1.0f);
		if (resizeEdges & Bottom) bottom = (std::max)(bottom + delta.y, top + 1.0f);
		const float width = right - left, height = bottom - top;
		if (trigger.x != left || trigger.y != top || trigger.width != width || trigger.height != height)
		{
			trigger.x = left; trigger.y = top; trigger.width = width; trigger.height = height;
			document.dirty = true;
		}
	}

	void Error(const std::string& message) const
	{
		std::fprintf(stderr, "Map-Editor: %s\n", message.c_str());
		MessageBoxA(owner, message.c_str(), "Map-Editor", MB_OK | MB_ICONERROR);
	}
	bool ConfirmDiscard() const
	{
		return !document.dirty || MessageBoxW(owner, L"Ungespeicherte Aenderungen verwerfen?", L"Map-Editor", MB_YESNO | MB_ICONWARNING) == IDYES;
	}
	static bool SafeName(const std::string& value)
	{
		return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-'; });
	}
	void RefreshAssets()
	{
		std::string error;
		if (!browser.Refresh(root, error)) Error(error);
	}
	void OpenTriggerDropdown(TriggerDropdown type)
	{
		if (!TriggerPanelVisible()) { Error("Bitte zuerst eine Triggerzone auswaehlen."); return; }
		if (triggerDropdown == type) { triggerDropdown = TriggerDropdown::None; return; }
		std::vector<std::string> entries;
		if (type == TriggerDropdown::Maps)
		{
			std::error_code ec;
			const auto directory = root / L"Assets" / L"Maps";
			std::filesystem::directory_iterator it(directory, ec), end;
			if (ec) { Error("Kartenverzeichnis konnte nicht gelesen werden: " + ec.message()); return; }
			while (it != end)
			{
				const auto path = it->path();
				const bool regular = it->is_regular_file(ec);
				if (ec) { Error("Kartendatei konnte nicht geprueft werden: " + ec.message()); return; }
				if (regular && path.extension() == L".map")
				{
					const std::string name = path.stem().string();
					if (!name.empty() && name.find_first_of(" \t\r\n\\/:*?\"<>|") == std::string::npos)
						entries.push_back("Load_" + name);
					else std::fprintf(stderr, "Map-Editor: Nicht unterstuetzter Kartenname: %s\n", name.c_str());
				}
				it.increment(ec);
				if (ec) { Error("Kartenverzeichnis konnte nicht vollstaendig gelesen werden: " + ec.message()); return; }
			}
			std::sort(entries.begin(), entries.end());
		}
		else
		{
			entries = { "screen_shake", "weather_none" };
			for (const auto& effect : GetMagicEffectDefinitions()) entries.push_back("magic_" + effect.id);
			for (const auto& effect : GetWeatherEffectDefinitions()) entries.push_back("weather_" + effect.id);
		}
		entries.insert(entries.begin(), NoTriggerEvent);
		dropdownEntries = std::move(entries);
		dropdownOffset = 0;
		triggerDropdown = type;
	}
	void ToggleTriggerGlow()
	{
		if (!TriggerPanelVisible()) { Error("Bitte zuerst eine Triggerzone auswaehlen."); return; }
		auto& trigger = document.map.triggers[selectedTrigger];
		trigger.glow = !trigger.glow;
		document.dirty = true;
	}
	void HandleTriggerPanelClick(int x, int y)
	{
		if (x < 970 || x >= 1270) return;
		if (y >= 150 && y < 180) OpenTriggerDropdown(TriggerDropdown::Maps);
		else if (y >= 205 && y < 235) OpenTriggerDropdown(TriggerDropdown::Effects);
		else if (y >= 480 && y < 508) ToggleTriggerGlow();
		else if (triggerDropdown != TriggerDropdown::None && y >= 250 && y < 442)
		{
			const std::size_t index = static_cast<std::size_t>(dropdownOffset + (y - 250) / 24);
			if (index < dropdownEntries.size())
			{
				auto& eventName = document.map.triggers[selectedTrigger].eventName;
				if (eventName != dropdownEntries[index]) { eventName = dropdownEntries[index]; document.dirty = true; }
				triggerDropdown = TriggerDropdown::None;
			}
		}
	}
	bool ChoosePath(bool create, std::filesystem::path& result)
	{
		const std::filesystem::path maps = root / L"Assets" / L"Maps";
		wchar_t file[32768]{};
		const std::wstring initial = maps.wstring();
		OPENFILENAMEW dialog{};
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = owner;
		dialog.lpstrFilter = L"Map-Dateien (*.map)\0*.map\0\0";
		dialog.lpstrFile = file;
		dialog.nMaxFile = static_cast<DWORD>(std::size(file));
		dialog.lpstrInitialDir = initial.c_str();
		dialog.lpstrDefExt = L"map";
		dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (create ? 0 : OFN_FILEMUSTEXIST);
		if (!(create ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog)))
		{
			const DWORD code = CommDlgExtendedError();
			if (code != 0) Error("Dateidialog fehlgeschlagen: " + std::to_string(code));
			return false;
		}
		result = file;
		std::error_code ec;
		if (!std::filesystem::equivalent(result.parent_path(), maps, ec) || ec || result.extension() != L".map" || !SafeName(result.stem().string()))
		{
			Error("Karten muessen direkt in Assets\\Maps liegen und einen Namen aus Buchstaben, Ziffern, _ oder - haben.");
			return false;
		}
		if (create && std::filesystem::exists(result, ec))
		{
			Error("Diese Karte existiert bereits. Bitte oeffnen oder einen anderen Namen waehlen.");
			return false;
		}
		if (ec) { Error("Kartenpfad konnte nicht geprueft werden: " + ec.message()); return false; }
		return true;
	}
	void Open(bool create)
	{
		if (!ConfirmDiscard()) return;
		std::filesystem::path path;
		if (!ChoosePath(create, path)) return;
		if (create)
		{
			document.New(path, path.stem().string());
			spawn = { 0.0f, 0.0f };
		}
		else
		{
			std::string error;
			if (!document.Load(path, error)) { Error(error); return; }
			spawn = { 0.0f, 0.0f };
		}
		camera = spawn;
		selectedObject = static_cast<std::size_t>(-1);
		selectedTrigger = static_cast<std::size_t>(-1);
		triggerDropdown = TriggerDropdown::None;
		draggingTrigger = creatingTrigger = false;
		resizeEdges = 0;
	}
	bool Save()
	{
		std::string error;
		if (!document.Save(error)) { Error(error); return false; }
		selectedObject = static_cast<std::size_t>(-1);
		selectedTrigger = static_cast<std::size_t>(-1);
		triggerDropdown = TriggerDropdown::None;
		draggingTrigger = creatingTrigger = false;
		resizeEdges = 0;
		return true;
	}
	void Apply(GameState& game)
	{
		if (document.dirty)
		{
			if (MessageBoxW(owner, L"Die Karte muss vor der Uebernahme gespeichert werden. Jetzt speichern?", L"Map-Editor", MB_YESNO | MB_ICONQUESTION) != IDYES || !Save()) return;
		}
		if (document.path.empty()) { Error("Bitte zuerst eine Karte oeffnen oder erstellen."); return; }
		std::string error;
		if (!document.IsCurrent(error)) { Error(error); return; }
		MapData loaded = document.map;
		GameState test;
		test.activeMap = loaded;
		bool warning = IsSolidObjectBlockingPosition(test, spawn.x, spawn.y);
		for (const TriggerZone& trigger : loaded.triggers) warning = warning || (trigger.eventName != NoTriggerEvent && IsInsideTrigger(trigger, spawn.x, spawn.y));
		if (warning && MessageBoxW(owner, L"Die Spielerposition liegt in einem Hindernis oder Trigger. Trotzdem uebernehmen?", L"Map-Editor", MB_YESNO | MB_ICONWARNING) != IDYES) return;
		game.activeMap = std::move(loaded);
		game.activeMapPath = document.path.lexically_relative(root).generic_string();
		game.player.x = spawn.x;
		game.player.y = spawn.y;
		game.player.moving = false;
		game.weather = WeatherState{ game.activeMap.defaultWeather };
		game.cameraShake = CameraShakeState{};
		game.mapTransition = MapTransitionState{};
		ClearMagicEffects(game.magicEffects);
		game.lastTriggeredEvent = "MapLoaded: " + game.activeMap.id;
		MessageBoxW(owner, L"Die Karte ist jetzt im Spiel aktiv. Das Spiel bleibt bis zum Schliessen des Editors pausiert.", L"Map-Editor", MB_OK | MB_ICONINFORMATION);
	}
	static sf::FloatRect EditorViewport(sf::Vector2u windowSize)
	{
		if (windowSize.x == 0u || windowSize.y == 0u) return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
		const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		constexpr float editorRatio = 1280.0f / 800.0f;
		if (windowRatio > editorRatio)
		{
			const float width = editorRatio / windowRatio;
			return { { (1.0f - width) * 0.5f, 0.0f }, { width, 1.0f } };
		}
		if (windowRatio < editorRatio)
		{
			const float height = windowRatio / editorRatio;
			return { { 0.0f, (1.0f - height) * 0.5f }, { 1.0f, height } };
		}
		return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
	}
	sf::View EditorView(const sf::RenderWindow& window) const
	{
		sf::View view;
		view.setSize({ 1280.0f, 800.0f });
		view.setCenter({ 640.0f, 400.0f });
		view.setViewport(EditorViewport(window.getSize()));
		return view;
	}
	sf::View MapView(const sf::RenderWindow& window) const
	{
		const sf::FloatRect editorViewport = EditorViewport(window.getSize());
		sf::View view;
		view.setSize({ 1000.0f / zoom, 710.0f / zoom });
		view.setCenter({ 500.0f / zoom, 355.0f / zoom });
		view.setViewport({
			{ editorViewport.position.x + (280.0f / 1280.0f) * editorViewport.size.x,
			  editorViewport.position.y + (60.0f / 800.0f) * editorViewport.size.y },
			{ (1000.0f / 1280.0f) * editorViewport.size.x,
			  (710.0f / 800.0f) * editorViewport.size.y }
		});
		return view;
	}
	sf::Vector2f EditorPosition(const sf::RenderWindow& window, sf::Vector2i pixel) const
	{
		return window.mapPixelToCoords(pixel, EditorView(window));
	}
	static bool IsMapArea(sf::Vector2f position)
	{
		return position.x >= 280.0f && position.x < 1280.0f && position.y >= 60.0f && position.y < 770.0f;
	}
	bool PlaceSelectedSprite(sf::Vector2f position)
	{
		if (document.path.empty() || browser.selected.empty()) return false;
		if (newType == MapObjectType::Scenery || newType == MapObjectType::Untergrund)
		{
			position.x = SnapToGrid(position.x, 64.0f);
			position.y = SnapToGrid(position.y, 64.0f);
		}
		if (hasLastPlacementPosition && position == lastPlacementPosition) return false;
		std::size_t number = 1;
		std::string id;
		do { id = "object_" + std::to_string(number++); } while (document.HasUsedId(id));
		document.map.objects.push_back({ id, newType, position.x, position.y, browser.selected, newSolid });
		document.dirty = true;
		lastPlacementPosition = position;
		hasLastPlacementPosition = true;
		placementDragActive = true;
		return true;
	}
	void PlaceDraggedSprites(sf::Vector2f position)
	{
		if (!hasLastPlacementPosition)
		{
			PlaceSelectedSprite(position);
			return;
		}
		const sf::Vector2f start = lastPlacementPosition;
		const sf::Vector2f delta = position - start;
		const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
		constexpr float spacing = 64.0f;
		const int count = static_cast<int>(distance / spacing);
		for (int index = 1; index <= count; ++index)
		{
			const float fraction = spacing * static_cast<float>(index) / distance;
			PlaceSelectedSprite(start + delta * fraction);
		}
	}
	sf::Vector2f WorldPosition(const sf::RenderWindow& window, sf::Vector2i pixel) const
	{
		const sf::Vector2f pos = window.mapPixelToCoords(pixel, MapView(window));
		return { pos.x + camera.x - 500.0f / zoom, pos.y + camera.y - 355.0f / zoom };
	}
	void ToggleSelectMode()
	{
		triggerMode = false;
		selectedTrigger = static_cast<std::size_t>(-1);
		triggerDropdown = TriggerDropdown::None;
		draggingTrigger = creatingTrigger = false;
		resizeEdges = 0;
		selectMode = !selectMode;
		spawnMode = false;
		if (!selectMode)
		{
			selectedObject = static_cast<std::size_t>(-1);
			draggingObject = false;
		}
	}
	void HandleKey(sf::Keyboard::Key key, sf::RenderWindow& window)
	{
		if (mapPropertiesOpen)
		{
			if (key == sf::Keyboard::Key::Escape) mapPropertiesOpen = false;
			return;
		}
		if (key == sf::Keyboard::Key::Escape) { if (ConfirmDiscard()) window.close(); }
		else if (key == sf::Keyboard::Key::Tab) ToggleSelectMode();
		else if (key == sf::Keyboard::Key::P) { spawnMode = true; draggingTrigger = creatingTrigger = false; resizeEdges = 0; }
		else if (key == sf::Keyboard::Key::T)
		{
			triggerMode = !triggerMode; selectMode = false; spawnMode = false;
			selectedObject = selectedTrigger = static_cast<std::size_t>(-1);
			triggerDropdown = TriggerDropdown::None;
			draggingTrigger = creatingTrigger = false;
			resizeEdges = 0;
		}
		else if (key == sf::Keyboard::Key::Delete && triggerMode)
		{
			if (selectedTrigger < document.map.triggers.size())
			{
				document.map.triggers.erase(document.map.triggers.begin() + selectedTrigger);
				selectedTrigger = static_cast<std::size_t>(-1);
				triggerDropdown = TriggerDropdown::None;
				draggingTrigger = creatingTrigger = false;
				resizeEdges = 0;
				document.dirty = true;
			}
		}
		else if (key == sf::Keyboard::Key::Delete)
		{
			std::optional<std::size_t> target;
			if (selectMode && selectedObject < document.map.objects.size())
			{
				target = selectedObject;
			}
			else
			{
				const sf::Vector2i pixel = sf::Mouse::getPosition(window);
				const sf::Vector2f position = EditorPosition(window, pixel);
				const bool overPanel = ObjectPanelVisible() && position.x >= 960 && position.y >= 70 && position.y < 270;
				if (!overPanel && position.x >= 280 && position.x < 1280 && position.y >= 60 && position.y < 770)
				{
					const sf::Vector2f worldPosition = WorldPosition(window, pixel);
					target = document.FindObjectAt(worldPosition.x, worldPosition.y);
				}
			}
			if (target)
			{
				document.RemoveObject(*target);
				selectedObject = static_cast<std::size_t>(-1);
				draggingObject = false;
			}
		}
	}
	void HandlePress(const sf::Event::MouseButtonPressed& event, sf::RenderWindow& window, GameState& game)
	{
		if (event.button == sf::Mouse::Button::Left)
		{
			placementDragActive = false;
			hasLastPlacementPosition = false;
		}
		const sf::Vector2f position = EditorPosition(window, event.position);
		const int x = static_cast<int>(position.x), y = static_cast<int>(position.y);
		if (mapPropertiesOpen)
		{
			if (event.button == sf::Mouse::Button::Left) HandleMapPropertiesClick(position);
			return;
		}
		if (TriggerPanelVisible() && x >= 960 && y >= 70 && y < 580)
		{
			if (event.button == sf::Mouse::Button::Left) HandleTriggerPanelClick(x, y);
			return;
		}
		if (ObjectPanelVisible() && x >= 960 && y >= 70 && y < 270)
		{
			if (event.button == sf::Mouse::Button::Left && x >= 970 && x < 1270 && y >= 150 && y < 180)
			{
				if (selectMode)
				{
					auto& object = document.map.objects[selectedObject];
					object.solid = !object.solid;
					document.dirty = true;
				}
				else newSolid = !newSolid;
			}
			return;
		}
		if (event.button == sf::Mouse::Button::Middle && x >= 280 && y >= 60 && y < 770)
		{ panning = true; lastMouse = position; return; }
		if (event.button != sf::Mouse::Button::Left) return;
		triggerDropdown = TriggerDropdown::None;
		if (y < 50)
		{
			if (x < 100) Open(true);
			else if (x < 200) Open(false);
			else if (x < 300) Save();
			else if (x < 450) Apply(game);
			else if (x < 560) RefreshAssets();
			else if (x < 820)
			{
				mapPropertiesOpen = true;
				mapWeatherOffset = 0;
			}
			else if (x < 940 && triggerMode)
			{
				OpenTriggerDropdown(TriggerDropdown::Effects);
			}
			else if (x < 1040 && triggerMode)
			{
				ToggleTriggerGlow();
			}
			else if (x < 940)
			{
				MapObjectType& type = selectedObject < document.map.objects.size() ? document.map.objects[selectedObject].type : newType;
				type = type == MapObjectType::Scenery ? MapObjectType::Untergrund : type == MapObjectType::Untergrund ? MapObjectType::Wall : type == MapObjectType::Wall ? MapObjectType::Npc : MapObjectType::Scenery;
				if (selectedObject < document.map.objects.size())
				{
					MapObject& object = document.map.objects[selectedObject];
					if (type == MapObjectType::Scenery || type == MapObjectType::Untergrund) { object.x = SnapToGrid(object.x, 64); object.y = SnapToGrid(object.y, 64); }
					document.dirty = true;
				}
			}
			else if (x < 1040) { }
			else if (x < 1150) HandleKey(sf::Keyboard::Key::P, window);
			else ToggleSelectMode();
			return;
		}
		if (x < 280 && y >= 720 && y < 760)
		{
			HandleKey(sf::Keyboard::Key::T, window);
			return;
		}
		if (x < 280 && y >= 100 && y < 100 + 17 * 36)
		{
			const std::size_t index = static_cast<std::size_t>(browser.offset + (y - 100) / 36);
			if (index < browser.visible.size())
			{
				const auto entry = browser.visible[index];
				if (entry.folder) browser.Toggle(entry.path);
				else { browser.selected = entry.path; selectedObject = selectedTrigger = static_cast<std::size_t>(-1); selectMode = triggerMode = spawnMode = false; draggingTrigger = creatingTrigger = false; resizeEdges = 0; placementDragActive = true; }
			}
			return;
		}
		if (x < 280 || y < 60 || y >= 770 || document.path.empty()) return;
		sf::Vector2f pos = WorldPosition(window, event.position);
		if (spawnMode) { spawn = pos; spawnMode = false; return; }
		if (triggerMode)
		{
			if (selectedTrigger < document.map.triggers.size())
			{
				const auto& trigger = document.map.triggers[selectedTrigger];
				const bool moveSelected = (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) &&
					pos.x >= trigger.x && pos.x <= trigger.x + trigger.width && pos.y >= trigger.y && pos.y <= trigger.y + trigger.height;
				if (moveSelected)
				{
					triggerOffset = { pos.x - trigger.x, pos.y - trigger.y };
					draggingTrigger = true;
					return;
				}
				resizeEdges = TriggerEdgesAt(trigger, pos);
				if (resizeEdges)
				{
					resizeStart = pos;
					resizePosition = { trigger.x, trigger.y };
					resizeSize = { trigger.width, trigger.height };
					return;
				}
			}
			const auto existing = document.FindTriggerAt(pos.x, pos.y);
			if (existing)
			{
				selectedTrigger = *existing;
				const auto& trigger = document.map.triggers[selectedTrigger];
				triggerOffset = { pos.x - trigger.x, pos.y - trigger.y };
				draggingTrigger = true;
			}
			else
			{
				std::size_t number = 1; std::string id;
				do { id = "trigger_" + std::to_string(number++); } while (document.HasUsedTriggerId(id));
				document.map.triggers.push_back({ id, pos.x, pos.y, 64.0f, 64.0f, "screen_shake", false });
				selectedTrigger = document.map.triggers.size() - 1;
				triggerAnchor = pos; creatingTrigger = true; document.dirty = true;
			}
			return;
		}
		if (selectMode)
		{
			const auto object = document.FindObjectAt(pos.x, pos.y);
			selectedObject = object.value_or(static_cast<std::size_t>(-1));
			draggingObject = object.has_value();
			return;
		}
		if (browser.selected.empty()) { Error("Bitte zuerst ein Sprite auswaehlen."); return; }
		PlaceSelectedSprite(pos);
	}
	static const char* TypeName(MapObjectType type)
	{
		return type == MapObjectType::Untergrund ? "untergrund" : type == MapObjectType::Wall ? "wall" : type == MapObjectType::Npc ? "npc" : "scenery";
	}
	void HandleMapPropertiesClick(sf::Vector2f position)
	{
		if (position.x < 420.0f || position.x >= 860.0f || position.y < 140.0f || position.y >= 660.0f || position.y < 188.0f)
		{
			mapPropertiesOpen = false;
			return;
		}
		if (position.x >= 440.0f && position.x < 820.0f && position.y >= 250.0f && position.y < 288.0f)
		{
			if (document.path.empty() || browser.selected.empty())
				Error("Bitte zuerst eine Karte oeffnen oder erstellen und ein Sprite im Asset-Browser auswaehlen.");
			else if (document.map.defaultGroundPath != browser.selected)
			{
				document.map.defaultGroundPath = browser.selected;
				document.dirty = true;
			}
			return;
		}
		if (position.x >= 440.0f && position.x < 820.0f && position.y >= 330.0f && position.y < 600.0f)
		{
			const int row = static_cast<int>((position.y - 330.0f) / 30.0f);
			const int index = mapWeatherOffset + row;
			const auto& weather = GetWeatherEffectDefinitions();
			if (index >= 0 && index <= static_cast<int>(weather.size()))
			{
				const std::string selected = index == 0 ? "none" : weather[static_cast<std::size_t>(index - 1)].id;
				if (document.path.empty()) Error("Bitte zuerst eine Karte oeffnen oder erstellen.");
				else if (document.map.defaultWeather != selected)
				{
					document.map.defaultWeather = selected;
					document.dirty = true;
				}
			}
		}
	}
	void DrawMapPropertiesPanel(sf::RenderWindow& window, const sf::Font& font) const
	{
		if (!mapPropertiesOpen) return;
		auto rectangle = [&](float x, float y, float width, float height, sf::Color color)
		{
			sf::RectangleShape shape({ width, height });
			shape.setPosition({ x, y });
			shape.setFillColor(color);
			window.draw(shape);
		};
		rectangle(420, 140, 440, 520, sf::Color(28, 31, 37, 250));
		rectangle(420, 140, 440, 48, sf::Color(48, 52, 60));
		Label(window, font, "MAP EIGENSCHAFTEN", 440, 155, 18);
		Label(window, font, "Schliessen (Esc)", 710, 157, 12);
		Label(window, font, "Standardboden", 440, 195, 14);
		std::string ground = document.map.defaultGroundPath.empty() ? "Kein Boden gesetzt" : "Aktuell: " + document.map.defaultGroundPath;
		if (ground.size() > 54) ground = "..." + ground.substr(ground.size() - 51);
		Label(window, font, ground, 440, 211, 11);
		std::string selectedSprite = browser.selected.empty() ? "Kein Sprite ausgewaehlt" : "Sprite: " + browser.selected;
		if (selectedSprite.size() > 54) selectedSprite = "..." + selectedSprite.substr(selectedSprite.size() - 51);
		Label(window, font, selectedSprite, 440, 228, 11);
		rectangle(440, 252, 380, 34, sf::Color(65, 70, 80));
		Label(window, font, "Ausgewaehltes Sprite als Boden setzen", 450, 261, 13);
		Label(window, font, "Standardwetter (Mausrad zum Scrollen)", 440, 300, 14);
		const auto& weather = GetWeatherEffectDefinitions();
		const int weatherCount = static_cast<int>(weather.size()) + 1;
		const int visibleRows = (std::min)(9, weatherCount);
		for (int row = 0; row < visibleRows; ++row)
		{
			const int index = mapWeatherOffset + row;
			const std::string id = index == 0 ? "none" : weather[static_cast<std::size_t>(index - 1)].id;
			const bool selected = id == document.map.defaultWeather;
			rectangle(440, static_cast<float>(330 + row * 30), 380, 28,
				selected ? sf::Color(70, 105, 110) : sf::Color(55, 59, 67));
			Label(window, font, index == 0 ? "Kein Wetter (none)" : id, 450, static_cast<float>(337 + row * 30), 13);
		}
		if (weatherCount > visibleRows)
			Label(window, font, std::to_string(mapWeatherOffset + 1) + "-" + std::to_string(mapWeatherOffset + visibleRows) + " / " + std::to_string(weatherCount), 440, 610, 12);
	}
	static void Label(sf::RenderTarget& target, const sf::Font& font, const std::string& text, float x, float y, unsigned int size = 14)
	{
		sf::Text label(font, text, size);
		label.setPosition({ x, y });
		target.draw(label);
	}
	void DrawObjectPanel(sf::RenderWindow& window, const sf::Font& font) const
	{
		if (!ObjectPanelVisible()) return;
		const MapObject* object = selectMode ? &document.map.objects[selectedObject] : nullptr;
		const bool solid = object ? object->solid : newSolid;
		sf::RectangleShape panel({ 320, 200 });
		panel.setPosition({ 960, 70 });
		panel.setFillColor(sf::Color(35, 38, 45, 245));
		window.draw(panel);
		std::string title = object ? "OBJEKT: " + object->id : "NEUE OBJEKTE";
		if (title.size() > 33) title = title.substr(0, 30) + "...";
		Label(window, font, title, 970, 80, 14);
		Label(window, font, std::string("Typ: ") + TypeName(object ? object->type : newType), 970, 105, 13);
		sf::RectangleShape box({ 20, 20 });
		box.setPosition({ 970, 152 });
		box.setFillColor(sf::Color(185, 190, 195));
		window.draw(box);
		box.setSize({ 16, 16 });
		box.setPosition({ 972, 154 });
		box.setFillColor(solid ? sf::Color(70, 145, 150) : sf::Color(35, 38, 45));
		window.draw(box);
		if (solid) Label(window, font, "X", 975, 152, 14);
		Label(window, font, "Solid aktivieren", 998, 153, 14);
		Label(window, font, "Blockiert die Spielerfigur bei", 970, 190, 13);
		Label(window, font, "Kollision; unabhaengig vom Typ.", 970, 208, 13);
		if (!object) Label(window, font, "Vorgabe fuer neu platzierte Objekte.", 970, 236, 13);
	}
	void DrawTriggerPanel(sf::RenderWindow& window, const sf::Font& font) const
	{
		if (!TriggerPanelVisible()) return;
		const auto& trigger = document.map.triggers[selectedTrigger];
		auto rectangle = [&](float x, float y, float width, float height, sf::Color color)
		{
			sf::RectangleShape shape({ width, height }); shape.setPosition({ x, y }); shape.setFillColor(color); window.draw(shape);
		};
		auto shortText = [](const std::string& text)
		{
			return text.size() > 30 ? text.substr(0, 27) + "..." : text;
		};
		rectangle(960, 70, 320, 510, sf::Color(35, 38, 45, 245));
		const bool noEvent = trigger.eventName == NoTriggerEvent;
		Label(window, font, "TRIGGER: " + shortText(trigger.id), 970, 80, 14);
		Label(window, font, "Ereignis: " + (noEvent ? std::string("Kein Ereignis") : shortText(trigger.eventName)), 970, 105, 13);
		Label(window, font, "Load Map", 970, 130, 13);
		rectangle(970, 150, 300, 30, sf::Color(65, 70, 80));
		Label(window, font, noEvent ? "Kein Ereignis  v" : trigger.eventName.starts_with("Load_") ? shortText(trigger.eventName.substr(5)) + "  v" : "Karte auswaehlen  v", 978, 156, 13);
		Label(window, font, "Effekte", 970, 185, 13);
		rectangle(970, 205, 300, 30, sf::Color(65, 70, 80));
		Label(window, font, noEvent ? "Kein Ereignis  v" : !trigger.eventName.starts_with("Load_") ? shortText(trigger.eventName) + "  v" : "Effekt auswaehlen  v", 978, 211, 13);
		if (triggerDropdown != TriggerDropdown::None)
		{
			for (int row = 0; row < 8 && dropdownOffset + row < static_cast<int>(dropdownEntries.size()); ++row)
			{
				const auto& entry = dropdownEntries[static_cast<std::size_t>(dropdownOffset + row)];
				const float y = static_cast<float>(250 + row * 24);
				rectangle(970, y, 300, 23, entry == trigger.eventName ? sf::Color(70, 105, 110) : sf::Color(60, 64, 72));
				Label(window, font, entry == NoTriggerEvent ? "Kein Ereignis" : shortText(triggerDropdown == TriggerDropdown::Maps ? entry.substr(5) : entry), 978, y + 3, 13);
			}
			Label(window, font, "Mausrad: Liste scrollen", 970, 448, 12);
		}
		else
		{
			Label(window, font, "Ein Trigger hat hoechstens ein Ereignis.", 970, 255, 13);
			if (trigger.eventName.starts_with("Load_"))
				Label(window, font, "Zielposition: " + std::to_string(trigger.spawnX) + ", " + std::to_string(trigger.spawnY), 970, 280, 12);
		}
		rectangle(970, 482, 20, 20, sf::Color(185, 190, 195));
		rectangle(972, 484, 16, 16, trigger.glow ? sf::Color(70, 145, 150) : sf::Color(35, 38, 45));
		if (trigger.glow) Label(window, font, "X", 975, 482, 14);
		Label(window, font, "Glow aktivieren", 998, 483, 14);
		Label(window, font, "Laesst die Zone sichtbar pulsieren,", 970, 515, 13);
		Label(window, font, "auch im Release. Nur Darstellung;", 970, 533, 13);
		Label(window, font, "aendert weder Ereignis noch Kollision.", 970, 551, 13);
	}
	void DrawPlacementPreview(sf::RenderWindow& window, float left, float top)
	{
		if (browser.selected.empty() || triggerMode || selectMode || spawnMode) return;
		const sf::Vector2i pixel = sf::Mouse::getPosition(window);
		if (!IsMapArea(EditorPosition(window, pixel))) return;
		sf::Vector2f position = WorldPosition(window, pixel);
		if (newType == MapObjectType::Scenery || newType == MapObjectType::Untergrund)
		{
			position.x = SnapToGrid(position.x, 64.0f);
			position.y = SnapToGrid(position.y, 64.0f);
		}
		if (sf::Texture* texture = GetCachedTexture(browser.selected))
		{
			const sf::Vector2u size = texture->getSize();
			if (size.x == 0u || size.y == 0u) return;
			sf::Sprite sprite(*texture);
			sprite.setOrigin({ size.x * 0.5f, size.y * 0.5f });
			sprite.setScale({ 64.0f / size.x, 64.0f / size.y });
			sprite.setColor(sf::Color(255, 255, 255, 150));
			sprite.setPosition({ position.x - left, position.y - top });
			window.draw(sprite);
		}
	}
	void Draw(sf::RenderWindow& window, const sf::Font& font)
	{
		window.clear(sf::Color(22, 34, 26));
		window.setView(MapView(window));
		Viewport viewport{ static_cast<int>(1000.0f / zoom), static_cast<int>(710.0f / zoom) };
		GameState preview;
		preview.activeMap = document.map;
		preview.player.x = camera.x; preview.player.y = camera.y;
		RenderDefaultGround(window, viewport, preview);
		RenderMapObjects(window, viewport, preview);
		RenderTriggerDebug(window, viewport, preview);
		sf::VertexArray grid(sf::PrimitiveType::Lines);
		const float left = camera.x - viewport.width * 0.5f, top = camera.y - viewport.height * 0.5f;
		const sf::Color gridColor(255, 255, 255, 45);
		for (float x = std::ceil(left / 64) * 64; x < left + viewport.width; x += 64)
		{ const float px = x - left; grid.append(sf::Vertex({ px, 0 }, gridColor)); grid.append(sf::Vertex({ px, static_cast<float>(viewport.height) }, gridColor)); }
		for (float y = std::ceil(top / 64) * 64; y < top + viewport.height; y += 64)
		{ const float py = y - top; grid.append(sf::Vertex({ 0, py }, gridColor)); grid.append(sf::Vertex({ static_cast<float>(viewport.width), py }, gridColor)); }
		window.draw(grid);
		sf::CircleShape marker(8.0f / zoom);
		marker.setOrigin({ 8.0f / zoom, 8.0f / zoom });
		marker.setPosition({ spawn.x - left, spawn.y - top });
		marker.setFillColor(sf::Color::Cyan);
		window.draw(marker);
		if (selectedObject < document.map.objects.size())
		{
			const auto& item = document.map.objects[selectedObject];
			sf::RectangleShape selection({ 64, 64 });
			selection.setOrigin({ 32, 32 });
			selection.setPosition({ item.x - left, item.y - top });
			selection.setFillColor(sf::Color::Transparent);
			selection.setOutlineColor(sf::Color::Yellow);
			selection.setOutlineThickness(2.0f / zoom);
			window.draw(selection);
		}
		if (triggerMode && selectedTrigger < document.map.triggers.size())
		{
			const auto& trigger = document.map.triggers[selectedTrigger];
			sf::RectangleShape selection({ trigger.width, trigger.height });
			selection.setPosition({ trigger.x - left, trigger.y - top });
			selection.setFillColor(sf::Color::Transparent);
			selection.setOutlineColor(sf::Color::Cyan);
			selection.setOutlineThickness(2.0f / zoom);
			window.draw(selection);
			const float handleSize = 8.0f / zoom;
			sf::RectangleShape handle({ handleSize, handleSize });
			handle.setOrigin({ handleSize / 2.0f, handleSize / 2.0f });
			handle.setFillColor(sf::Color::Cyan);
			for (int row = 0; row < 3; ++row)
				for (int column = 0; column < 3; ++column)
				{
					if (row == 1 && column == 1) continue;
					handle.setPosition({ trigger.x - left + trigger.width * column / 2.0f, trigger.y - top + trigger.height * row / 2.0f });
					window.draw(handle);
				}
		}
		DrawPlacementPreview(window, left, top);
		window.setView(EditorView(window));
		sf::RectangleShape bar({ 1280, 60 }); bar.setFillColor(sf::Color(35, 38, 45)); window.draw(bar);
		sf::RectangleShape side({ 280, 740 }); side.setPosition({ 0, 60 }); side.setFillColor(sf::Color(40, 43, 48)); window.draw(side);
		const char* buttons[] = { "Neu", "Oeffnen", "Speichern", "Im Spiel uebernehmen", "Assets neu", "Map Eigenschaften", triggerMode ? "Effekt" : "Typ", triggerMode ? "Glow" : "", "Startpunkt", "Modus" };
		const float positions[] = { 5, 105, 205, 305, 455, 565, 825, 945, 1045, 1155 };
		const float ends[] = { 100, 200, 300, 450, 560, 820, 940, 1040, 1150, 1280 };
		for (int i = 0; i < 10; ++i)
		{
			if (buttons[i][0] == '\0') continue;
			sf::RectangleShape button({ ends[i] - positions[i] - 4, 38 });
			button.setPosition({ positions[i], 5 });
			button.setFillColor(sf::Color(65, 70, 80));
			window.draw(button);
			Label(window, font, buttons[i], positions[i] + 3, 15, 13);
		}
		Label(window, font, "SPRITES (Mausrad)", 8, 70);
		for (int row = 0; row < 17 && browser.offset + row < static_cast<int>(browser.visible.size()); ++row)
		{
			const auto& entry = browser.visible[static_cast<std::size_t>(browser.offset + row)];
			const float y = static_cast<float>(100 + row * 36);
			const float indent = static_cast<float>((std::min)(entry.depth, 6) * 16);
			if (!entry.folder && entry.path == browser.selected) { sf::RectangleShape highlight({ 278, 35 }); highlight.setPosition({ 0, y }); highlight.setFillColor(sf::Color(75, 85, 100)); window.draw(highlight); }
			if (!entry.folder)
			{
				if (sf::Texture* texture = GetCachedTexture(entry.path))
				{
					sf::Sprite sprite(*texture);
					const auto size = texture->getSize();
					if (size.x && size.y) sprite.setScale({ 30.0f / size.x, 30.0f / size.y });
					sprite.setPosition({ 3 + indent, y + 2 }); window.draw(sprite);
				}
			}
			std::string name = entry.folder ? std::string(browser.IsExpanded(entry.path) ? "[-] " : "[+] ") + entry.label : entry.label;
			const std::size_t limit = static_cast<std::size_t>(22 - (std::min)(entry.depth, 6) * 2);
			if (name.size() > limit) name = name.substr(0, limit - 3) + "...";
			Label(window, font, name, (entry.folder ? 8.0f : 38.0f) + indent, y + 7, 12);
		}
		sf::RectangleShape triggerButton({ 270, 36 });
		triggerButton.setPosition({ 5, 720 });
		triggerButton.setFillColor(triggerMode ? sf::Color(70, 105, 110) : sf::Color(65, 70, 80));
		window.draw(triggerButton);
		Label(window, font, "Trigger-Werkzeug (T)", 12, 730, 14);
		DrawTriggerPanel(window, font);
		DrawObjectPanel(window, font);
		DrawMapPropertiesPanel(window, font);
		const MapObject* selected = selectedObject < document.map.objects.size() ? &document.map.objects[selectedObject] : nullptr;
		std::string info = std::string(spawnMode ? "Startpunkt: Klick auf Karte" : selectMode ? "Auswaehlen/Verschieben" : "Platzieren") + " | Typ: " + TypeName(selected ? selected->type : newType) + " | solid=" + ((selected ? selected->solid : newSolid) ? "1" : "0") + " | Wetter: " + document.map.defaultWeather;
		if (selected) info += " | " + selected->id + " (" + std::to_string(static_cast<int>(selected->x)) + ", " + std::to_string(static_cast<int>(selected->y)) + ")";
		if (triggerMode && !spawnMode)
		{
			info = "Trigger: Freie Stelle ziehen = neu | Zone ziehen = verschieben | Entf = loeschen";
			if (selectedTrigger < document.map.triggers.size())
			{
				const auto& trigger = document.map.triggers[selectedTrigger];
				info = trigger.id + " | " + trigger.eventName + " | glow=" + (trigger.glow ? "1" : "0") + " | " + std::to_string(static_cast<int>(trigger.width)) + " x " + std::to_string(static_cast<int>(trigger.height)) + " | Rand ziehen = Groesse";
			}
		}
		Label(window, font, info, 285, 773, 12);
		std::string title = document.path.empty() ? "Keine Karte" : document.path.filename().string();
		if (document.dirty) title += " *";
		window.setTitle("Map-Editor - " + title + " | Spielordner: " + root.string() + " | T: Trigger, Tab: Objekte, P: Startpunkt, Entf: Loeschen, Mitteltaste: Verschieben");
		window.display();
	}
};
#endif
