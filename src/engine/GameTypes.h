#pragma once

#include <string>
#include <vector>

#include "MagicEffects.h"

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
	
	// Kollisionsradius oder Sprite-Größe in Pixeln
	int size = 64;

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
	
	// Ob das Objekt Kollisionen verursacht (wahr für Wände)
	bool solid = false;
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
	
	// Ob sich der Spieler aktuell innerhalb dieser Zone befindet
	bool isPlayerInside = false;
};

// Daten für eine einzelne Karte/Level
struct MapData
{
	// Eindeutiger Identifier für die Karte (z.B. "level_1", "world_map")
	std::string id;
	
	// Standard-Pfad zum Hintergrund-Boden-Sprite
	std::string defaultGroundPath;
	
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
	
	// Verstrichene Zeit seit Beginn des Übergangs (für Animation)
	float timer = 0.0f;
	
	// Gesamtdauer des Übergangs in Sekunden
	float duration = 0.28f;
	
	// Pfad zur Ziel-Karte (z.B. "maps/level_2.map")
	std::string targetMapPath;
	
	// Zielposition des Spielers auf der neuen Karte
	float targetPlayerX = 0.0f;
	
	// Zielposition des Spielers auf der neuen Karte
	float targetPlayerY = 0.0f;
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
	
	// Zuletzt ausgelöstes Ereignis (für Debugging/Logging)
	std::string lastTriggeredEvent;
	
	// Aktueller Karten-Übergangszustand
	MapTransitionState mapTransition;

	MagicEffects magicEffects;
};
