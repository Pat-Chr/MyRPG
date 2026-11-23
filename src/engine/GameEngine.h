#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <utility>

#include "GameTypes.h"
#include "MagicEffectRegistry.h"
#include "MapLoader.h"

// Prüft, ob ein gegebener Punkt innerhalb eines Trigger-Bereichs liegt.
// Ein Trigger ist ein rechteckiger Bereich mit definierten Abmessungen.
inline bool IsInsideTrigger(const TriggerZone& trigger, float x, float y)
{
    return x >= trigger.x && x <= (trigger.x + trigger.width) && y >= trigger.y && y <= (trigger.y + trigger.height);
}

// Prüft, ob gerade eine Map-Übergangsanimation läuft.
// Wenn eine Map-Transition aktiv ist, sind keine Spieleraktionen oder Trigger-Auslöser erlaubt.
inline bool IsMapTransitionActive(const GameState& game)
{
    return game.mapTransition.phase != MapTransitionPhase::None;
}

// Leitet den Dateinamen einer Zielkarte aus einem Load-Event ab.
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

// Löst einen Effekt über seinen Shader-Namen auf (z. B. "magic_fire").
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

// Startet das Ausblenden der Szene. Karte und Spielerposition wechseln erst bei Schwarzbild.
// Ein laufender Übergang wird nicht ersetzt.
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

// Lädt die Zielkarte zwischen den Fade-Phasen. Schlägt das Laden fehl, wird die alte Karte wieder eingeblendet.
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

// Löst Ereignisse beim Betreten aus, nicht solange der Spieler in der Zone bleibt.
// Bei bereits laufendem Kartenübergang wird die Triggerprüfung übersprungen.
inline void UpdateTriggers(GameState& game)
{
    if (IsMapTransitionActive(game))
    {
        return;
    }

    constexpr float shakeIntensity = 8.0f;
    constexpr float shakeDuration = 0.32f;

    for (TriggerZone& trigger : game.activeMap.triggers)
    {
        const bool isInside = IsInsideTrigger(trigger, game.player.x, game.player.y);

        if (isInside && !trigger.isPlayerInside && trigger.eventName != NoTriggerEvent)
        {
#ifdef _DEBUG
            std::cout << "[Trigger] Map '" << game.activeMap.id << "', trigger '" << trigger.id
                << "' activated event '" << trigger.eventName << "' at ("
                << game.player.x << ", " << game.player.y << ").\n";
#endif
            game.lastTriggeredEvent = trigger.eventName;

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
            }
        }

        trigger.isPlayerInside = isInside;
    }
}

// Initialisiert die Spiel-Engine mit einer einfachen Fallback-Map.
// Wird verwendet, wenn keine Map geladen werden kann oder als Startzustand.
// Erstellt ein einfaches Grundfeld und einen Test-Stein sowie einen Trigger-Bereich.
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

// Initialisiert den Spielzustand mit der ersten verfügbaren Map.
// Versucht, die Test-Map zu laden. Wenn das fehlschlägt, wird eine Fallback-Map verwendet.
inline void InitializeGameState(GameState& game)
{
    game.player.x = 0.0f;
    game.player.y = 0.0f;
    game.player.direction = PlayerDirection::Down;
    game.player.moving = false;
    game.lastTriggeredEvent.clear();
    ClearMagicEffects(game.magicEffects);
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

// Stellt den Bewegungszustand des Spielers basierend auf Tastatureingaben ein.
// Setzt die Richtungsfahnen (up, down, left, right) für die Bewegungsberechnung.
inline void SetMovementState(InputState& input, bool up, bool down, bool left, bool right)
{
    input.up = up;
    input.down = down;
    input.left = left;
    input.right = right;
}

inline bool IsObjectBlockingPlayer(const MapObject& object, float playerX, float playerY)
{
    constexpr float objectHalfSize = 32.0f;
    constexpr float playerHalfSize = Player::hitboxSize * 0.5f;
    const float hitboxCenterY = Player::HitboxCenterY(playerY);
    return object.solid && playerX + playerHalfSize > object.x - objectHalfSize &&
        playerX - playerHalfSize < object.x + objectHalfSize &&
        hitboxCenterY + playerHalfSize > object.y - objectHalfSize &&
        hitboxCenterY - playerHalfSize < object.y + objectHalfSize;
}

// Prüft überlappende Kollisionsboxen. Ausschlaggebend ist nur solid, nicht der Objekttyp.
inline bool IsSolidObjectBlockingPosition(const GameState& game, float playerX, float playerY)
{
    for (const MapObject& object : game.activeMap.objects)
        if (IsObjectBlockingPlayer(object, playerX, playerY)) return true;
    return false;
}

inline bool IsNpcPositionBlocked(const GameState& game, const MapObject& npc, float x, float y);

inline bool TryRestorePlayerPosition(GameState& game, const std::string& mapPath, float x, float y, std::string& error)
{
    GameState restored{};
    if (!TryLoadMapFromFile(mapPath, restored.activeMap))
    {
        error = "Die gespeicherte Karte konnte nicht geladen werden: " + mapPath;
        return false;
    }
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

    // Alle betroffenen NPCs werden gemeinsam zugewiesen, damit keiner einem anderen den einzigen Platz nimmt.
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

    restored.activeMapPath = mapPath;
    restored.weather = WeatherState{ restored.activeMap.defaultWeather };
    for (TriggerZone& trigger : restored.activeMap.triggers)
    {
        trigger.isPlayerInside = IsInsideTrigger(trigger, x, y);
    }
    game = std::move(restored);
    return true;
}

// Spricht den nächsten NPC im Blickfeld an und gibt ihn zurück.
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
        if (object.type != MapObjectType::Npc) continue;
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
        if (&other != &npc && other.solid && std::abs(x - other.x) < 64.0f && std::abs(y - other.y) < 64.0f)
            return true;
    }
    return std::abs(x - game.player.x) < 32.0f + Player::hitboxSize * 0.5f &&
        std::abs(y - Player::HitboxCenterY(game.player.y)) < 32.0f + Player::hitboxSize * 0.5f;
}

inline void UpdateNpcWalking(GameState& game, MapObject& npc, float deltaTime)
{
    if (!npc.walkingAreaEnabled || npc.walkingCells.empty())
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
            for (WalkingCell cell : neighbors)
                if (contains(cell) && !IsNpcPositionBlocked(game, npc, cell.x * 64.0f + 32.0f, cell.y * 64.0f + 32.0f))
                    candidates.push_back(cell);
            if (candidates.empty()) return;
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

// Effekte laufen während eines Kartenübergangs weiter; Bewegung und Triggerprüfung pausieren.
inline void UpdateGame(GameState& game, float deltaTime)
{
    UpdateMagicEffects(game.magicEffects, deltaTime);
    UpdateCameraShake(game.cameraShake, deltaTime);
    UpdateWeather(game.weather, deltaTime);
    for (MapObject& object : game.activeMap.objects)
    {
        if (object.type == MapObjectType::Npc)
            object.npcAnimationTime += deltaTime;
    }

    if (IsMapTransitionActive(game))
    {
        game.player.moving = false;
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

    float moveX = 0.0f;
    float moveY = 0.0f;

    if (game.input.left) moveX -= 1.0f;
    if (game.input.right) moveX += 1.0f;
    if (game.input.up) moveY -= 1.0f;
    if (game.input.down) moveY += 1.0f;

    const float length = std::sqrt((moveX * moveX) + (moveY * moveY));
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    // Achsen getrennt prüfen, damit diagonale Bewegung an Hindernissen entlanggleiten kann.
    const float previousX = game.player.x;
    const float previousY = game.player.y;
    const float nextX = game.player.x + (moveX * game.player.speed * deltaTime);
    if (!IsSolidObjectBlockingPosition(game, nextX, game.player.y))
    {
        game.player.x = nextX;
    }

    const float nextY = game.player.y + (moveY * game.player.speed * deltaTime);
    if (!IsSolidObjectBlockingPosition(game, game.player.x, nextY))
    {
        game.player.y = nextY;
    }

    const float actualX = game.player.x - previousX;
    const float actualY = game.player.y - previousY;
    game.player.moving = actualX != 0.0f || actualY != 0.0f;
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
