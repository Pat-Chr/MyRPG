#pragma once

#ifdef _DEBUG
#include <Windows.h>
#include <commdlg.h>
#include <SFML/Graphics.hpp>
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
		sf::RenderWindow window(sf::VideoMode({ 1280u, 800u }), "MyRPG Map-Editor", sf::Style::Titlebar | sf::Style::Close);
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
					if (button->button == sf::Mouse::Button::Left) { draggingObject = false; draggingTrigger = false; creatingTrigger = false; }
					if (button->button == sf::Mouse::Button::Middle) panning = false;
				}
				else if (const auto* move = event->getIf<sf::Event::MouseMoved>())
				{
					if (panning)
					{
						camera.x -= static_cast<float>(move->position.x - lastMouse.x) / zoom;
						camera.y -= static_cast<float>(move->position.y - lastMouse.y) / zoom;
						lastMouse = move->position;
					}
					else if (draggingObject && selectedObject < document.map.objects.size())
					{
						const sf::Vector2f pos = WorldPosition(window, move->position);
						MapObject& object = document.map.objects[selectedObject];
						object.x = object.type == MapObjectType::Scenery || object.type == MapObjectType::Untergrund ? SnapToGrid(pos.x, 64.0f) : pos.x;
						object.y = object.type == MapObjectType::Scenery || object.type == MapObjectType::Untergrund ? SnapToGrid(pos.y, 64.0f) : pos.y;
						document.dirty = true;
					}
					else if ((draggingTrigger || creatingTrigger) && selectedTrigger < document.map.triggers.size())
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
				}
				else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
				{
					if (wheel->position.x < 280)
					{
						const int maxOffset = (std::max)(0, static_cast<int>(browser.visible.size()) - 17);
						browser.offset = std::clamp(browser.offset - (wheel->delta > 0 ? 3 : -3), 0, maxOffset);
					}
					else zoom = std::clamp(zoom * (wheel->delta > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 4.0f);
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
	sf::Vector2i lastMouse{};
	float zoom = 1.0f;
	bool selectMode = false, spawnMode = false, triggerMode = false, panning = false, draggingObject = false, draggingTrigger = false, creatingTrigger = false;
	MapObjectType newType = MapObjectType::Scenery;
	bool newSolid = false;

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
		draggingTrigger = creatingTrigger = false;
	}
	bool Save()
	{
		std::string error;
		if (!document.Save(error)) { Error(error); return false; }
		selectedObject = static_cast<std::size_t>(-1);
		selectedTrigger = static_cast<std::size_t>(-1);
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
		for (const TriggerZone& trigger : loaded.triggers) warning = warning || IsInsideTrigger(trigger, spawn.x, spawn.y);
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
	sf::View MapView() const
	{
		sf::View view;
		view.setSize({ 1000.0f / zoom, 710.0f / zoom });
		view.setCenter({ 500.0f / zoom, 355.0f / zoom });
		view.setViewport({ { 280.0f / 1280.0f, 60.0f / 800.0f }, { 1000.0f / 1280.0f, 710.0f / 800.0f } });
		return view;
	}
	sf::Vector2f WorldPosition(const sf::RenderWindow& window, sf::Vector2i pixel) const
	{
		const sf::Vector2f pos = window.mapPixelToCoords(pixel, MapView());
		return { pos.x + camera.x - 500.0f / zoom, pos.y + camera.y - 355.0f / zoom };
	}
	void ToggleSelectMode()
	{
		triggerMode = false;
		selectedTrigger = static_cast<std::size_t>(-1);
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
		if (key == sf::Keyboard::Key::Escape) { if (ConfirmDiscard()) window.close(); }
		else if (key == sf::Keyboard::Key::Tab) ToggleSelectMode();
		else if (key == sf::Keyboard::Key::P) { spawnMode = true; }
		else if (key == sf::Keyboard::Key::T)
		{
			triggerMode = !triggerMode; selectMode = false; spawnMode = false;
			selectedObject = selectedTrigger = static_cast<std::size_t>(-1);
		}
		else if (key == sf::Keyboard::Key::Delete && triggerMode)
		{
			if (selectedTrigger < document.map.triggers.size())
			{
				document.map.triggers.erase(document.map.triggers.begin() + selectedTrigger);
				selectedTrigger = static_cast<std::size_t>(-1);
				draggingTrigger = creatingTrigger = false;
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
				if (pixel.x >= 280 && pixel.x < 1280 && pixel.y >= 60 && pixel.y < 770)
				{
					const sf::Vector2f position = WorldPosition(window, pixel);
					target = document.FindObjectAt(position.x, position.y);
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
		const int x = event.position.x, y = event.position.y;
		if (event.button == sf::Mouse::Button::Middle && x >= 280 && y >= 60 && y < 770)
		{ panning = true; lastMouse = event.position; return; }
		if (event.button != sf::Mouse::Button::Left) return;
		if (y < 50)
		{
			if (x < 100) Open(true);
			else if (x < 200) Open(false);
			else if (x < 300) Save();
			else if (x < 450) Apply(game);
			else if (x < 560) RefreshAssets();
			else if (x < 700)
			{
				if (document.path.empty() || browser.selected.empty()) Error("Bitte zuerst eine Karte und ein Sprite auswaehlen.");
				else { document.map.defaultGroundPath = browser.selected; document.dirty = true; }
			}
			else if (x < 820)
			{
				if (document.path.empty()) { Error("Bitte zuerst eine Karte oeffnen oder erstellen."); return; }
				const auto& weather = GetWeatherEffectDefinitions();
				auto current = std::find_if(weather.begin(), weather.end(), [&](const WeatherEffectDefinition& item) { return item.id == document.map.defaultWeather; });
				document.map.defaultWeather = document.map.defaultWeather == "none" && !weather.empty() ? weather.front().id : current != weather.end() && ++current != weather.end() ? current->id : "none";
				document.dirty = true;
			}
			else if (x < 940 && triggerMode)
			{
				if (selectedTrigger >= document.map.triggers.size()) { Error("Bitte zuerst eine Triggerzone auswaehlen."); return; }
				std::vector<std::string> effects{ "screen_shake", "weather_none" };
				for (const auto& effect : GetMagicEffectDefinitions()) effects.push_back("magic_" + effect.id);
				for (const auto& effect : GetWeatherEffectDefinitions()) effects.push_back("weather_" + effect.id);
				auto& eventName = document.map.triggers[selectedTrigger].eventName;
				auto current = std::find(effects.begin(), effects.end(), eventName);
				if (current == effects.end() || ++current == effects.end()) eventName = effects.front();
				else eventName = *current;
				document.dirty = true;
			}
			else if (x < 1040 && triggerMode)
			{
				if (selectedTrigger >= document.map.triggers.size()) { Error("Bitte zuerst eine Triggerzone auswaehlen."); return; }
				auto& trigger = document.map.triggers[selectedTrigger]; trigger.glow = !trigger.glow; document.dirty = true;
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
			else if (x < 1040)
			{
				if (selectedObject < document.map.objects.size()) { document.map.objects[selectedObject].solid = !document.map.objects[selectedObject].solid; document.dirty = true; }
				else newSolid = !newSolid;
			}
			else if (x < 1150) spawnMode = true;
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
				else { browser.selected = entry.path; selectedObject = selectedTrigger = static_cast<std::size_t>(-1); selectMode = triggerMode = spawnMode = false; }
			}
			return;
		}
		if (x < 280 || y < 60 || y >= 770 || document.path.empty()) return;
		sf::Vector2f pos = WorldPosition(window, event.position);
		if (spawnMode) { spawn = pos; spawnMode = false; return; }
		if (triggerMode)
		{
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
		std::size_t number = 1;
		std::string id;
		do { id = "object_" + std::to_string(number++); } while (document.HasUsedId(id));
		if (newType == MapObjectType::Scenery || newType == MapObjectType::Untergrund) { pos.x = SnapToGrid(pos.x, 64); pos.y = SnapToGrid(pos.y, 64); }
		document.map.objects.push_back({ id, newType, pos.x, pos.y, browser.selected, newSolid });
		document.dirty = true;
	}
	static const char* TypeName(MapObjectType type)
	{
		return type == MapObjectType::Untergrund ? "untergrund" : type == MapObjectType::Wall ? "wall" : type == MapObjectType::Npc ? "npc" : "scenery";
	}
	static void Label(sf::RenderTarget& target, const sf::Font& font, const std::string& text, float x, float y, unsigned int size = 14)
	{
		sf::Text label(font, text, size);
		label.setPosition({ x, y });
		target.draw(label);
	}
	void Draw(sf::RenderWindow& window, const sf::Font& font)
	{
		window.clear(sf::Color(22, 34, 26));
		window.setView(MapView());
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
		}
		window.setView(window.getDefaultView());
		sf::RectangleShape bar({ 1280, 60 }); bar.setFillColor(sf::Color(35, 38, 45)); window.draw(bar);
		sf::RectangleShape side({ 280, 740 }); side.setPosition({ 0, 60 }); side.setFillColor(sf::Color(40, 43, 48)); window.draw(side);
		const char* buttons[] = { "Neu", "Oeffnen", "Speichern", "Im Spiel uebernehmen", "Assets neu", "Boden setzen", "Wetter", triggerMode ? "Effekt" : "Typ", triggerMode ? "Glow" : "Solid", "Startpunkt", "Modus" };
		const float positions[] = { 5, 105, 205, 305, 455, 565, 705, 825, 945, 1045, 1155 };
		const float ends[] = { 100, 200, 300, 450, 560, 700, 820, 940, 1040, 1150, 1280 };
		for (int i = 0; i < 11; ++i)
		{
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
		const MapObject* selected = selectedObject < document.map.objects.size() ? &document.map.objects[selectedObject] : nullptr;
		std::string info = std::string(spawnMode ? "Startpunkt: Klick auf Karte" : selectMode ? "Auswaehlen/Verschieben" : "Platzieren") + " | Typ: " + TypeName(selected ? selected->type : newType) + " | solid=" + ((selected ? selected->solid : newSolid) ? "1" : "0") + " | Wetter: " + document.map.defaultWeather;
		if (selected) info += " | " + selected->id + " (" + std::to_string(static_cast<int>(selected->x)) + ", " + std::to_string(static_cast<int>(selected->y)) + ")";
		if (triggerMode && !spawnMode)
		{
			info = "Trigger: Freie Stelle ziehen = neu | Zone ziehen = verschieben | Entf = loeschen";
			if (selectedTrigger < document.map.triggers.size())
			{
				const auto& trigger = document.map.triggers[selectedTrigger];
				info = trigger.id + " | " + trigger.eventName + " | glow=" + (trigger.glow ? "1" : "0") + " | " + std::to_string(static_cast<int>(trigger.width)) + " x " + std::to_string(static_cast<int>(trigger.height));
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
