#pragma once

#include <algorithm>
#include <cmath>

#include "GameTypes.h"
#include "MapLoader.h"

inline bool IsInsideTrigger(const TriggerZone& trigger, float x, float y)
{
    return x >= trigger.x && x <= (trigger.x + trigger.width) && y >= trigger.y && y <= (trigger.y + trigger.height);
}

inline void UpdateTriggers(GameState& game)
{
    for (TriggerZone& trigger : game.activeMap.triggers)
    {
        const bool isInside = IsInsideTrigger(trigger, game.player.x, game.player.y);

        if (isInside && !trigger.isPlayerInside)
        {
            game.lastTriggeredEvent = trigger.eventName;
        }

        trigger.isPlayerInside = isInside;
    }
}

inline void InitializeFallbackMap(GameState& game)
{
    game.activeMap.id = "fallback_map";
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

inline void InitializeGameState(GameState& game)
{
    game.player.x = 0.0f;
    game.player.y = 0.0f;
    game.lastTriggeredEvent.clear();

    if (!TryLoadMapFromFile("Assets/Maps/0001_TestMap.map", game.activeMap))
    {
        InitializeFallbackMap(game);
    }
}

inline void SetMovementState(InputState& input, bool up, bool down, bool left, bool right)
{
    input.up = up;
    input.down = down;
    input.left = left;
    input.right = right;
}

inline bool IsSolidObjectBlockingPosition(const GameState& game, float playerX, float playerY)
{
    constexpr float mapObjectSize = 64.0f;
    const float objectHalfSize = mapObjectSize * 0.5f;
    const float playerHalfSize = static_cast<float>(game.player.size) * 0.35f;

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
        const float playerTop = playerY - playerHalfSize;
        const float playerBottom = playerY + playerHalfSize;

        const bool overlapX = playerRight > objectLeft && playerLeft < objectRight;
        const bool overlapY = playerBottom > objectTop && playerTop < objectBottom;

        if (overlapX && overlapY)
        {
            return true;
        }
    }

    return false;
}

inline void UpdateGame(GameState& game, float deltaTime)
{
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

    UpdateTriggers(game);
}
