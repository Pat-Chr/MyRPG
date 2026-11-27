#pragma once

#include "../BuildConfig.h"

#include <optional>
#include <string>
#include <vector>

#include "DamageNumbers.h"
#include "MagicEffects.h"
#include "Weather.h"

inline constexpr char NoTriggerEvent[] = "none";
inline constexpr char DialogueTriggerEvent[] = "dialogue";

enum class PlayerDirection
{
	Down, Up, Left, Right, Northwest, Northeast, Southwest, Southeast
};

inline constexpr PlayerDirection CardinalPlayerDirection(PlayerDirection direction)
{
	switch (direction)
	{
	case PlayerDirection::Northwest:
	case PlayerDirection::Southwest: return PlayerDirection::Left;
	case PlayerDirection::Northeast:
	case PlayerDirection::Southeast: return PlayerDirection::Right;
	default: return direction;
	}
}

// Player information with position and movement speed.
struct Player
{
	// Player X-coordinate in world coordinates.
	float x = 0.0f;

	// Player Y-coordinate in world coordinates.
	float y = 0.0f;

	// Movement speed in pixels per second.
	float speed = 220.0f;

	static constexpr float maxStamina = 100.0f;
	static constexpr float sprintSpeedMultiplier = 1.65f;
	static constexpr float staminaDrainPerSecond = 27.0f;
	static constexpr float staminaRecoveryPerSecond = 36.0f;
	static constexpr float attackAnimationSpeedMultiplier = 2.0f;
	float stamina = maxStamina;
	bool sprintExhausted = false;
	bool staminaRecoveryRequiresStop = false;

	int size = 64;

	// Side length of the square collision area, independent of the sprite size.
	static constexpr float hitboxSize = 24.0f;
	static constexpr float hitboxOffsetY = 20.0f; // Positive values shift it downward.

	static constexpr float HitboxCenterY(float playerY) { return playerY + hitboxOffsetY; }

	PlayerDirection direction = PlayerDirection::Down;
	bool moving = false;
	bool attacking = false;
	float attackElapsed = 0.0f;
	int attackDamage = 10;
	int currentAttackDamage = 0;
	bool attackDamageApplied = false;
};

// Keyboard state used for movement controls.
struct InputState
{
	// Up key (W/Up) is pressed.
	bool up = false;

	// Down key (S/Down) is pressed.
	bool down = false;

	// Left key (A/Left) is pressed.
	bool left = false;

	// Right key (D/Right) is pressed.
	bool right = false;

	// A Shift key is pressed during active gameplay.
	bool sprint = false;
};

// Camera viewport and visible area.
struct Viewport
{
	// Width of the visible area in pixels.
	int width = 0;

	// Height of the visible area in pixels.
	int height = 0;
};

// Types of objects on the map.
enum class MapObjectType
{
	Ground,       // Background ground (non-solid).
	Scenery,      // Decorative elements such as trees and rocks.
	Wall,         // Colliding walls and barriers.
	Npc           // Non-player characters.
};

struct WalkingCell
{
	int x = 0;
	int y = 0;
	bool operator==(const WalkingCell&) const = default;
};

// An individual object on the map.
struct MapObject
{
	// Unique identifier for the object.
	std::string id;

	// Object type; decoration may still be solid.
	MapObjectType type = MapObjectType::Scenery;

	// X-coordinate in world coordinates.
	float x = 0.0f;

	// Y-coordinate in world coordinates.
	float y = 0.0f;

	// Path to the sprite file, for example "assets/sprites/tree.png".
	std::string spritePath;

	// Controls collision independently of the object type.
	bool solid = false;

	// Optional tile rectangle from a spritesheet in texture pixels.
	bool hasTile = false;
	int tileX = 0;
	int tileY = 0;
	int tileWidth = 0;
	int tileHeight = 0;

	std::string npcName;
	PlayerDirection npcDirection = PlayerDirection::Down;
	double npcAnimationTime = 0.0;
	bool walkingAreaEnabled = false;
	std::vector<WalkingCell> walkingCells;
	bool npcMoving = false;
	float npcWalkWait = 2.0f;
	float npcTargetX = 0.0f, npcTargetY = 0.0f;
	int health = 100;
	bool infiniteHealth = false;
	bool passive = true;
	bool enemy = false;
	float hitRemaining = 0.0f;
	int lastDamage = 0;
};

// Event trigger zone.
struct TriggerZone
{
	// Unique identifier for the zone.
	std::string id;

	// X-coordinate of the top-left corner.
	float x = 0.0f;

	// Y-coordinate of the top-left corner.
	float y = 0.0f;

	// Zone width in pixels.
	float width = 0.0f;

	// Zone height in pixels.
	float height = 0.0f;

	// Name of the event to trigger, for example "collect_coin" or "open_door".
	std::string eventName;

	// Whether the zone glows through a shader, including in Release builds.
	bool glow = false;

	// Whether the player is currently inside this zone.
	bool isPlayerInside = false;

	float spawnX = 0.0f;
	float spawnY = 0.0f;
};

// Data for one map or level.
struct MapData
{
	// Unique identifier for the map, for example "level_1" or "world_map".
	std::string id;

	// Default path to the background ground sprite.
	std::string defaultGroundPath;

	std::string defaultWeather = "none";

	// List of map objects such as walls, scenery, and NPCs.
	std::vector<MapObject> objects;

	// List of trigger zones with defined events.
	std::vector<TriggerZone> triggers;
};

// Phases of a map transition.
enum class MapTransitionPhase
{
	None,          // No transition is running.
	FadeOut,       // Fade out from the current map.
	FadeIn         // Fade in to the new map.
};

// State for a map change.
struct MapTransitionState
{
	// Current transition phase.
	MapTransitionPhase phase = MapTransitionPhase::None;

	// Elapsed time in the current phase, in seconds.
	float timer = 0.0f;

	// Duration of each fade phase, not the total transition duration.
	float duration = 0.28f;

	// Path to the target map, for example "maps/level_2.map".
	std::string targetMapPath;

	// Target player X-coordinate on the new map.
	float targetPlayerX = 0.0f;

	// Target player Y-coordinate on the new map.
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

// Complete game state for the current level.
struct GameState
{
	// Current player state.
	Player player;

	// Current input state.
	InputState input;

	// Active map that is currently displayed.
	MapData activeMap;
	std::string activeMapPath;

	// Last triggered event, used for debugging and logging.
	std::string lastTriggeredEvent;
	std::string pendingMonologue;
	std::optional<PlayerDirection> monologueOriginalDirection;

	// Current map-transition state.
	MapTransitionState mapTransition;
	CameraShakeState cameraShake;

	MagicEffects magicEffects;
	WeatherState weather;
	std::vector<DamageNumber> damageNumbers;
};
