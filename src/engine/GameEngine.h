#pragma once

#include <algorithm>
#include <cmath>

#include "GameTypes.h"

inline void InitializeGameState(GameState& game)
{
    game.player.x = 0.0f;
    game.player.y = 0.0f;
}

inline void SetMovementState(InputState& input, bool up, bool down, bool left, bool right)
{
    input.up = up;
    input.down = down;
    input.left = left;
    input.right = right;
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

    game.player.x += moveX * game.player.speed * deltaTime;
    game.player.y += moveY * game.player.speed * deltaTime;
}
