#pragma once

#include <SFML/Audio.hpp>

#include <optional>

class SoundEffects
{
public:
	bool Load()
	{
		if (!clickBuffer.loadFromFile("Assets/Sounds/UI/click.wav")) return false;
		click.emplace(clickBuffer);
		click->setVolume(50.0f);
		return true;
	}

	void PlayClick()
	{
		if (click) click->play();
	}

private:
	sf::SoundBuffer clickBuffer;
	std::optional<sf::Sound> click;
};
