#pragma once

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

struct Viewport
{
	int width = 0;
	int height = 0;
};

struct GameState
{
	Player player;
	InputState input;
};
