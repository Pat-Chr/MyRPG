#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
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

// Prüft überlappende Kollisionsboxen. Ausschlaggebend ist nur solid, nicht der Objekttyp.
inline bool IsSolidObjectBlockingPosition(const GameState& game, float playerX, float playerY)
{
    constexpr float mapObjectSize = 64.0f;
    const float objectHalfSize = mapObjectSize * 0.5f;
    constexpr float playerHalfSize = Player::hitboxSize * 0.5f;
    const float hitboxCenterY = Player::HitboxCenterY(playerY);

    for (const MapObject& object : game.activeMap.objects)
    {
        if (!object.solid)
        {
            continue;
        }

        const float objectLeft = object.x - objectHalfSize;
        const float objectRight = object.x + objectHalfSize;
        const float objectTop = object.y - objectHalfSize;
        const float objectBottom = object.y + objectHalfSize;

        const float playerLeft = playerX - playerHalfSize;
        const float playerRight = playerX + playerHalfSize;
        const float playerTop = hitboxCenterY - playerHalfSize;
        const float playerBottom = hitboxCenterY + playerHalfSize;

        const bool overlapX = playerRight > objectLeft && playerLeft < objectRight;
        const bool overlapY = playerBottom > objectTop && playerTop < objectBottom;

        if (overlapX && overlapY)
        {
            return true;
        }
    }

    return false;
}

inline bool TryRestorePlayerPosition(GameState& game, const std::string& mapPath, float x, float y, std::string& error)
{
    GameState restored{};
    if (!TryLoadMapFromFile(mapPath, restored.activeMap))
    {
        error = "Die gespeicherte Karte konnte nicht geladen werden: " + mapPath;
        return false;
    }
    if (IsSolidObjectBlockingPosition(restored, x, y))
    {
        error = "Die gespeicherte Spielerposition liegt in einem Hindernis.";
        return false;
    }

    restored.activeMapPath = mapPath;
    restored.player.x = x;
    restored.player.y = y;
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
        UpdateMapTransition(game, deltaTime);
        return;
    }

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
