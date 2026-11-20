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
				if (event->is<sf::Event::Closed>() && ConfirmEditorClose()) gameWindow.close();
			while (const std::optional event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					if (ConfirmEditorClose()) window.close();
				}
				else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) HandleKey(key->code, window);
				else if (const auto* text = event->getIf<sf::Event::TextEntered>())
				{
					if (renameTriggerOpen)
					{
						if (text->unicode == 8 && !triggerNameInput.empty()) triggerNameInput.pop_back();
						else if (text->unicode >= 32 && text->unicode < 127 && triggerNameInput.size() < 64) triggerNameInput += static_cast<char>(text->unicode);
					}
				}
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
					if (renameTriggerOpen) continue;
					if (pickingTarget)
					{
						if (IsMapArea(position)) zoom = std::clamp(zoom * (wheel->delta > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 4.0f);
					}
					else if (mapPropertiesOpen && position.x >= 440 && position.x < 820 && position.y >= 330 && position.y < 600)
					{
						const int weatherCount = static_cast<int>(GetWeatherEffectDefinitions().size()) + 1;
						mapWeatherOffset = std::clamp(mapWeatherOffset - (wheel->delta > 0 ? 1 : -1), 0, (std::max)(0, weatherCount - 9));
					}
					else if (mapPropertiesOpen)
					{
						// The map properties dialog handles scrolling only over its weather list.
					}
					else if (TriggerPanelVisible() && position.x >= 960 && position.y >= 70 && position.y < 660)
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
			if (window.hasFocus() && !mapPropertiesOpen && !renameTriggerOpen)
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
			PollSheetEvents(window);
			PanSheet(panDelta);
			DrawSheetWindow(font);
		}
		sheetWindow.reset();
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
	bool renameTriggerOpen = false;
	std::string triggerNameInput;
	bool pickingTarget = false;
	MapData targetMap;
	sf::Vector2f sourceCamera{};
	float sourceZoom = 1.0f;
	bool selectMode = false, spawnMode = false, triggerMode = false, panning = false, draggingObject = false, draggingTrigger = false, creatingTrigger = false;
	enum TriggerEdge { Left = 1, Right = 2, Top = 4, Bottom = 8 };
	int resizeEdges = 0;
	MapObjectType newType = MapObjectType::Scenery;
	bool newSolid = false;
	enum class TriggerDropdown { None, Maps, Effects, Weather };
	TriggerDropdown triggerDropdown = TriggerDropdown::None;
	std::vector<std::string> dropdownEntries;
	int dropdownOffset = 0;

	// Spritesheet-Ansicht: eigenes Fenster, Inhaltsbereich ab y=70 (Fensterpixel)
	static constexpr float SheetContentY = 70.0f;
	std::optional<sf::RenderWindow> sheetWindow;
	bool sheetDragging = false, brushTile = false, sheetPanning = false;
	int sheetCell = 16;
	float sheetScale = 4.0f, sheetScrollX = 0.0f, sheetScrollY = 0.0f;
	sf::Vector2f lastSheetMouse{};
	sf::IntRect brushRect{};

	void ResetSheetView()
	{
		sheetScale = 64.0f / static_cast<float>(sheetCell);
		sheetScrollX = sheetScrollY = 0.0f;
	}
	void ClampSheetScroll(sf::Vector2u size)
	{
		if (!sheetWindow) return;
		const sf::Vector2u view = sheetWindow->getSize();
		sheetScrollX = std::clamp(sheetScrollX, 64.0f - static_cast<float>(view.x), (std::max)(0.0f, size.x * sheetScale - 64.0f));
		sheetScrollY = std::clamp(sheetScrollY, 64.0f + SheetContentY - static_cast<float>(view.y), (std::max)(0.0f, size.y * sheetScale - 64.0f));
	}
	void SetSheetCell(int value)
	{
		sheetCell = std::clamp(value, 1, 512);
		brushTile = false;
		ResetSheetView();
	}
	void ToggleSheet()
	{
		if (sheetWindow)
		{
			sheetWindow.reset();
			sheetDragging = sheetPanning = false;
			return;
		}
		if (browser.selected.empty()) { Error("Bitte zuerst ein Spritesheet auswaehlen."); return; }
		sheetWindow.emplace(sf::VideoMode({ 900u, 720u }), "MyRPG Sheet-Ansicht", sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close);
		sheetWindow->setFramerateLimit(60);
		sheetWindow->setKeyRepeatEnabled(false);
		ResetSheetView();
	}
	// Liefert den Tile-Ausschnitt unter dem Mauszeiger (Fensterpixel), falls vorhanden
	std::optional<sf::IntRect> SheetTileAt(sf::Vector2f p)
	{
		if (p.x < 0.0f || p.y < SheetContentY) return std::nullopt;
		sf::Texture* texture = GetCachedTexture(browser.selected);
		if (!texture) return std::nullopt;
		const sf::Vector2u size = texture->getSize();
		const int tx = static_cast<int>(std::floor((p.x + sheetScrollX) / sheetScale));
		const int ty = static_cast<int>(std::floor((p.y - SheetContentY + sheetScrollY) / sheetScale));
		if (tx < 0 || ty < 0 || tx >= static_cast<int>(size.x) || ty >= static_cast<int>(size.y)) return std::nullopt;
		const int cx = tx / sheetCell * sheetCell, cy = ty / sheetCell * sheetCell;
		return sf::IntRect({ cx, cy }, { (std::min)(sheetCell, static_cast<int>(size.x) - cx), (std::min)(sheetCell, static_cast<int>(size.y) - cy) });
	}
	void HandleSheetClick(sf::Vector2f p)
	{
		if (p.y < SheetContentY)
		{
			if (p.y < 6 || p.y >= 34) return;
			if (p.x >= 70 && p.x < 100) SetSheetCell(sheetCell - 1);
			else if (p.x >= 150 && p.x < 180) SetSheetCell(sheetCell + 1);
			else if (p.x >= 195 && p.x < 375)
			{
				const int index = static_cast<int>((p.x - 195) / 45);
				constexpr int presets[] = { 8, 16, 32, 64 };
				if (index >= 0 && index < 4 && std::fmod(p.x - 195, 45.0f) < 40.0f) SetSheetCell(presets[index]);
			}
			return;
		}
		if (const auto tile = SheetTileAt(p))
		{
			brushRect = *tile;
			brushTile = true;
			sheetDragging = true;
			selectedObject = selectedTrigger = static_cast<std::size_t>(-1);
			selectMode = triggerMode = spawnMode = false;
			draggingTrigger = creatingTrigger = false;
			resizeEdges = 0;
		}
	}
	// releasePixel liegt in Sheet-Fensterkoordinaten; ausserhalb des Sheet-Fensters wird auf die Karte gesetzt
	void HandleSheetRelease(const sf::RenderWindow& window, sf::Vector2i releasePixel)
	{
		if (!sheetDragging) return;
		sheetDragging = false;
		const sf::Vector2u sheetSize = sheetWindow ? sheetWindow->getSize() : sf::Vector2u{};
		if (releasePixel.x >= 0 && releasePixel.y >= 0 && releasePixel.x < static_cast<int>(sheetSize.x) && releasePixel.y < static_cast<int>(sheetSize.y)) return;
		const sf::Vector2i pixel = sf::Mouse::getPosition(window);
		const sf::Vector2u size = window.getSize();
		if (pixel.x < 0 || pixel.y < 0 || pixel.x >= static_cast<int>(size.x) || pixel.y >= static_cast<int>(size.y)) return;
		const sf::Vector2f position = EditorPosition(window, pixel);
		if (mapPropertiesOpen || !IsMapArea(position)) return;
		if (ObjectPanelVisible() && position.x >= 960 && position.y >= 70 && position.y < 270) return;
		if (document.path.empty()) return;
		hasLastPlacementPosition = false;
		PlaceSelectedSprite(WorldPosition(window, pixel));
		placementDragActive = false;
		hasLastPlacementPosition = false;
	}
	void PollSheetEvents(const sf::RenderWindow& mainWindow)
	{
		while (sheetWindow && sheetWindow->isOpen())
		{
			const std::optional event = sheetWindow->pollEvent();
			if (!event) break;
			if (event->is<sf::Event::Closed>()) { sheetWindow.reset(); sheetDragging = sheetPanning = false; return; }
			if (const auto* resized = event->getIf<sf::Event::Resized>())
				sheetWindow->setView(sf::View(sf::FloatRect({ 0.0f, 0.0f }, { static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) })));
			else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
			{
				if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::G) { ToggleSheet(); return; }
			}
			else if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>())
			{
				const sf::Vector2f position = sheetWindow->mapPixelToCoords(press->position);
				if (press->button == sf::Mouse::Button::Left) HandleSheetClick(position);
				else if (press->button == sf::Mouse::Button::Middle && position.y >= SheetContentY) { sheetPanning = true; lastSheetMouse = position; }
			}
			else if (const auto* release = event->getIf<sf::Event::MouseButtonReleased>())
			{
				if (release->button == sf::Mouse::Button::Left) HandleSheetRelease(mainWindow, release->position);
				else if (release->button == sf::Mouse::Button::Middle) sheetPanning = false;
			}
			else if (const auto* move = event->getIf<sf::Event::MouseMoved>())
			{
				const sf::Vector2f position = sheetWindow->mapPixelToCoords(move->position);
				if (sheetPanning)
				{
					sheetScrollX -= position.x - lastSheetMouse.x;
					sheetScrollY -= position.y - lastSheetMouse.y;
					lastSheetMouse = position;
				}
			}
			else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
			{
				const sf::Vector2f position = sheetWindow->mapPixelToCoords(wheel->position);
				if (position.y < SheetContentY) continue;
				// Zoom um den Mauszeiger, sodass der Punkt unter der Maus stehen bleibt
				const float oldScale = sheetScale;
				sheetScale = std::clamp(sheetScale * (wheel->delta > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 32.0f);
				const float ratio = sheetScale / oldScale;
				sheetScrollX = (position.x + sheetScrollX) * ratio - position.x;
				sheetScrollY = (position.y - SheetContentY + sheetScrollY) * ratio - (position.y - SheetContentY);
			}
		}
	}
	void PanSheet(float delta)
	{
		if (!sheetWindow || !sheetWindow->hasFocus()) return;
		const float panX = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) -
			static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left));
		const float panY = static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) -
			static_cast<float>(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up));
		const float length = std::sqrt(panX * panX + panY * panY);
		if (length <= 0.0f) return;
		sheetScrollX += panX * 500.0f * delta / length;
		sheetScrollY += panY * 500.0f * delta / length;
	}
	// Halbtransparente Vorschau des gezogenen Tiles im Haupteditor
	void DrawSheetGhost(sf::RenderWindow& window)
	{
		if (!sheetDragging || !brushTile) return;
		sf::Texture* texture = GetCachedTexture(browser.selected);
		if (!texture || brushRect.size.x <= 0 || brushRect.size.y <= 0) return;
		const sf::Vector2f mouse = EditorPosition(window, sf::Mouse::getPosition(window));
		if (!IsMapArea(mouse)) return;
		sf::Sprite ghost(*texture);
		ghost.setTextureRect(brushRect);
		ghost.setScale({ 64.0f / brushRect.size.x, 64.0f / brushRect.size.y });
		ghost.setOrigin({ brushRect.size.x * 0.5f, brushRect.size.y * 0.5f });
		ghost.setColor(sf::Color(255, 255, 255, 170));
		ghost.setPosition(mouse);
		window.draw(ghost);
	}
	void DrawSheetWindow(const sf::Font& font)
	{
		if (!sheetWindow) return;
		sf::RenderWindow& window = *sheetWindow;
		window.clear(sf::Color(30, 33, 40));
		sf::Texture* texture = GetCachedTexture(browser.selected);
		if (texture)
		{
			const sf::Vector2u size = texture->getSize();
			ClampSheetScroll(size);
			const sf::Vector2f origin{ -sheetScrollX, SheetContentY - sheetScrollY };
			sf::RectangleShape backdrop({ size.x * sheetScale, size.y * sheetScale });
			backdrop.setPosition(origin);
			backdrop.setFillColor(sf::Color(55, 58, 66));
			window.draw(backdrop);
			sf::Sprite sprite(*texture);
			sprite.setPosition(origin);
			sprite.setScale({ sheetScale, sheetScale });
			window.draw(sprite);
			sf::VertexArray lines(sf::PrimitiveType::Lines);
			const sf::Color lineColor(255, 255, 255, 70);
			const float step = sheetCell * sheetScale, width = size.x * sheetScale, height = size.y * sheetScale;
			for (float x = 0; x <= width + 0.5f; x += step) { lines.append(sf::Vertex({ origin.x + x, origin.y }, lineColor)); lines.append(sf::Vertex({ origin.x + x, origin.y + height }, lineColor)); }
			for (float y = 0; y <= height + 0.5f; y += step) { lines.append(sf::Vertex({ origin.x, origin.y + y }, lineColor)); lines.append(sf::Vertex({ origin.x + width, origin.y + y }, lineColor)); }
			window.draw(lines);
			auto outline = [&](const sf::IntRect& rect, sf::Color color)
			{
				sf::RectangleShape box({ rect.size.x * sheetScale, rect.size.y * sheetScale });
				box.setPosition({ origin.x + rect.position.x * sheetScale, origin.y + rect.position.y * sheetScale });
				box.setFillColor(sf::Color(color.r, color.g, color.b, 40));
				box.setOutlineColor(color);
				box.setOutlineThickness(2.0f);
				window.draw(box);
			};
			if (brushTile) outline(brushRect, sf::Color::Yellow);
			if (const auto hover = SheetTileAt(window.mapPixelToCoords(sf::Mouse::getPosition(window)))) outline(*hover, sf::Color::Cyan);
		}
		sf::RectangleShape bar({ static_cast<float>(window.getSize().x), SheetContentY });
		bar.setFillColor(sf::Color(35, 38, 45));
		window.draw(bar);
		auto button = [&](float x, float w, const std::string& text, bool active = false)
		{
			sf::RectangleShape shape({ w, 28 });
			shape.setPosition({ x, 6 });
			shape.setFillColor(active ? sf::Color(70, 105, 110) : sf::Color(65, 70, 80));
			window.draw(shape);
			Label(window, font, text, x + 6, 10, 14);
		};
		Label(window, font, "Raster:", 8, 10, 14);
		button(70, 30, "-");
		Label(window, font, std::to_string(sheetCell), 110, 10, 14);
		button(150, 30, "+");
		constexpr int presets[] = { 8, 16, 32, 64 };
		for (int i = 0; i < 4; ++i) button(195.0f + 45.0f * i, 40, std::to_string(presets[i]), presets[i] == sheetCell);
		char info[96];
		std::snprintf(info, sizeof info, "Zelle %d px -> 64 px im Spiel | Ansicht x%.2f", sheetCell, sheetScale);
		Label(window, font, info, 8, 40, 12);
		Label(window, font, "Klick = Tile waehlen, ziehen = auf Karte setzen | WASD/Pfeile/Mitteltaste = bewegen | Mausrad = Zoom", 8, 54, 11);
		window.display();
	}

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
	bool ConfirmEditorClose() const
	{
		return document.dirty ? ConfirmDiscard() :
			MessageBoxW(owner, L"Map-Editor wirklich schliessen?", L"Map-Editor", MB_YESNO | MB_ICONQUESTION) == IDYES;
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
		else if (type == TriggerDropdown::Effects)
		{
			entries = { "screen_shake" };
			for (const auto& effect : GetMagicEffectDefinitions()) entries.push_back("magic_" + effect.id);
		}
		else
		{
			entries = { "weather_none" };
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
	void BeginTargetPick()
	{
		const auto& trigger = document.map.triggers[selectedTrigger];
		std::string relativePath;
		if (!TryResolveMapTransitionTarget(trigger.eventName, relativePath))
		{
			Error("Ungueltige Zielkarte fuer diesen Trigger.");
			return;
		}
		const std::filesystem::path targetPath = (root / relativePath).lexically_normal();
		MapData loaded;
		if (targetPath == document.path.lexically_normal()) loaded = document.map;
		else if (!TryLoadMapFromFile(targetPath.string(), loaded))
		{
			Error("Die Zielkarte konnte nicht geladen werden: " + targetPath.string());
			return;
		}
		targetMap = std::move(loaded);
		sourceCamera = camera;
		sourceZoom = zoom;
		camera = { trigger.spawnX, trigger.spawnY };
		pickingTarget = true;
		panning = draggingObject = draggingTrigger = creatingTrigger = false;
		resizeEdges = 0;
		placementDragActive = hasLastPlacementPosition = false;
		sheetWindow.reset();
		sheetDragging = sheetPanning = false;
	}
	void EndTargetPick()
	{
		pickingTarget = false;
		panning = false;
		camera = sourceCamera;
		zoom = sourceZoom;
		targetMap = MapData{};
	}
	void ConfirmTriggerName()
	{
		std::string error;
		if (!document.RenameTrigger(selectedTrigger, triggerNameInput, error)) { Error(error); return; }
		renameTriggerOpen = false;
	}
	void HandleTriggerPanelClick(int x, int y)
	{
		if (x < 970 || x >= 1270) return;
		if (x >= 1165 && y >= 75 && y < 100)
		{
			triggerNameInput = document.map.triggers[selectedTrigger].id;
			renameTriggerOpen = true;
			triggerDropdown = TriggerDropdown::None;
			panning = draggingObject = draggingTrigger = creatingTrigger = false;
			resizeEdges = 0;
			sheetWindow.reset();
			sheetDragging = sheetPanning = false;
		}
		else if (y >= 150 && y < 180) OpenTriggerDropdown(TriggerDropdown::Maps);
		else if (y >= 205 && y < 235) OpenTriggerDropdown(TriggerDropdown::Effects);
		else if (y >= 260 && y < 290) OpenTriggerDropdown(TriggerDropdown::Weather);
		else if (triggerDropdown == TriggerDropdown::None && y >= 340 && y < 374 && document.map.triggers[selectedTrigger].eventName.starts_with("Load_")) BeginTargetPick();
		else if (y >= 530 && y < 555) ToggleTriggerGlow();
		else if (triggerDropdown != TriggerDropdown::None && y >= 300 && y < 492)
		{
			const std::size_t index = static_cast<std::size_t>(dropdownOffset + (y - 300) / 24);
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
		MapObject object{ id, newType, position.x, position.y, browser.selected, newSolid };
		if (brushTile)
		{
			object.hasTile = true;
			object.tileX = brushRect.position.x; object.tileY = brushRect.position.y;
			object.tileWidth = brushRect.size.x; object.tileHeight = brushRect.size.y;
		}
		document.map.objects.push_back(object);
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
		const int width = static_cast<int>(1000.0f / zoom), height = static_cast<int>(710.0f / zoom);
		return { pos.x + camera.x - width * 0.5f, pos.y + camera.y - height * 0.5f };
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
		if (renameTriggerOpen)
		{
			if (key == sf::Keyboard::Key::Escape) renameTriggerOpen = false;
			else if (key == sf::Keyboard::Key::Enter) ConfirmTriggerName();
			return;
		}
		if (pickingTarget)
		{
			if (key == sf::Keyboard::Key::Escape) EndTargetPick();
			return;
		}
		if (mapPropertiesOpen)
		{
			if (key == sf::Keyboard::Key::Escape) mapPropertiesOpen = false;
			return;
		}
		if (key == sf::Keyboard::Key::Escape) { if (ConfirmEditorClose()) window.close(); }
		else if (key == sf::Keyboard::Key::Tab) ToggleSelectMode();
		else if (key == sf::Keyboard::Key::G) ToggleSheet();
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
		if (renameTriggerOpen)
		{
			if (event.button == sf::Mouse::Button::Left && y >= 460 && y < 500)
			{
				if (x >= 450 && x < 625) ConfirmTriggerName();
				else if (x >= 655 && x < 830) renameTriggerOpen = false;
			}
			return;
		}
		if (pickingTarget)
		{
			if (event.button == sf::Mouse::Button::Middle && IsMapArea(position)) { panning = true; lastMouse = position; }
			else if (event.button == sf::Mouse::Button::Left && IsMapArea(position))
			{
				const sf::Vector2f pos = WorldPosition(window, event.position);
				GameState test;
				test.activeMap = targetMap;
				bool warning = IsSolidObjectBlockingPosition(test, pos.x, pos.y);
				for (const TriggerZone& zone : targetMap.triggers) warning = warning || (zone.eventName != NoTriggerEvent && IsInsideTrigger(zone, pos.x, pos.y));
				if (warning && MessageBoxW(owner, L"Die Zielposition liegt in einem Hindernis oder Trigger. Trotzdem waehlen?", L"Map-Editor", MB_YESNO | MB_ICONWARNING) != IDYES) return;
				auto& trigger = document.map.triggers[selectedTrigger];
				if (trigger.spawnX != pos.x || trigger.spawnY != pos.y)
				{
					trigger.spawnX = pos.x; trigger.spawnY = pos.y;
					document.dirty = true;
				}
				EndTargetPick();
			}
			return;
		}
		if (mapPropertiesOpen)
		{
			if (event.button == sf::Mouse::Button::Left) HandleMapPropertiesClick(position);
			return;
		}
		if (TriggerPanelVisible() && x >= 960 && y >= 70 && y < 660)
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
		if (x < 280 && y >= 764 && y < 796)
		{
			ToggleSheet();
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
				else { browser.selected = entry.path; brushTile = false; sheetScrollX = sheetScrollY = 0.0f; sheetDragging = false; selectedObject = selectedTrigger = static_cast<std::size_t>(-1); selectMode = triggerMode = spawnMode = false; draggingTrigger = creatingTrigger = false; resizeEdges = 0; placementDragActive = true; }
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
		rectangle(960, 70, 320, 590, sf::Color(35, 38, 45, 245));
		const bool noEvent = trigger.eventName == NoTriggerEvent;
		const bool mapEvent = trigger.eventName.starts_with("Load_");
		const bool weatherEvent = trigger.eventName.starts_with("weather_");
		const std::string title = trigger.id.size() > 15 ? trigger.id.substr(0, 12) + "..." : trigger.id;
		Label(window, font, "TRIGGER: " + title, 970, 80, 14);
		rectangle(1165, 75, 105, 25, sf::Color(65, 70, 80));
		Label(window, font, "Umbenennen", 1170, 80, 12);
		Label(window, font, "Ereignis: " + (noEvent ? std::string("Kein Ereignis") : shortText(trigger.eventName)), 970, 105, 13);
		Label(window, font, "Load Map", 970, 130, 13);
		rectangle(970, 150, 300, 30, sf::Color(65, 70, 80));
		Label(window, font, noEvent ? "Kein Ereignis  v" : mapEvent ? shortText(trigger.eventName.substr(5)) + "  v" : "Karte auswaehlen  v", 978, 156, 13);
		Label(window, font, "Effekte", 970, 185, 13);
		rectangle(970, 205, 300, 30, sf::Color(65, 70, 80));
		Label(window, font, noEvent ? "Kein Ereignis  v" : !mapEvent && !weatherEvent ? shortText(trigger.eventName) + "  v" : "Effekt auswaehlen  v", 978, 211, 13);
		Label(window, font, "Wetter", 970, 240, 13);
		rectangle(970, 260, 300, 30, sf::Color(65, 70, 80));
		Label(window, font, noEvent ? "Kein Ereignis  v" : weatherEvent ? shortText(trigger.eventName.substr(8)) + "  v" : "Wetter auswaehlen  v", 978, 266, 13);
		if (triggerDropdown != TriggerDropdown::None)
		{
			for (int row = 0; row < 8 && dropdownOffset + row < static_cast<int>(dropdownEntries.size()); ++row)
			{
				const auto& entry = dropdownEntries[static_cast<std::size_t>(dropdownOffset + row)];
				const float y = static_cast<float>(300 + row * 24);
				rectangle(970, y, 300, 23, entry == trigger.eventName ? sf::Color(70, 105, 110) : sf::Color(60, 64, 72));
				const std::string label = entry == NoTriggerEvent ? "Kein Ereignis" :
					triggerDropdown == TriggerDropdown::Maps ? entry.substr(5) :
					triggerDropdown == TriggerDropdown::Weather ? entry.substr(8) : entry;
				Label(window, font, shortText(label), 978, y + 3, 13);
			}
			Label(window, font, "Mausrad: Liste scrollen", 970, 500, 12);
		}
		else
		{
			Label(window, font, "Ein Trigger hat hoechstens ein Ereignis.", 970, 300, 13);
			if (mapEvent)
			{
				Label(window, font, "Zielposition: " + std::to_string(trigger.spawnX) + ", " + std::to_string(trigger.spawnY), 970, 320, 12);
				rectangle(970, 340, 300, 34, sf::Color(65, 70, 80));
				Label(window, font, "Zielposition auf Karte waehlen", 978, 349, 13);
			}
		}
		rectangle(970, 530, 20, 20, sf::Color(185, 190, 195));
		rectangle(972, 532, 16, 16, trigger.glow ? sf::Color(70, 145, 150) : sf::Color(35, 38, 45));
		if (trigger.glow) Label(window, font, "X", 975, 530, 14);
		Label(window, font, "Glow aktivieren", 998, 531, 14);
		Label(window, font, "Laesst die Zone sichtbar pulsieren,", 970, 563, 13);
		Label(window, font, "auch im Release. Nur Darstellung;", 970, 581, 13);
		Label(window, font, "aendert weder Ereignis noch Kollision.", 970, 599, 13);
	}
	void DrawRenameTriggerDialog(sf::RenderWindow& window, const sf::Font& font) const
	{
		if (!renameTriggerOpen) return;
		auto rectangle = [&](float x, float y, float width, float height, sf::Color color)
		{
			sf::RectangleShape shape({ width, height }); shape.setPosition({ x, y }); shape.setFillColor(color); window.draw(shape);
		};
		rectangle(420, 270, 440, 250, sf::Color(28, 31, 37, 250));
		rectangle(420, 270, 440, 48, sf::Color(48, 52, 60));
		Label(window, font, "TRIGGER UMBENENNEN", 440, 285, 18);
		Label(window, font, "Name (Buchstaben, Ziffern, _ und -):", 450, 335, 14);
		rectangle(450, 365, 380, 36, sf::Color(65, 70, 80));
		const std::string visible = triggerNameInput.size() > 38 ? "..." + triggerNameInput.substr(triggerNameInput.size() - 35) : triggerNameInput;
		Label(window, font, visible + "_", 458, 374, 14);
		Label(window, font, "Enter: Uebernehmen | Esc: Abbrechen", 450, 420, 13);
		rectangle(450, 460, 175, 40, sf::Color(70, 105, 110));
		rectangle(655, 460, 175, 40, sf::Color(65, 70, 80));
		Label(window, font, "Uebernehmen", 460, 471, 14);
		Label(window, font, "Abbrechen", 665, 471, 14);
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
			sf::Vector2u size = texture->getSize();
			sf::Sprite sprite(*texture);
			if (brushTile)
			{
				sprite.setTextureRect(brushRect);
				size = { static_cast<unsigned>(brushRect.size.x), static_cast<unsigned>(brushRect.size.y) };
			}
			if (size.x == 0u || size.y == 0u) return;
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
		preview.activeMap = pickingTarget ? targetMap : document.map;
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
		const sf::Vector2f markerPosition = pickingTarget ? sf::Vector2f{ document.map.triggers[selectedTrigger].spawnX, document.map.triggers[selectedTrigger].spawnY } : spawn;
		marker.setPosition({ markerPosition.x - left, markerPosition.y - top });
		marker.setFillColor(sf::Color::Cyan);
		window.draw(marker);
		if (!pickingTarget && selectedObject < document.map.objects.size())
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
		if (!pickingTarget && triggerMode && selectedTrigger < document.map.triggers.size())
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
		if (!pickingTarget) DrawPlacementPreview(window, left, top);
		window.setView(EditorView(window));
		if (pickingTarget)
		{
			sf::RectangleShape bar({ 1280, 60 }); bar.setFillColor(sf::Color(35, 38, 45)); window.draw(bar);
			sf::RectangleShape side({ 280, 740 }); side.setPosition({ 0, 60 }); side.setFillColor(sf::Color(40, 43, 48)); window.draw(side);
			Label(window, font, "ZIELKARTE: " + document.map.triggers[selectedTrigger].eventName.substr(5), 10, 15, 16);
			Label(window, font, "Klick: Zielposition waehlen", 10, 80, 13);
			Label(window, font, "Esc: Abbrechen", 10, 100, 13);
			Label(window, font, "WASD/Pfeile oder Mitteltaste:", 10, 130, 12);
			Label(window, font, "Ansicht bewegen", 10, 148, 12);
			Label(window, font, "Mausrad: Zoom", 10, 170, 12);
			window.setTitle("Map-Editor - Zielposition waehlen");
			window.display();
			return;
		}
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
		sf::RectangleShape sheetButton({ 270, 32 });
		sheetButton.setPosition({ 5, 764 });
		sheetButton.setFillColor(sheetWindow ? sf::Color(70, 105, 110) : sf::Color(65, 70, 80));
		window.draw(sheetButton);
		Label(window, font, "Sheet-Ansicht (G)", 12, 772, 14);
		DrawTriggerPanel(window, font);
		DrawObjectPanel(window, font);
		DrawSheetGhost(window);
		DrawMapPropertiesPanel(window, font);
		DrawRenameTriggerDialog(window, font);
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
