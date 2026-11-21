#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <string>

#include "MenuFont.h"

class DialogBox
{
public:
	bool Open(const std::string& text)
	{
		if (!fontLoaded)
		{
			fontLoaded = LoadMenuFont(font);
			if (!fontLoaded) return false;
		}
		message = text;
		open = true;
		return true;
	}

	bool IsOpen() const { return open; }

	void HandleKey(sf::Keyboard::Key key)
	{
		if (key == sf::Keyboard::Key::E || key == sf::Keyboard::Key::Enter ||
			key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Escape)
			open = false;
	}

	void Draw(sf::RenderTarget& target, sf::Vector2u resolution) const
	{
		const float width = static_cast<float>(resolution.x);
		const float height = static_cast<float>(resolution.y);
		sf::RectangleShape border({ width - 104.0f, 162.0f });
		border.setPosition({ 52.0f, height - 202.0f });
		border.setFillColor(sf::Color(207, 218, 223));
		target.draw(border);

		sf::RectangleShape inner({ width - 112.0f, 154.0f });
		inner.setPosition({ 56.0f, height - 198.0f });
		inner.setFillColor(sf::Color(20, 35, 75));
		inner.setOutlineThickness(-3.0f);
		inner.setOutlineColor(sf::Color(105, 146, 173));
		target.draw(inner);

		sf::Text text(font, sf::String::fromUtf8(message.begin(), message.end()), 30);
		text.setPosition({ 84.0f, height - 180.0f });
		text.setFillColor(sf::Color::White);
		target.draw(text);

		sf::Text hint(font, "E / Enter / Leertaste / Esc", 16);
		hint.setFillColor(sf::Color(183, 199, 215));
		const auto bounds = hint.getLocalBounds();
		hint.setPosition({ width - 84.0f - bounds.size.x, height - 72.0f });
		target.draw(hint);
	}

private:
	sf::Font font;
	std::string message;
	bool fontLoaded = false;
	bool open = false;
};
