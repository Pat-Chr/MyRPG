#pragma once

#include <string>
#include <vector>

struct Player
{
	float x = 0.0f;
	float y = 0.0f;
	float speed = 220.0f;
	int size = 64;
};

struct InputState
{
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
};

struct Viewport
{
	int width = 0;
	int height = 0;
};

enum class MapObjectType
{
	Untergrund,
	Scenery,
	Wall,
	Npc
};

struct MapObject
{
	std::string id;
	MapObjectType type = MapObjectType::Scenery;
	float x = 0.0f;
	float y = 0.0f;
	std::string spritePath;
	bool solid = false;
};

struct TriggerZone
{
	std::string id;
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	std::string eventName;
	bool isPlayerInside = false;
};

struct MapData
{
	std::string id;
	std::string defaultGroundPath;
	std::vector<MapObject> objects;
	std::vector<TriggerZone> triggers;
};

enum class MapTransitionPhase
{
	None,
	FadeOut,
	FadeIn
};

struct MapTransitionState
{
	MapTransitionPhase phase = MapTransitionPhase::None;
	float timer = 0.0f;
	float duration = 0.28f;
	std::string targetMapPath;
	float targetPlayerX = 0.0f;
	float targetPlayerY = 0.0f;
};

struct GameState
{
	Player player;
	InputState input;
	MapData activeMap;
	std::string lastTriggeredEvent;
	MapTransitionState mapTransition;
};
