#pragma once

#include <Windows.h>

#include <algorithm>
#include <cmath>

#include "GameTypes.h"

inline void InitializeGameState(GameState& game, const RECT& clientRect)
{
    (void)clientRect;
    game.player.x = 0.0f;
    game.player.y = 0.0f;
    game.lastTick = GetTickCount64();
}

inline void SetMovementKey(InputState& input, WPARAM key, bool isPressed)
{
    switch (key)
    {
    case 'W':
    case VK_UP:
        input.up = isPressed;
        break;
    case 'S':
    case VK_DOWN:
        input.down = isPressed;
        break;
    case 'A':
    case VK_LEFT:
        input.left = isPressed;
        break;
    case 'D':
    case VK_RIGHT:
        input.right = isPressed;
        break;
    default:
        break;
    }
}

inline void UpdateGame(GameState& game, const RECT& clientRect, float deltaTime)
{
    (void)clientRect;

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

    game.player.x += moveX * game.player.speed * deltaTime;
    game.player.y += moveY * game.player.speed * deltaTime;
}
