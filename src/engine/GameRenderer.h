#pragma once

#include <Windows.h>

#include <cmath>

#include "GameTypes.h"

inline void RenderGame(HDC hdc, const RECT& clientRect, const GameState& game)
{
    HBRUSH bgBrush = CreateSolidBrush(RGB(22, 34, 26));
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(40, 64, 44));
    HGDIOBJ oldPen = SelectObject(hdc, gridPen);

    const int tileSize = 32;
    const int gridOffsetX = static_cast<int>(std::fmod(-game.player.x, static_cast<float>(tileSize)));
    const int gridOffsetY = static_cast<int>(std::fmod(-game.player.y, static_cast<float>(tileSize)));

    for (int x = gridOffsetX; x < width; x += tileSize)
    {
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, height);
    }

    for (int y = gridOffsetY; y < height; y += tileSize)
    {
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, width, y);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    const int halfSize = game.player.size / 2;
    const int centerX = width / 2;
    const int centerY = height / 2;

    RECT playerRect{};
    playerRect.left = centerX - halfSize;
    playerRect.top = centerY - halfSize;
    playerRect.right = centerX + halfSize;
    playerRect.bottom = centerY + halfSize;

    HBRUSH playerBrush = CreateSolidBrush(RGB(176, 220, 255));
    FillRect(hdc, &playerRect, playerBrush);
    DeleteObject(playerBrush);
}
