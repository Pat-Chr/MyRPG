#pragma once

#include <SFML/Graphics.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <utility>

#include "GameTypes.h"

inline sf::Vector2f WorldToScreen(float worldX, float worldY, const GameState& game, const Viewport& viewport)
{
    const float halfWidth = static_cast<float>(viewport.width) * 0.5f;
    const float halfHeight = static_cast<float>(viewport.height) * 0.5f;

    return sf::Vector2f((worldX - game.player.x) + halfWidth, (worldY - game.player.y) + halfHeight);
}

inline sf::Texture* GetCachedTexture(const std::string& path)
{
    static std::unordered_map<std::string, sf::Texture> textureCache;

    auto existing = textureCache.find(path);
    if (existing != textureCache.end())
    {
        return &existing->second;
    }

    sf::Texture texture;
    if (!texture.loadFromFile(path))
    {
        return nullptr;
    }

    texture.setSmooth(false);

    auto [created, inserted] = textureCache.emplace(path, std::move(texture));
    if (!inserted)
    {
        return nullptr;
    }

    return &created->second;
}

inline void RenderMapObjects(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    constexpr float objectSize = 64.0f;

    for (const MapObject& object : game.activeMap.objects)
    {
        const sf::Vector2f screenPos = WorldToScreen(object.x, object.y, game, viewport);

        if (sf::Texture* texture = GetCachedTexture(object.spritePath))
        {
            sf::Sprite sprite(*texture);
            const sf::Vector2u textureSize = texture->getSize();

            if (textureSize.x > 0u && textureSize.y > 0u)
            {
                sprite.setOrigin({ textureSize.x * 0.5f, textureSize.y * 0.5f });
                sprite.setScale({ objectSize / textureSize.x, objectSize / textureSize.y });
            }

            sprite.setPosition(screenPos);
            target.draw(sprite);
            continue;
        }

        sf::RectangleShape fallback({ objectSize, objectSize });
        fallback.setOrigin({ objectSize * 0.5f, objectSize * 0.5f });
        fallback.setPosition(screenPos);

        if (object.type == MapObjectType::Wall)
        {
            fallback.setFillColor(sf::Color(140, 140, 140));
        }
        else if (object.type == MapObjectType::Npc)
        {
            fallback.setFillColor(sf::Color(225, 180, 130));
        }
        else
        {
            fallback.setFillColor(sf::Color(110, 138, 110));
        }

        target.draw(fallback);
    }
}

inline void RenderTriggerDebug(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    for (const TriggerZone& trigger : game.activeMap.triggers)
    {
        sf::RectangleShape area({ trigger.width, trigger.height });
        area.setPosition(WorldToScreen(trigger.x, trigger.y, game, viewport));
        area.setFillColor(sf::Color(120, 120, 0, 35));
        area.setOutlineThickness(1.0f);
        area.setOutlineColor(trigger.isPlayerInside ? sf::Color(255, 220, 120) : sf::Color(180, 180, 90));

        target.draw(area);
    }
}

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

    RenderMapObjects(target, viewport, game);
    RenderTriggerDebug(target, viewport, game);

    const float playerSize = static_cast<float>(game.player.size);

    static sf::Texture playerTexture;
    static const bool playerTextureLoaded = playerTexture.loadFromFile("Assets/Sprites/Characters/main_character.png");

    if (playerTextureLoaded)
    {
        playerTexture.setSmooth(false);

        sf::Sprite playerSprite(playerTexture);
        const sf::Vector2u textureSize = playerTexture.getSize();

        if (textureSize.x > 0u && textureSize.y > 0u)
        {
            playerSprite.setOrigin({ textureSize.x * 0.5f, textureSize.y * 0.5f });
            playerSprite.setScale({ playerSize / textureSize.x, playerSize / textureSize.y });
        }

        playerSprite.setPosition({ viewport.width * 0.5f, viewport.height * 0.5f });
        target.draw(playerSprite);
    }
    else
    {
        sf::RectangleShape player({ playerSize, playerSize });
        player.setFillColor(sf::Color(176, 220, 255));
        player.setOrigin({ playerSize * 0.5f, playerSize * 0.5f });
        player.setPosition({ viewport.width * 0.5f, viewport.height * 0.5f });

        target.draw(player);
    }

    static sf::Font debugFont;
    static const bool debugFontLoaded = debugFont.openFromFile("C:/Windows/Fonts/consola.ttf") ||
                                        debugFont.openFromFile("C:/Windows/Fonts/arial.ttf");

    if (!debugFontLoaded)
    {
        return;
    }

    char coordinateBuffer[64];
    std::snprintf(coordinateBuffer, sizeof(coordinateBuffer), "Map: %s   X: %.1f  Y: %.1f", game.activeMap.id.c_str(), game.player.x, game.player.y);

    sf::Text coordinateText(debugFont, coordinateBuffer, 16);
    coordinateText.setFillColor(sf::Color(230, 230, 230));

    const sf::FloatRect bounds = coordinateText.getLocalBounds();
    coordinateText.setPosition({ static_cast<float>(viewport.width) - bounds.size.x - 12.0f, 8.0f });

    target.draw(coordinateText);

    if (!game.lastTriggeredEvent.empty())
    {
        std::string triggerLine = "Trigger: " + game.lastTriggeredEvent;
        sf::Text triggerText(debugFont, triggerLine, 16);
        triggerText.setFillColor(sf::Color(255, 230, 140));
        triggerText.setPosition({ 12.0f, 8.0f });
        target.draw(triggerText);
    }
}
