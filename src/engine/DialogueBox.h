#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <string>
#include <vector>

#include "MenuFont.h"

class DialogueBox
{
public:
	bool Open(const std::vector<std::string>& paragraphs, const std::string& speaker)
	{
		if (paragraphs.empty()) return false;
		if (!fontLoaded)
		{
			fontLoaded = LoadMenuFont(font);
			if (!fontLoaded) return false;
		}
		messages = paragraphs;
		currentMessage = 0;
		speakerName = speaker;
		open = true;
		return true;
	}

	bool IsOpen() const { return open; }

	void Advance()
	{
		if (open && ++currentMessage >= messages.size()) open = false;
	}

	void HandleKey(sf::Keyboard::Key key)
	{
		if (key == sf::Keyboard::Key::Escape) open = false;
		else if (key == sf::Keyboard::Key::E || key == sf::Keyboard::Key::Enter ||
			key == sf::Keyboard::Key::Space) Advance();
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

		if (!speakerName.empty())
		{
			sf::Text name(font, sf::String::fromUtf8(speakerName.begin(), speakerName.end()), 22);
			name.setPosition({ 84.0f, height - 184.0f });
			name.setFillColor(sf::Color(183, 199, 215));
			target.draw(name);
		}
		const auto& message = messages[currentMessage];
		const sf::String source = sf::String::fromUtf8(message.begin(), message.end());
		const float availableWidth = width - 168.0f;
		const float availableHeight = speakerName.empty() ? 100.0f : 70.0f;
		sf::String wrapped;
		unsigned int characterSize = 30;
		for (;;)
		{
			wrapped.clear();
			float lineWidth = 0.0f;
			std::size_t lines = 1;
			char32_t previous = 0;
			for (std::size_t i = 0; i < source.getSize(); ++i)
			{
				const char32_t character = source[i];
				if (character == U'\n')
				{
					wrapped += character;
					lineWidth = 0.0f;
					previous = 0;
					++lines;
					continue;
				}
				if (character != U' ' && (i == 0 || source[i - 1] == U' ' || source[i - 1] == U'\n'))
				{
					float wordWidth = 0.0f;
					char32_t wordPrevious = previous;
					for (std::size_t j = i; j < source.getSize() && source[j] != U' ' && source[j] != U'\n'; ++j)
					{
						wordWidth += font.getKerning(wordPrevious, source[j], characterSize) + font.getGlyph(source[j], characterSize, false).advance;
						wordPrevious = source[j];
					}
					if (lineWidth > 0.0f && lineWidth + wordWidth > availableWidth)
					{
						wrapped += U'\n';
						++lines;
						lineWidth = 0.0f;
						previous = 0;
					}
				}
				float advance = font.getKerning(previous, character, characterSize) + font.getGlyph(character, characterSize, false).advance;
				if (lineWidth > 0.0f && lineWidth + advance > availableWidth)
				{
					wrapped += U'\n';
					++lines;
					lineWidth = 0.0f;
					advance = font.getGlyph(character, characterSize, false).advance;
				}
				wrapped += character;
				lineWidth += advance;
				previous = character;
			}
			if (lines * font.getLineSpacing(characterSize) <= availableHeight || characterSize == 1) break;
			--characterSize;
		}
		sf::Text text(font, wrapped, characterSize);
		text.setPosition({ 84.0f, height - (speakerName.empty() ? 180.0f : 150.0f) });
		text.setFillColor(sf::Color::White);
		target.draw(text);

		sf::Text hint(font, "Klick / E / Enter / Leertaste | Esc", 16);
		hint.setFillColor(sf::Color(183, 199, 215));
		const auto bounds = hint.getLocalBounds();
		hint.setPosition({ width - 84.0f - bounds.size.x, height - 72.0f });
		target.draw(hint);
	}

private:
	sf::Font font;
	std::vector<std::string> messages;
	std::size_t currentMessage = 0;
	std::string speakerName;
	bool fontLoaded = false;
	bool open = false;
};
