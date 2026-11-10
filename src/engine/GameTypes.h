#pragma once

#include <Windows.h>

struct Player
{
	float x = 0.0f;
	float y = 0.0f;
	float speed = 220.0f;
	int size = 24;
};

struct InputState
{
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
};

struct GameState
{
	Player player;
	InputState input;
	ULONGLONG lastTick = 0;
};
