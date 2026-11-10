#pragma once

#include <SFML/Graphics.hpp>

#include <cmath>
#include <cstdio>

#include "GameTypes.h"

inline void RenderGame(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    target.clear(sf::Color(22, 34, 26));

    const int tileSize = 32;
    const int gridOffsetX = static_cast<int>(std::fmod(-game.player.x, static_cast<float>(tileSize)));
    const int gridOffsetY = static_cast<int>(std::fmod(-game.player.y, static_cast<float>(tileSize)));

    sf::VertexArray grid(sf::PrimitiveType::Lines);
    const sf::Color gridColor(40, 64, 44);

    for (int x = gridOffsetX; x < viewport.width; x += tileSize)
    {
        grid.append(sf::Vertex({ static_cast<float>(x), 0.0f }, gridColor));
        grid.append(sf::Vertex({ static_cast<float>(x), static_cast<float>(viewport.height) }, gridColor));
    }

    for (int y = gridOffsetY; y < viewport.height; y += tileSize)
    {
        grid.append(sf::Vertex({ 0.0f, static_cast<float>(y) }, gridColor));
        grid.append(sf::Vertex({ static_cast<float>(viewport.width), static_cast<float>(y) }, gridColor));
    }

    target.draw(grid);

    const float playerSize = static_cast<float>(game.player.size);
    sf::RectangleShape player({ playerSize, playerSize });
    player.setFillColor(sf::Color(176, 220, 255));
    player.setOrigin({ playerSize * 0.5f, playerSize * 0.5f });
    player.setPosition({ viewport.width * 0.5f, viewport.height * 0.5f });

    target.draw(player);

    static sf::Font debugFont;
    static const bool debugFontLoaded = debugFont.openFromFile("C:/Windows/Fonts/consola.ttf") ||
                                        debugFont.openFromFile("C:/Windows/Fonts/arial.ttf");

    if (!debugFontLoaded)
    {
        return;
    }

    char coordinateBuffer[64];
    std::snprintf(coordinateBuffer, sizeof(coordinateBuffer), "X: %.1f  Y: %.1f", game.player.x, game.player.y);

    sf::Text coordinateText(debugFont, coordinateBuffer, 16);
    coordinateText.setFillColor(sf::Color(230, 230, 230));

    const sf::FloatRect bounds = coordinateText.getLocalBounds();
    coordinateText.setPosition({ static_cast<float>(viewport.width) - bounds.size.x - 12.0f, 8.0f });

    target.draw(coordinateText);
}
