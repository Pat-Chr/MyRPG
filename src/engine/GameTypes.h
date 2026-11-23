#pragma once

#include "../BuildConfig.h"

#include <string>
#include <vector>

#include "MagicEffects.h"
#include "Weather.h"

inline constexpr char NoTriggerEvent[] = "none";

enum class PlayerDirection
{
	Down, Up, Left, Right, Northwest, Northeast, Southwest, Southeast
};

// Spieler-Informationen mit Position und Geschwindigkeit
struct Player
{
	// X-Koordinate des Spielers im Weltkoordinatensystem
	float x = 0.0f;
	
	// Y-Koordinate des Spielers im Weltkoordinatensystem
	float y = 0.0f;
	
	// Bewegungsgeschwindigkeit (Pixel pro Sekunde)
	float speed = 220.0f;

	int size = 64;

	// Seitenlänge der quadratischen Kollisionsfläche, unabhängig von der Sprite-Größe.
	static constexpr float hitboxSize = 24.0f;
	static constexpr float hitboxOffsetY = 20.0f; // Positiv = nach unten schieben

	static constexpr float HitboxCenterY(float playerY) { return playerY + hitboxOffsetY; }

	PlayerDirection direction = PlayerDirection::Down;
	bool moving = false;
};

// Tastatur/Zustand der Eingabe für die Steuerung
struct InputState
{
	// Oben-Taste (W/↑) ist gedrückt
	bool up = false;
	
	// Unten-Taste (S/↓) ist gedrückt
	bool down = false;
	
	// Links-Taste (A/←) ist gedrückt
	bool left = false;
	
	// Rechts-Taste (D/→) ist gedrückt
	bool right = false;
};

// Anzeigebereich der Kamera/Sichtfeld
struct Viewport
{
	// Breite des sichtbaren Bereichs in Pixeln
	int width = 0;
	
	// Höhe des sichtbaren Bereichs in Pixeln
	int height = 0;
};

// Typen von Objekten in der Karte
enum class MapObjectType
{
	Untergrund,   // Hintergrundboden (nicht-solid)
	Scenery,      // Dekorative Elemente (Bäume, Felsen, etc.)
	Wall,         // Kollidierende Wände/Barrieren
	Npc           // Nicht-spielerische Charaktere
};

// Einzelne Objekte in der Karte
struct MapObject
{
	// Eindeutiger Identifier für das Objekt
	std::string id;
	
	// Typ des Objekts (sieht aus wie Dekoration, ist aber vielleicht solid)
	MapObjectType type = MapObjectType::Scenery;
	
	// X-Koordinate im Weltkoordinatensystem
	float x = 0.0f;
	
	// Y-Koordinate im Weltkoordinatensystem
	float y = 0.0f;
	
	// Pfad zum Sprite-Datei (z.B. "assets/sprites/tree.png")
	std::string spritePath;
	
	// Bestimmt die Kollision unabhängig vom Objekttyp.
	bool solid = false;

	// Optionaler Ausschnitt (Tile) aus einem Spritesheet in Texturpixeln
	bool hasTile = false;
	int tileX = 0;
	int tileY = 0;
	int tileWidth = 0;
	int tileHeight = 0;

	std::string npcName;
	PlayerDirection npcDirection = PlayerDirection::Down;
	double npcAnimationTime = 0.0;
};

// Auslöse-Zone für Ereignisse (Trigger)
struct TriggerZone
{
	// Eindeutiger Identifier für die Zone
	std::string id;
	
	// X-Koordinate der oberen linken Ecke
	float x = 0.0f;
	
	// Y-Koordinate der oberen linken Ecke
	float y = 0.0f;
	
	// Breite der Zone in Pixeln
	float width = 0.0f;
	
	// Höhe der Zone in Pixeln
	float height = 0.0f;
	
	// Name des Ereignisses, das ausgelöst wird (z.B. "collect_coin", "open_door")
	std::string eventName;
	
	// Ob die Zone sichtbar leuchtet (Glow-Shader, auch im Release-Build)
	bool glow = false;

	// Ob sich der Spieler aktuell innerhalb dieser Zone befindet
	bool isPlayerInside = false;

	float spawnX = 0.0f;
	float spawnY = 0.0f;
};

// Daten für eine einzelne Karte/Level
struct MapData
{
	// Eindeutiger Identifier für die Karte (z.B. "level_1", "world_map")
	std::string id;
	
	// Standard-Pfad zum Hintergrund-Boden-Sprite
	std::string defaultGroundPath;

	std::string defaultWeather = "none";

	// Liste aller Objekte in der Karte (Wände, Dekoration, NPCs)
	std::vector<MapObject> objects;
	
	// Liste aller Trigger-Zonen mit definierten Ereignissen
	std::vector<TriggerZone> triggers;
};

// Phasen eines Karten-Übergangs
enum class MapTransitionPhase
{
	None,          // Kein Übergang läuft
	FadeOut,       // Fading aus der aktuellen Karte
	FadeIn         // Fade in die neue Karte
};

// Zustand für einen Karten-Wechsel
struct MapTransitionState
{
	// Aktueller Übergangs-Phasenstatus
	MapTransitionPhase phase = MapTransitionPhase::None;

	// Verstrichene Zeit in der aktuellen Phase, in Sekunden.
	float timer = 0.0f;

	// Dauer je Fade-Phase, nicht die Gesamtdauer des Kartenwechsels.
	float duration = 0.28f;

	// Pfad zur Ziel-Karte (z.B. "maps/level_2.map")
	std::string targetMapPath;
	
	// Zielposition des Spielers auf der neuen Karte
	float targetPlayerX = 0.0f;
	
	// Zielposition des Spielers auf der neuen Karte
	float targetPlayerY = 0.0f;
};

struct CameraShakeState
{
	float elapsed = 0.0f;
	float duration = 0.0f;
	float intensity = 0.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
};

// Gesamtzustand des Spiels (Single-Page/Level-basiert)
struct GameState
{
	// Aktueller Spieler-Zustand
	Player player;
	
	// Aktueller Eingabe-Zustand (Tasten/Haltungen)
	InputState input;

	// Aktive Karte, die gerade angezeigt wird
	MapData activeMap;
	std::string activeMapPath;

	// Zuletzt ausgelöstes Ereignis (für Debugging/Logging)
	std::string lastTriggeredEvent;
	
	// Aktueller Karten-Übergangszustand
	MapTransitionState mapTransition;
	CameraShakeState cameraShake;

	MagicEffects magicEffects;
	WeatherState weather;
};
