#pragma once

#include <SFML/Graphics.hpp>

#include <cstdio>

#include "MenuFont.h"

// Verwaltet den Titelbildschirm; Start und Beenden führt die Hauptschleife anhand von Action aus.
class MainMenu
{
public:
	enum class Action { None, StartGame, ExitGame };

	bool Load()
	{
		if (!LoadMenuFont(font))
		{
			return false;
		}
		if (!background.loadFromFile("Assets/UI/title_screen.png"))
		{
			std::fprintf(stderr, "Failed to load Assets/UI/title_screen.png.\n");
			return false;
		}
		return true;
	}

	void Reset() { selected = 0; }

	Action HandleKey(sf::Keyboard::Key key)
	{
		if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Down)
		{
			selected = 1 - selected;
		}
		else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
		{
			return selected == 0 ? Action::StartGame : Action::ExitGame;
		}
		return Action::None;
	}

	Action HandleClick(sf::Vector2f position, sf::Vector2u resolution)
	{
		for (int index = 0; index < 2; ++index)
		{
			if (ButtonBounds(index, resolution).contains(position))
			{
				selected = index;
				return index == 0 ? Action::StartGame : Action::ExitGame;
			}
		}
		return Action::None;
	}

	void Draw(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const sf::Vector2u textureSize = background.getSize();
		sf::Sprite sprite(background);
		sprite.setScale({ static_cast<float>(resolution.x) / textureSize.x,
						  static_cast<float>(resolution.y) / textureSize.y });
		target.draw(sprite);

		for (int index = 0; index < 2; ++index)
		{
			const sf::FloatRect bounds = ButtonBounds(index, resolution);
			sf::RectangleShape button(bounds.size);
			button.setPosition(bounds.position);
			button.setFillColor(index == selected ? sf::Color(72, 81, 65, 235) : sf::Color(28, 35, 32, 235));
			button.setOutlineThickness(2.0f);
			button.setOutlineColor(index == selected ? sf::Color(255, 220, 140) : sf::Color(160, 150, 115));
			target.draw(button);

			sf::Text label(font, index == 0 ? "Start Game" : "Exit Game", 27);
			const sf::FloatRect textBounds = label.getLocalBounds();
			label.setOrigin({ textBounds.position.x + textBounds.size.x * 0.5f,
							  textBounds.position.y + textBounds.size.y * 0.5f });
			label.setPosition({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
			target.draw(label);
		}
	}

private:
	static sf::FloatRect ButtonBounds(int index, sf::Vector2u resolution)
	{
		return { { static_cast<float>(resolution.x) * 0.5f - 150.0f,
				   static_cast<float>(resolution.y) * 0.55f + index * 75.0f }, { 300.0f, 54.0f } };
	}

	sf::Font font;
	sf::Texture background;
	int selected = 0;
};
