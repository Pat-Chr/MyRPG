#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <utility>

#include "AnimationLoader.h"
#include "GameTypes.h"
#include "MagicEffectRegistry.h"
#include "MapLoader.h"

// Checks whether the player's collision box overlaps a trigger area.
inline bool IsPlayerOverlappingTrigger(const TriggerZone& trigger, float playerX, float playerY)
{
    constexpr float playerHalfSize = Player::hitboxSize * 0.5f;
    const float hitboxCenterY = Player::HitboxCenterY(playerY);
    return trigger.width > 0.0f && trigger.height > 0.0f &&
        playerX + playerHalfSize > trigger.x && playerX - playerHalfSize < trigger.x + trigger.width &&
        hitboxCenterY + playerHalfSize > trigger.y && hitboxCenterY - playerHalfSize < trigger.y + trigger.height;
}

// Checks whether a map-transition animation is running.
// While a map transition is active, player actions and trigger evaluation are disabled.
inline bool IsMapTransitionActive(const GameState& game)
{
    return game.mapTransition.phase != MapTransitionPhase::None;
}

// Derives a target map filename from a Load event.
inline bool TryResolveMapTransitionTarget(const std::string& eventName, std::string& outMapPath)
{
    constexpr const char* prefix = "Load_";
    if (!eventName.starts_with(prefix))
    {
        return false;
    }

    const std::string mapName = eventName.substr(5);
    if (mapName.empty() || mapName.find_first_of("\\/:*?\"<>|") != std::string::npos)
    {
        std::cerr << "Invalid map transition event: " << eventName << '\n';
        return false;
    }

    outMapPath = "Assets/Maps/" + mapName + ".map";
    return true;
}

// Resolves an effect from its shader name, for example "magic_fire".
inline bool TryResolveMagicEffect(const std::string& eventName, MagicEffectType& outType)
{
    constexpr const char* prefix = "magic_";
    if (eventName.starts_with(prefix))
    {
        const std::string id = eventName.substr(6);
        if (FindMagicEffectDefinition(id) != nullptr)
        {
            outType = id;
            return true;
        }
    }

    constexpr const char* legacyPrefix = "Effect_";
    if (eventName == "Effect_Magic" && FindMagicEffectDefinition("sparkle") != nullptr)
    {
        outType = "sparkle";
        return true;
    }
    if (eventName.starts_with(legacyPrefix))
    {
        std::string id = eventName.substr(7);
        std::transform(id.begin(), id.end(), id.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
        if (FindMagicEffectDefinition(id) != nullptr)
        {
            outType = std::move(id);
            return true;
        }
    }

    return false;
}

// Starts fading out the scene. The map and player position change only at the black frame.
// An active transition is not replaced.
inline void BeginMapTransition(GameState& game, 
                               std::string targetMapPath, 
                               float targetPlayerX, 
                               float targetPlayerY)
{
    if (IsMapTransitionActive(game))
    {
        return;
    }

    #ifdef _DEBUG
    std::cout << "[Map] Transition requested to '" << targetMapPath << "' at spawn ("
        << targetPlayerX << ", " << targetPlayerY << ").\n";
#endif
    game.player.attacking = false;
    game.mapTransition.phase = MapTransitionPhase::FadeOut;
    game.mapTransition.timer = 0.0f;
    game.mapTransition.targetMapPath = std::move(targetMapPath);
    game.mapTransition.targetPlayerX = targetPlayerX;
    game.mapTransition.targetPlayerY = targetPlayerY;
}

inline void StartCameraShake(CameraShakeState& shake, float intensity, float duration)
{
    shake = CameraShakeState{};
    shake.intensity = intensity;
    shake.duration = duration;
}

inline void UpdateCameraShake(CameraShakeState& shake, float deltaTime)
{
    if (shake.intensity <= 0.0f || shake.duration <= 0.0f)
    {
        return;
    }

    shake.elapsed += deltaTime;
    if (shake.elapsed >= shake.duration)
    {
        shake = CameraShakeState{};
        return;
    }

    const float progress = shake.elapsed / shake.duration;
    const float envelope = 1.0f - progress;
    shake.offsetX = std::sin(shake.elapsed * 75.0f) * shake.intensity * envelope;
    shake.offsetY = std::sin(shake.elapsed * 91.0f) * shake.intensity * 0.75f * envelope;
}

// Loads the target map between fade phases. If loading fails, the old map fades back in.
inline void UpdateMapTransition(GameState& game, float deltaTime)
{
    if (!IsMapTransitionActive(game))
    {
        return;
    }

    game.mapTransition.timer += deltaTime;
    if (game.mapTransition.timer < game.mapTransition.duration)
    {
        return;
    }

    if (game.mapTransition.phase == MapTransitionPhase::FadeOut)
    {
        MapData nextMap;
        if (TryLoadMapFromFile(game.mapTransition.targetMapPath, nextMap))
        {
            game.activeMap = std::move(nextMap);
            game.activeMapPath = game.mapTransition.targetMapPath;
            game.player.x = game.mapTransition.targetPlayerX;
            game.player.y = game.mapTransition.targetPlayerY;
            game.weather = WeatherState{ game.activeMap.defaultWeather };
            ClearMagicEffects(game.magicEffects);
            game.damageNumbers.clear();
            game.cameraShake = CameraShakeState{};
            game.lastTriggeredEvent = "MapLoaded: " + game.activeMap.id;
        }
        else
        {
            std::cerr << "Failed to load map: " << game.mapTransition.targetMapPath << '\n';
            game.lastTriggeredEvent = "MapLoadFailed: " + game.mapTransition.targetMapPath;
        }

        game.mapTransition.phase = MapTransitionPhase::FadeIn;
        game.mapTransition.timer = 0.0f;
        return;
    }

    game.mapTransition.phase = MapTransitionPhase::None;
    game.mapTransition.timer = 0.0f;
    game.mapTransition.targetMapPath.clear();
}

inline void BeginPlayerMonologue(GameState& game)
{
    if (game.monologueOriginalDirection) return;
    game.monologueOriginalDirection = game.player.direction;
    game.player.direction = PlayerDirection::Down;
    game.player.moving = false;
    game.player.attacking = false;
    game.input = InputState{};
}

inline void EndPlayerMonologue(GameState& game)
{
    if (!game.monologueOriginalDirection) return;
    game.player.direction = *game.monologueOriginalDirection;
    game.monologueOriginalDirection.reset();
    game.player.moving = false;
    game.input = InputState{};
}

// Fires events on entry, not while the player remains inside the zone.
// Trigger checks are skipped during an active map transition.
inline void UpdateTriggers(GameState& game)
{
    if (IsMapTransitionActive(game) || !game.pendingMonologue.empty() || game.monologueOriginalDirection)
    {
        return;
    }

    constexpr float shakeIntensity = 8.0f;
    constexpr float shakeDuration = 0.32f;

    for (TriggerZone& trigger : game.activeMap.triggers)
    {
        const bool isInside = IsPlayerOverlappingTrigger(trigger, game.player.x, game.player.y);

        if (isInside && !trigger.isPlayerInside && trigger.eventName != NoTriggerEvent)
        {
#ifdef _DEBUG
            std::cout << "[Trigger] Map '" << game.activeMap.id << "', trigger '" << trigger.id
                << "' activated event '" << trigger.eventName << "' at ("
                << game.player.x << ", " << game.player.y << ").\n";
#endif
            game.lastTriggeredEvent = trigger.eventName;

            if (trigger.eventName == DialogueTriggerEvent)
            {
                game.pendingMonologue = trigger.id;
                trigger.isPlayerInside = true;
                return;
            }

            if (trigger.eventName.starts_with("weather_"))
            {
                std::string weather;
                if (TryParseWeather(trigger.eventName.substr(8), weather))
                {
                    SetWeather(game.weather, weather);
                }
                else
                {
                    std::cerr << "Invalid weather event: " << trigger.eventName << '\n';
                }
            }

            if (trigger.eventName == "screen_shake")
            {
                StartCameraShake(game.cameraShake, shakeIntensity, shakeDuration);
            }

            MagicEffectType magicEffect;
            if (TryResolveMagicEffect(trigger.eventName, magicEffect))
            {
#ifdef _DEBUG
                std::cout << "[Magic] Starting effect '" << magicEffect << "' at ("
                    << game.player.x << ", " << game.player.y << ").\n";
#endif
                StartMagicEffect(game.magicEffects, magicEffect, game.player.x, game.player.y);
                const MagicEffectDefinition* definition = FindMagicEffectDefinition(magicEffect);
                if (definition != nullptr && definition->screenShake)
                {
                    StartCameraShake(game.cameraShake, shakeIntensity, shakeDuration);
                }
            }

            std::string targetMapPath;
            if (TryResolveMapTransitionTarget(trigger.eventName, targetMapPath))
            {
                BeginMapTransition(game, std::move(targetMapPath), trigger.spawnX, trigger.spawnY);
                trigger.isPlayerInside = true;
                return;
            }
        }

        trigger.isPlayerInside = isInside;
    }
}

// Initializes the game engine with a simple fallback map.
// Used when no map can be loaded or as the initial state.
// Creates a basic ground field, a test rock, and a trigger area.
inline void InitializeFallbackMap(GameState& game)
{
    game.activeMap.id = "fallback_map";
    game.activeMap.defaultGroundPath = "Assets/Sprites/Environment/basic_grass.png";
    game.activeMap.defaultWeather = "none";
    game.activeMap.objects.clear();
    game.activeMap.triggers.clear();

    game.activeMap.objects.push_back(MapObject{
        "fallback_rock",
        MapObjectType::Scenery,
        64.0f,
        64.0f,
        "Assets/Sprites/Environment/basic_rock.png",
        true
    });

    game.activeMap.triggers.push_back(TriggerZone{
        "fallback_trigger",
        -64.0f,
        -64.0f,
        128.0f,
        128.0f,
        "FallbackTrigger",
        false
    });
}

// Initializes the game state with the first available map.
// Tries to load the test map; if that fails, uses the fallback map.
inline void InitializeGameState(GameState& game)
{
    game.player.x = 0.0f;
    game.player.y = 0.0f;
    game.player.direction = PlayerDirection::Down;
    game.player.moving = false;
    game.player.attacking = false;
    game.player.attackElapsed = 0.0f;
    game.lastTriggeredEvent.clear();
    game.pendingMonologue.clear();
    game.monologueOriginalDirection.reset();
    ClearMagicEffects(game.magicEffects);
    game.damageNumbers.clear();
    game.cameraShake = CameraShakeState{};

    constexpr const char* startMapPath = "Assets/Maps/0001_TestMap.map";
    if (!TryLoadMapFromFile(startMapPath, game.activeMap))
    {
        InitializeFallbackMap(game);
        game.activeMapPath.clear();
    }
    else
    {
        game.activeMapPath = startMapPath;
    }
    game.weather = WeatherState{ game.activeMap.defaultWeather };
}

// Sets the player's movement state from keyboard input.
// Sets the direction flags (up, down, left, right) used by movement calculation.
inline void SetMovementState(InputState& input, bool up, bool down, bool left, bool right)
{
    input.up = up;
    input.down = down;
    input.left = left;
    input.right = right;
}

inline bool IsCharacterOverlappingObject(float x, float y, const MapObject& object)
{
    constexpr float characterHalfSize = Player::hitboxSize * 0.5f;
    const float objectHalfSize = object.type == MapObjectType::Npc ? characterHalfSize : 32.0f;
    const float objectCenterY = object.type == MapObjectType::Npc ? Player::HitboxCenterY(object.y) : object.y;
    return std::abs(x - object.x) < characterHalfSize + objectHalfSize &&
        std::abs(Player::HitboxCenterY(y) - objectCenterY) < characterHalfSize + objectHalfSize;
}

inline bool IsObjectBlockingPlayer(const MapObject& object, float playerX, float playerY)
{
    return object.solid && (object.type != MapObjectType::Npc || IsNpcAlive(object)) &&
        IsCharacterOverlappingObject(playerX, playerY, object);
}

// Checks overlapping collision boxes; only solid objects block the player.
inline bool IsSolidObjectBlockingPosition(const GameState& game, float playerX, float playerY)
{
    for (const MapObject& object : game.activeMap.objects)
        if (IsObjectBlockingPlayer(object, playerX, playerY)) return true;
    return false;
}

inline bool IsNpcPositionBlocked(const GameState& game, const MapObject& npc, float x, float y);

inline bool TryResolvePlayerPositionCollisions(GameState& restored, const std::string& mapPath,
    float x, float y, std::string& error)
{
    restored.player.x = x;
    restored.player.y = y;
    std::vector<std::size_t> blockingNpcs;
    for (std::size_t i = 0; i < restored.activeMap.objects.size(); ++i)
    {
        const MapObject& object = restored.activeMap.objects[i];
        if (!IsObjectBlockingPlayer(object, x, y)) continue;
        if (object.type != MapObjectType::Npc)
        {
            error = "Die gespeicherte Spielerposition liegt in einem Hindernis.";
            return false;
        }
        blockingNpcs.push_back(i);
    }

    // Assign all affected NPCs together so none steals another NPC's only available position.
    for (std::size_t index : blockingNpcs) restored.activeMap.objects[index].solid = false;
    using CellKey = std::pair<int, int>;
    std::vector<std::vector<CellKey>> candidates(blockingNpcs.size());
    for (std::size_t i = 0; i < blockingNpcs.size(); ++i)
    {
        const MapObject& npc = restored.activeMap.objects[blockingNpcs[i]];
        if (npc.walkingCells.empty())
        {
            error = "Der NPC '" + npc.id + "' blockiert die gespeicherte Spielerposition. In der geladenen Karte " +
                mapPath + " sind fuer diesen NPC keine Walking-Area-Felder gespeichert.";
            return false;
        }
        for (const WalkingCell& cell : npc.walkingCells)
        {
            if (std::floor(npc.x / 64.0f) == cell.x && std::floor(npc.y / 64.0f) == cell.y) continue;
            const float targetX = cell.x * 64.0f + 32.0f, targetY = cell.y * 64.0f + 32.0f;
            if (!IsNpcPositionBlocked(restored, npc, targetX, targetY)) candidates[i].emplace_back(cell.x, cell.y);
        }
        if (candidates[i].empty())
        {
            error = "Der NPC '" + npc.id + "' blockiert die gespeicherte Spielerposition. Seine " +
                std::to_string(npc.walkingCells.size()) + " Walking-Area-Felder in " + mapPath +
                " bieten kein freies anderes Feld; das aktuelle Feld und von Spieler oder soliden Objekten blockierte Felder scheiden aus.";
            return false;
        }
        std::sort(candidates[i].begin(), candidates[i].end(), [&](CellKey a, CellKey b) {
            const double ax = a.first * 64.0 + 32.0 - npc.x, ay = a.second * 64.0 + 32.0 - npc.y;
            const double bx = b.first * 64.0 + 32.0 - npc.x, by = b.second * 64.0 + 32.0 - npc.y;
            return ax * ax + ay * ay < bx * bx + by * by;
        });
    }
    std::map<CellKey, std::size_t> assignments;
    auto assign = [&](auto&& self, std::size_t npc, std::set<CellKey>& visited) -> bool {
        for (const CellKey& cell : candidates[npc])
        {
            if (!visited.insert(cell).second) continue;
            const auto occupied = assignments.find(cell);
            if (occupied == assignments.end() || self(self, occupied->second, visited))
            {
                assignments[cell] = npc;
                return true;
            }
        }
        return false;
    };
    for (std::size_t i = 0; i < blockingNpcs.size(); ++i)
    {
        std::set<CellKey> visited;
        if (!assign(assign, i, visited))
        {
            error = "Die gespeicherte Spielerposition wird von mehreren NPCs blockiert. Fuer NPC '" +
                restored.activeMap.objects[blockingNpcs[i]].id + "' und die anderen NPCs stehen in " + mapPath +
                " nicht genug unterschiedliche freie Walking-Area-Felder zur Verfuegung.";
            return false;
        }
    }
    for (const auto& [cell, index] : assignments)
    {
        MapObject& npc = restored.activeMap.objects[blockingNpcs[index]];
        npc.x = cell.first * 64.0f + 32.0f;
        npc.y = cell.second * 64.0f + 32.0f;
        npc.solid = true;
    }

    for (TriggerZone& trigger : restored.activeMap.triggers)
        trigger.isPlayerInside = IsPlayerOverlappingTrigger(trigger, x, y);
    return true;
}

inline bool ValidateMapPlayerCollisionRecovery(const MapData& map, const std::string& mapPath, std::string& error)
{
    for (std::size_t i = 0; i < map.objects.size(); ++i)
    {
        const MapObject& object = map.objects[i];
        if (object.type != MapObjectType::Npc || !object.solid) continue;
        std::vector<std::pair<float, float>> positions{ { object.x, object.y } };
        if (object.walkingAreaEnabled)
            for (const WalkingCell& cell : object.walkingCells)
                positions.emplace_back(cell.x * 64.0f + 32.0f, cell.y * 64.0f + 32.0f);
        for (const auto& [x, y] : positions)
        {
            GameState candidate{};
            candidate.activeMap = map;
            candidate.activeMap.objects[i].x = x;
            candidate.activeMap.objects[i].y = y;
            if (!TryResolvePlayerPositionCollisions(candidate, mapPath, x, y, error))
            {
                error = "Kollisionspruefung fuer NPC '" + object.id + "' auf Karte '" + mapPath + "' bei (" +
                    std::to_string(x) + ", " + std::to_string(y) + ") fehlgeschlagen: " + error;
                return false;
            }
        }
    }
    return true;
}

inline bool TryRestorePlayerPosition(GameState& game, const std::string& mapPath, float x, float y, std::string& error)
{
    GameState restored{};
    if (!TryLoadMapFromFile(mapPath, restored.activeMap))
    {
        error = "Die gespeicherte Karte konnte nicht geladen werden: " + mapPath;
        return false;
    }
    if (!TryResolvePlayerPositionCollisions(restored, mapPath, x, y, error)) return false;
    restored.activeMapPath = mapPath;
    restored.weather = WeatherState{ restored.activeMap.defaultWeather };
    game = std::move(restored);
    return true;
}

// Interacts with the nearest NPC in view and returns it.
inline MapObject* InteractWithNpc(GameState& game)
{
    if (IsMapTransitionActive(game)) return nullptr;

    float facingX = 0.0f, facingY = 0.0f;
    switch (game.player.direction)
    {
    case PlayerDirection::Down: facingY = 1.0f; break;
    case PlayerDirection::Up: facingY = -1.0f; break;
    case PlayerDirection::Left: facingX = -1.0f; break;
    case PlayerDirection::Right: facingX = 1.0f; break;
    case PlayerDirection::Northwest: facingX = -0.70710678f; facingY = -0.70710678f; break;
    case PlayerDirection::Northeast: facingX = 0.70710678f; facingY = -0.70710678f; break;
    case PlayerDirection::Southwest: facingX = -0.70710678f; facingY = 0.70710678f; break;
    case PlayerDirection::Southeast: facingX = 0.70710678f; facingY = 0.70710678f; break;
    }

    constexpr float maxDistance = 100.0f;
    constexpr float maxSideDistance = 48.0f;
    float closestDistanceSquared = maxDistance * maxDistance;
    MapObject* closest = nullptr;
    for (MapObject& object : game.activeMap.objects)
    {
        if (object.type != MapObjectType::Npc || !IsNpcAlive(object) || IsStationaryNpc(object)) continue;
        const float dx = object.x - game.player.x, dy = object.y - game.player.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (dx * facingX + dy * facingY <= 0.0f ||
            std::abs(dx * facingY - dy * facingX) > maxSideDistance || distanceSquared > closestDistanceSquared) continue;
        closest = &object;
        closestDistanceSquared = distanceSquared;
    }
    if (!closest) return nullptr;

    closest->npcMoving = false;
    closest->npcWalkWait = 2.0f;
    const float dx = game.player.x - closest->x, dy = game.player.y - closest->y;
    const PlayerDirection direction = std::abs(dx) > std::abs(dy)
        ? (dx < 0.0f ? PlayerDirection::Left : PlayerDirection::Right)
        : (dy < 0.0f ? PlayerDirection::Up : PlayerDirection::Down);
    if (closest->npcDirection != direction)
    {
        closest->npcDirection = direction;
        closest->npcAnimationTime = 0.0;
    }
    return closest;
}

inline bool IsNpcPositionBlocked(const GameState& game, const MapObject& npc, float x, float y)
{
    for (const MapObject& other : game.activeMap.objects)
    {
        if (&other != &npc && other.solid && (other.type != MapObjectType::Npc || IsNpcAlive(other)) &&
            IsCharacterOverlappingObject(x, y, other))
            return true;
    }
    return std::abs(x - game.player.x) < Player::hitboxSize &&
        std::abs(Player::HitboxCenterY(y) - Player::HitboxCenterY(game.player.y)) < Player::hitboxSize;
}

inline void UpdateNpcWalking(GameState& game, MapObject& npc, float deltaTime)
{
    if (!IsNpcAlive(npc) || IsStationaryNpc(npc) || !npc.walkingAreaEnabled || npc.walkingCells.empty())
    {
        if (npc.npcMoving) npc.npcAnimationTime = 0.0;
        npc.npcMoving = false;
        return;
    }
    if (deltaTime <= 0.0f) return;
    static std::mt19937 random(std::random_device{}());
    if (!npc.npcMoving)
    {
        npc.npcWalkWait -= deltaTime;
        if (npc.npcWalkWait > 0.0f) return;
        npc.npcWalkWait = std::uniform_real_distribution<float>(2.0f, 4.0f)(random);
        if (!std::isfinite(npc.x) || !std::isfinite(npc.y) || std::abs(npc.x) > 64000000.0f || std::abs(npc.y) > 64000000.0f) return;
        const WalkingCell current{ static_cast<int>(std::floor(npc.x / 64.0f)), static_cast<int>(std::floor(npc.y / 64.0f)) };
        const auto contains = [&](WalkingCell cell) { return std::find(npc.walkingCells.begin(), npc.walkingCells.end(), cell) != npc.walkingCells.end(); };
        if (!contains(current)) return;
        const float centerX = current.x * 64.0f + 32.0f, centerY = current.y * 64.0f + 32.0f;
        if (std::abs(npc.x - centerX) > 0.01f || std::abs(npc.y - centerY) > 0.01f)
        {
            npc.npcTargetX = centerX;
            npc.npcTargetY = centerY;
        }
        else
        {
            const WalkingCell neighbors[] = { { current.x - 1, current.y }, { current.x + 1, current.y },
                { current.x, current.y - 1 }, { current.x, current.y + 1 } };
            std::vector<WalkingCell> candidates;
            bool hasAdjacentCell = false;
            for (WalkingCell cell : neighbors)
            {
                if (!contains(cell)) continue;
                hasAdjacentCell = true;
                if (!IsNpcPositionBlocked(game, npc, cell.x * 64.0f + 32.0f, cell.y * 64.0f + 32.0f))
                    candidates.push_back(cell);
            }
            if (candidates.empty())
            {
#ifdef _DEBUG
                if (hasAdjacentCell)
                    std::cout << "[NPC] Movement blocked: no free adjacent Walking-Area cell for NPC '" << npc.id
                        << "' on map '" << game.activeMap.id << "'.\n";
#endif
                return;
            }
            const WalkingCell target = candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(random)];
            npc.npcTargetX = target.x * 64.0f + 32.0f;
            npc.npcTargetY = target.y * 64.0f + 32.0f;
        }
        npc.npcMoving = true;
        npc.npcAnimationTime = 0.0;
    }

    const float dx = npc.npcTargetX - npc.x, dy = npc.npcTargetY - npc.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= 0.001f) { npc.npcMoving = false; npc.npcAnimationTime = 0.0; return; }
    const PlayerDirection direction = std::abs(dx) > std::abs(dy)
        ? (dx < 0.0f ? PlayerDirection::Left : PlayerDirection::Right)
        : (dy < 0.0f ? PlayerDirection::Up : PlayerDirection::Down);
    if (npc.npcDirection != direction) { npc.npcDirection = direction; npc.npcAnimationTime = 0.0; }
    const float travel = (std::min)(distance, 90.0f * deltaTime);
    const int steps = (std::max)(1, static_cast<int>(std::ceil(travel / 8.0f)));
    const float stepX = dx / distance * travel / steps, stepY = dy / distance * travel / steps;
    for (int i = 0; i < steps; ++i)
    {
        if (IsNpcPositionBlocked(game, npc, npc.x + stepX, npc.y + stepY))
        {
#ifdef _DEBUG
            std::cout << "[NPC] Movement blocked for NPC '" << npc.id << "' on map '" << game.activeMap.id
                << "' at (" << npc.x + stepX << ", " << npc.y + stepY << ").\n";
#endif
            npc.npcMoving = false;
            npc.npcAnimationTime = 0.0;
            return;
        }
        npc.x += stepX;
        npc.y += stepY;
    }
    if (travel >= distance)
    {
        npc.x = npc.npcTargetX; npc.y = npc.npcTargetY;
        npc.npcMoving = false;
        npc.npcAnimationTime = 0.0;
    }
}

inline void StartPlayerAttack(GameState& game)
{
    if (IsMapTransitionActive(game) || game.player.attacking ||
        !game.pendingMonologue.empty() || game.monologueOriginalDirection) return;
    const PlayerDirection direction = CardinalPlayerDirection(game.player.direction);
    const auto& animation = GetPlayerAnimations()[12 + static_cast<std::size_t>(direction)];
    if (!animation) return;
    game.player.direction = direction;
    game.player.moving = false;
    game.player.currentAttackDamage = CalculatePlayerAttackDamage(game.player);
    game.player.stamina = 0.0f;
    game.player.sprintExhausted = true;
    game.player.attacking = true;
    game.player.attackElapsed = 0.0f;
    game.player.attackDamageApplied = false;
}

inline void UpdatePlayerStamina(Player& player, float deltaTime, bool sprinting)
{
    if (sprinting)
    {
        player.stamina = (std::max)(0.0f, player.stamina - Player::staminaDrainPerSecond * deltaTime);
        if (player.stamina == 0.0f)
        {
            player.sprintExhausted = true;
            player.staminaRecoveryRequiresStop = true;
        }
    }
    else if (!player.staminaRecoveryRequiresStop || (!player.moving && deltaTime > 0.0f))
    {
        player.staminaRecoveryRequiresStop = false;
        player.stamina = (std::min)(Player::maxStamina, player.stamina + Player::staminaRecoveryPerSecond * deltaTime);
    }
}

// Effects continue during map transitions; movement and trigger checks are paused.
inline void UpdateGame(GameState& game, float deltaTime)
{
    if (!game.pendingMonologue.empty() || game.monologueOriginalDirection) return;
    if (!game.input.sprint) game.player.sprintExhausted = false;
    UpdateMagicEffects(game.magicEffects, deltaTime);
    UpdateDamageNumbers(game.damageNumbers, deltaTime);
    UpdateCameraShake(game.cameraShake, deltaTime);
    UpdateWeather(game.weather, deltaTime);
    for (MapObject& object : game.activeMap.objects)
    {
        if (object.type == MapObjectType::Npc)
        {
            object.npcAnimationTime += deltaTime;
            object.hitRemaining = (std::max)(0.0f, object.hitRemaining - deltaTime);
        }
    }

    if (IsMapTransitionActive(game))
    {
        game.player.moving = false;
        UpdatePlayerStamina(game.player, deltaTime, false);
        for (MapObject& object : game.activeMap.objects)
            if (object.type == MapObjectType::Npc && object.npcMoving)
            {
                object.npcMoving = false;
                object.npcWalkWait = 2.0f;
                object.npcAnimationTime = 0.0;
            }
        UpdateMapTransition(game, deltaTime);
        return;
    }

    for (MapObject& object : game.activeMap.objects)
        if (object.type == MapObjectType::Npc) UpdateNpcWalking(game, object, deltaTime);

    if (game.player.attacking)
    {
        if (deltaTime > 0.0f) game.player.staminaRecoveryRequiresStop = false;
        const auto& animation = GetPlayerAnimations()[12 + static_cast<std::size_t>(game.player.direction)];
        game.player.attackElapsed += deltaTime * Player::attackAnimationSpeedMultiplier;
        const float duration = animation->frameDuration * static_cast<float>(animation->frames.size());
        if (game.player.attackElapsed >= duration * 0.5f) ApplyPlayerAttackDamage(game);
        if (game.player.attackElapsed >= duration)
            game.player.attacking = false;
        return;
    }

    float moveX = 0.0f;
    float moveY = 0.0f;

    if (game.input.left) moveX -= 1.0f;
    if (game.input.right) moveX += 1.0f;
    if (game.input.up) moveY -= 1.0f;
    if (game.input.down) moveY += 1.0f;

    const float length = std::sqrt((moveX * moveX) + (moveY * moveY));
    const bool wantsToSprint = game.input.sprint && !game.player.sprintExhausted &&
        game.player.stamina > 0.0f && length > 0.0f;
    const float movementSpeed = game.player.speed * (wantsToSprint ? Player::sprintSpeedMultiplier : 1.0f);
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    // Check axes separately so diagonal movement can slide along obstacles.
    const float previousX = game.player.x;
    const float previousY = game.player.y;
    const float nextX = game.player.x + (moveX * movementSpeed * deltaTime);
    if (!IsSolidObjectBlockingPosition(game, nextX, game.player.y))
    {
        game.player.x = nextX;
    }

    const float nextY = game.player.y + (moveY * movementSpeed * deltaTime);
    if (!IsSolidObjectBlockingPosition(game, game.player.x, nextY))
    {
        game.player.y = nextY;
    }

    const float actualX = game.player.x - previousX;
    const float actualY = game.player.y - previousY;
    game.player.moving = actualX != 0.0f || actualY != 0.0f;
    UpdatePlayerStamina(game.player, deltaTime, wantsToSprint && game.player.moving);
    const float facingX = game.player.moving ? actualX : moveX;
    const float facingY = game.player.moving ? actualY : moveY;
    if (facingY < 0.0f)
    {
        game.player.direction = facingX < 0.0f ? PlayerDirection::Northwest :
            facingX > 0.0f ? PlayerDirection::Northeast : PlayerDirection::Up;
    }
    else if (facingY > 0.0f)
    {
        game.player.direction = facingX < 0.0f ? PlayerDirection::Southwest :
            facingX > 0.0f ? PlayerDirection::Southeast : PlayerDirection::Down;
    }
    else if (facingX < 0.0f)
    {
        game.player.direction = PlayerDirection::Left;
    }
    else if (facingX > 0.0f)
    {
        game.player.direction = PlayerDirection::Right;
    }

    UpdateTriggers(game);
}
