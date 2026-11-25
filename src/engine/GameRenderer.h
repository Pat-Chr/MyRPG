#pragma once

#include <Windows.h>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AnimationLoader.h"
#include "GameTypes.h"
#include "MagicRenderer.h"
#include "WeatherRenderer.h"

/**
 * @brief Converts world coordinates to screen coordinates.
 * 
 * Since SFML does not provide a native camera, the player acts as a virtual
 * camera center. World coordinates are shifted relative to the player and
 * centered on the screen (half of the viewport).
 * 
 * @param worldX X-coordinate in the game world.
 * @param worldY Y-coordinate in the game world.
 * @param game Current game state containing the player position.
 * @param viewport Current viewport dimensions.
 * @return sf::Vector2f Screen position for the given world coordinates.
 */
inline sf::Vector2f WorldToScreen(float worldX, float worldY, const GameState& game, const Viewport& viewport)
{
    const float halfWidth = static_cast<float>(viewport.width) * 0.5f;
    const float halfHeight = static_cast<float>(viewport.height) * 0.5f;

    return sf::Vector2f((worldX - game.player.x) + halfWidth + game.cameraShake.offsetX,
                        (worldY - game.player.y) + halfHeight + game.cameraShake.offsetY);
}

/**
 * @brief Loads and caches SFML textures for efficient rendering.
 * 
 * Uses a thread-safe unordered map to cache loaded textures. It avoids repeated
 * file loads and reduces GPU overhead for frequent texture access. Textures use
 * setSmooth(false) for pixel-exact rendering without naive interpolation.
 * 
 * @param path Path to the texture file in the filesystem.
 * @return sf::Texture* Pointer to the loaded texture, or nullptr on load/cache failure.
 */
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

// The NPC file contains all directions and optional walking animations.
inline const SpriteAnimation* GetNpcAnimation(const MapObject& object, const sf::Texture& texture)
{
    if (object.type != MapObjectType::Npc || object.hasTile) return nullptr;
    static std::unordered_map<std::string, std::optional<NpcAnimations>> cache;
    auto found = cache.find(object.spritePath);
    if (found == cache.end())
    {
        const std::string path = "Assets/Animations/Characters/" + std::filesystem::path(object.spritePath).stem().string() + ".anim";
        std::error_code ec;
        const bool exists = std::filesystem::exists(path, ec);
        std::optional<NpcAnimations> loaded;
        if (ec)
            std::fprintf(stderr, "Failed to inspect NPC animation: %s (%s)\n", path.c_str(), ec.message().c_str());
        else if (exists)
        {
            NpcAnimations animations;
            bool valid = LoadNpcAnimationsFromFile(path, animations);
            for (std::size_t i = 0; valid && i < animations.size(); ++i)
            {
                if (!animations[i]) continue;
                const SpriteAnimation& animation = *animations[i];
                if (i < 4 && animation.texturePath != object.spritePath)
                {
                    std::fprintf(stderr, "NPC idle animation texture does not match object: %s\n", path.c_str());
                    valid = false; break;
                }
                const sf::Texture* clipTexture = animation.texturePath == object.spritePath ? &texture : GetCachedTexture(animation.texturePath);
                if (!clipTexture)
                {
                    std::fprintf(stderr, "Failed to load NPC animation texture '%s': %s\n", animation.texturePath.c_str(), path.c_str());
                    valid = false; break;
                }
                const sf::Vector2u size = clipTexture->getSize();
                for (const sf::IntRect& frame : animation.frames)
                {
                    if (static_cast<unsigned>(frame.position.x) > size.x || static_cast<unsigned>(frame.position.y) > size.y ||
                        static_cast<unsigned>(frame.size.x) > size.x - static_cast<unsigned>(frame.position.x) ||
                        static_cast<unsigned>(frame.size.y) > size.y - static_cast<unsigned>(frame.position.y))
                    {
                        std::fprintf(stderr, "NPC animation frame is outside sprite sheet '%s': %s\n", animation.texturePath.c_str(), path.c_str());
                        valid = false; break;
                    }
                }
            }
            if (valid) loaded = std::move(animations);
        }
        found = cache.emplace(object.spritePath, std::move(loaded)).first;
    }
    if (!found->second) return nullptr;
    const std::size_t direction = static_cast<std::size_t>(object.npcDirection);
    const std::size_t idle = direction < 4 ? direction : 0;
    const std::size_t clip = object.npcMoving && (*found->second)[idle + 4] ? idle + 4 : idle;
    return &*(*found->second)[clip];
}

inline std::size_t GetAnimationFrameIndex(const SpriteAnimation& animation, double elapsed)
{
    return static_cast<std::size_t>(std::fmod(elapsed / animation.frameDuration, static_cast<double>(animation.frames.size())));
}

inline sf::IntRect GetNpcFrame(const SpriteAnimation& animation, double elapsed)
{
    return animation.frames[GetAnimationFrameIndex(animation, elapsed)];
}

struct SpriteShadowData
{
    std::vector<std::optional<sf::Texture>> frames;
    std::vector<float> bottoms;
};

inline SpriteShadowData BuildSpriteShadowData(const SpriteAnimation& animation, const sf::Texture& texture, const char* owner)
{
    constexpr int shadowBlurX = 3;
    constexpr int shadowBlurY = 7;
    SpriteShadowData data;
    const sf::Image mask = texture.copyToImage();
    const sf::Vector2u maskSize = mask.getSize();
    if (maskSize.x == 0u || maskSize.y == 0u)
    {
        std::fprintf(stderr, "Failed to create %s shadow mask: %s\n", owner, animation.texturePath.c_str());
        return data;
    }

    for (const sf::IntRect& frame : animation.frames)
    {
        float bottom = static_cast<float>(frame.size.y);
        const unsigned int left = (std::min)(static_cast<unsigned int>(frame.position.x), maskSize.x);
        const unsigned int top = (std::min)(static_cast<unsigned int>(frame.position.y), maskSize.y);
        const unsigned int right = static_cast<unsigned int>((std::min)(
            static_cast<std::uint64_t>(frame.position.x) + frame.size.x, static_cast<std::uint64_t>(maskSize.x)));
        const unsigned int endY = static_cast<unsigned int>((std::min)(
            static_cast<std::uint64_t>(frame.position.y) + frame.size.y, static_cast<std::uint64_t>(maskSize.y)));
        bool found = false;
        for (unsigned int y = endY; y > top && !found; --y)
        {
            for (unsigned int x = left; x < right; ++x)
            {
                if (mask.getPixel({ x, y - 1 }).a != 0)
                {
                    bottom = static_cast<float>(y - static_cast<unsigned int>(frame.position.y));
                    found = true;
                    break;
                }
            }
        }

        data.bottoms.push_back(bottom);
        data.frames.emplace_back();
        if (static_cast<std::uint64_t>(frame.position.x) + frame.size.x > maskSize.x ||
            static_cast<std::uint64_t>(frame.position.y) + frame.size.y > maskSize.y)
        {
            std::fprintf(stderr, "%s shadow frame is outside sprite sheet: %s\n", owner, animation.texturePath.c_str());
            continue;
        }

        const int width = frame.size.x + 2 * shadowBlurX;
        const int height = frame.size.y + 2 * shadowBlurY;
        std::vector<float> horizontal(static_cast<std::size_t>(width) * height, 0.0f);
        for (int y = 0; y < frame.size.y; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                unsigned int sum = 0;
                for (int dx = -shadowBlurX; dx <= shadowBlurX; ++dx)
                {
                    const int sourceX = x - shadowBlurX + dx;
                    if (sourceX >= 0 && sourceX < frame.size.x)
                    {
                        sum += mask.getPixel({ static_cast<unsigned int>(frame.position.x + sourceX),
                                               static_cast<unsigned int>(frame.position.y + y) }).a;
                    }
                }
                horizontal[static_cast<std::size_t>(y + shadowBlurY) * width + x] =
                    static_cast<float>(sum) / (2 * shadowBlurX + 1);
            }
        }

        sf::Image blurred({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }, sf::Color(64, 64, 64, 0));
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float sum = 0.0f;
                for (int dy = -shadowBlurY; dy <= shadowBlurY; ++dy)
                {
                    const int sourceY = y + dy;
                    if (sourceY >= 0 && sourceY < height)
                        sum += horizontal[static_cast<std::size_t>(sourceY) * width + x];
                }
                const auto alpha = static_cast<std::uint8_t>(std::round(sum / (2 * shadowBlurY + 1)));
                blurred.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color(64, 64, 64, alpha));
            }
        }

        auto& shadowFrame = data.frames.back();
        shadowFrame.emplace();
        if (!shadowFrame->loadFromImage(blurred))
        {
            std::fprintf(stderr, "Failed to upload %s shadow mask: %s\n", owner, animation.texturePath.c_str());
            shadowFrame.reset();
        }
        else
        {
            shadowFrame->setSmooth(true);
        }
    }
    return data;
}

inline void DrawSpriteShadow(sf::RenderTarget& target, const SpriteShadowData& data, std::size_t frameIndex,
                             const sf::IntRect& frame, const sf::Vector2f& spritePosition,
                             float scaleX, float scaleY)
{
    constexpr int shadowBlurX = 3;
    constexpr int shadowBlurY = 7;
    constexpr float shadowHeightScale = 0.4f;
    if (frameIndex >= data.frames.size() || frameIndex >= data.bottoms.size() || !data.frames[frameIndex].has_value())
        return;

    const float frameHeight = static_cast<float>(frame.size.y);
    sf::Sprite shadow(*data.frames[frameIndex]);
    shadow.setOrigin({ static_cast<float>(frame.size.x) * 0.5f + shadowBlurX, data.bottoms[frameIndex] + shadowBlurY });
    shadow.setScale({ scaleX, -scaleY * shadowHeightScale });
    shadow.setPosition({ spritePosition.x, spritePosition.y + (data.bottoms[frameIndex] - frameHeight * 0.5f) * scaleY - 5.0f });
    shadow.setColor(sf::Color(255, 255, 255, 100));
    target.draw(shadow);
}

inline const SpriteShadowData* GetNpcShadowData(const MapObject& object, const SpriteAnimation& animation, const sf::Texture& texture)
{
    static std::unordered_map<std::string, std::optional<SpriteShadowData>> cache;
    const std::string key = object.spritePath + "#" + std::to_string(static_cast<int>(object.npcDirection)) +
        "#" + animation.texturePath + "#" + (object.npcMoving ? "walk" : "idle");
    auto found = cache.find(key);
    if (found == cache.end())
    {
        found = cache.emplace(key, BuildSpriteShadowData(animation, texture, "NPC")).first;
    }
    return &found->second.value();
}

/**
 * @brief Renders the default ground beneath the map.
 * 
 * Loads the ground texture from the configured path and renders every tile visible
 * around the player. The calculation area extends two tiles beyond the viewport
 * for seamless edge rendering.
 * 
 * If no texture is configured or loading fails, a simple green rectangle
 * (sf::Color(72, 152, 76)) is used as the fallback.
 * 
 * @param target SFML target used for rendering, for example a window or RenderTarget.
 * @param viewport Current viewport dimensions.
 * @param game Current game state containing the ground texture path.
 */
inline void RenderDefaultGround(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    if (game.activeMap.defaultGroundPath.empty())
    {
        return;
    }

    constexpr float tileSize = 64.0f;
    const float halfWidth = static_cast<float>(viewport.width) * 0.5f;
    const float halfHeight = static_cast<float>(viewport.height) * 0.5f;
    // Calculates the tile area around the player, expanded by one tile in every direction.
    const int firstTileX = static_cast<int>(std::floor((game.player.x - halfWidth) / tileSize)) - 1;
    const int lastTileX = static_cast<int>(std::ceil((game.player.x + halfWidth) / tileSize)) + 1;
    const int firstTileY = static_cast<int>(std::floor((game.player.y - halfHeight) / tileSize)) - 1;
    const int lastTileY = static_cast<int>(std::ceil((game.player.y + halfHeight) / tileSize)) + 1;
    
    sf::Texture* texture = GetCachedTexture(game.activeMap.defaultGroundPath);

    if (texture != nullptr)
    {
        sf::Sprite ground(*texture);
        const sf::Vector2u textureSize = texture->getSize();

        // Centers the sprite origin and scales it to the desired tile size.
        if (textureSize.x > 0u && textureSize.y > 0u)
        {
            ground.setOrigin({ textureSize.x * 0.5f, textureSize.y * 0.5f });
            ground.setScale({ tileSize / textureSize.x, tileSize / textureSize.y });
        }

        for (int tileY = firstTileY; tileY <= lastTileY; ++tileY)
        {
            for (int tileX = firstTileX; tileX <= lastTileX; ++tileX)
            {
                ground.setPosition(WorldToScreen(tileX * tileSize, tileY * tileSize, game, viewport));
                target.draw(ground);
            }
        }

        return;
    }

    // Fallback: a simple green rectangle when no texture is available.
    sf::RectangleShape ground({ tileSize, tileSize });
    ground.setOrigin({ tileSize * 0.5f, tileSize * 0.5f });
    ground.setFillColor(sf::Color(72, 152, 76));

    for (int tileY = firstTileY; tileY <= lastTileY; ++tileY)
    {
        for (int tileX = firstTileX; tileX <= lastTileX; ++tileX)
        {
            ground.setPosition(WorldToScreen(tileX * tileSize, tileY * tileSize, game, viewport));
            target.draw(ground);
        }
    }
}

/**
 * @brief Renders all map objects (walls, NPCs, and interactive elements).
 * 
 * Iterates through all objects in the active map and renders each as a texture
 * sprite or colored rectangle fallback according to object type:
 * - Wall: gray (140, 140, 140)
 * - Npc: beige (225, 180, 130)
 * - Ground: green (72, 152, 76)
 * - Other: light green (110, 138, 110)
 * 
 * @param target SFML target used for rendering.
 * @param viewport Current viewport dimensions.
 * @param game Current game state containing the map-object list.
 */
inline void RenderMapObjects(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    constexpr float objectSize = 64.0f;

    for (const MapObject& object : game.activeMap.objects)
    {
        const sf::Vector2f screenPos = WorldToScreen(object.x, object.y, game, viewport);

        if (sf::Texture* texture = GetCachedTexture(object.spritePath))
        {
            sf::Sprite sprite(*texture);
            sf::Vector2u textureSize = texture->getSize();
            const SpriteAnimation* animation = nullptr;
            std::size_t frameIndex = 0;
            sf::IntRect frame;
            float animationScale = 1.0f;
            if (object.hasTile)
            {
                sprite.setTextureRect(sf::IntRect({ object.tileX, object.tileY }, { object.tileWidth, object.tileHeight }));
                textureSize = { static_cast<unsigned>(object.tileWidth), static_cast<unsigned>(object.tileHeight) };
            }
            else if ((animation = GetNpcAnimation(object, *texture)) != nullptr)
            {
                frameIndex = GetAnimationFrameIndex(*animation, object.npcAnimationTime);
                frame = animation->frames[frameIndex];
                if (animation->texturePath != object.spritePath)
                {
                    texture = GetCachedTexture(animation->texturePath);
                    sprite.setTexture(*texture);
                }
                sprite.setTextureRect(frame);
                textureSize = { static_cast<unsigned>(frame.size.x), static_cast<unsigned>(frame.size.y) };
                animationScale = animation->scale;
            }

            // Centers and scales the sprite to the standard object size (64x64).
            if (textureSize.x > 0u && textureSize.y > 0u)
            {
                sprite.setOrigin({ textureSize.x * 0.5f, textureSize.y * 0.5f });
                sprite.setScale({ animationScale * objectSize / textureSize.x, animationScale * objectSize / textureSize.y });
            }

            sprite.setPosition(screenPos);
            if (animation != nullptr)
            {
                const SpriteShadowData* shadowData = GetNpcShadowData(object, *animation, *texture);
                DrawSpriteShadow(target, *shadowData, frameIndex, frame, screenPos, sprite.getScale().x, sprite.getScale().y);
            }
            target.draw(sprite);
            continue;
        }

        // Fallback rectangle when no texture is available.
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
        else if (object.type == MapObjectType::Ground)
        {
            fallback.setFillColor(sf::Color(72, 152, 76));
        }
        else
        {
            fallback.setFillColor(sf::Color(110, 138, 110));
        }

        target.draw(fallback);
    }
}

/**
 * @brief Rendert leuchtende Trigger-Zonen (Option `glow` im TRIGGER-Befehl) per Fragment-Shader.
 *
 * Works in Release builds as well. Without shader support, a simple
 * additives Rechteck als Fallback gezeichnet.
 */
inline void RenderTriggerGlow(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    static const char* fragmentSource = R"(
        uniform vec2 size;
        uniform float time;
        uniform vec3 glowColor;
        void main()
        {
            vec2 p = gl_TexCoord[0].xy;
            vec2 d = abs(p - size * 0.5) - size * 0.5;
            float sdf = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
            float pulse = 0.75 + 0.25 * sin(time * 3.0);
            float a = sdf < 0.0
                ? 0.18 + 0.45 * exp(sdf / 10.0)
                : 0.7 * exp(-sdf / 9.0);
            a *= pulse;
            gl_FragColor = vec4(glowColor * a, a);
        }
    )";

    static sf::Shader shader;
    static const bool shaderReady = sf::Shader::isAvailable()
        && shader.loadFromMemory(std::string_view(fragmentSource), sf::Shader::Type::Fragment);
    static sf::Clock clock;

    constexpr float pad = 28.0f;
    sf::RenderStates states;
    states.blendMode = sf::BlendAdd;
    if (shaderReady)
    {
        shader.setUniform("time", clock.getElapsedTime().asSeconds());
        shader.setUniform("glowColor", sf::Glsl::Vec3(1.0f, 0.82f, 0.35f));
        states.shader = &shader;
    }

    for (const TriggerZone& trigger : game.activeMap.triggers)
    {
        if (!trigger.glow)
        {
            continue;
        }

        const sf::Vector2f origin = WorldToScreen(trigger.x, trigger.y, game, viewport);
        if (!shaderReady)
        {
            sf::RectangleShape area({ trigger.width, trigger.height });
            area.setPosition(origin);
            area.setFillColor(sf::Color(255, 210, 90, 70));
            target.draw(area, states);
            continue;
        }

        const float w = trigger.width;
        const float h = trigger.height;
        const sf::Color white = sf::Color::White;
        const sf::Vector2f tl{ origin.x - pad, origin.y - pad };
        const sf::Vector2f tr{ origin.x + w + pad, origin.y - pad };
        const sf::Vector2f bl{ origin.x - pad, origin.y + h + pad };
        const sf::Vector2f br{ origin.x + w + pad, origin.y + h + pad };
        const sf::Vector2f ttl{ -pad, -pad };
        const sf::Vector2f ttr{ w + pad, -pad };
        const sf::Vector2f tbl{ -pad, h + pad };
        const sf::Vector2f tbr{ w + pad, h + pad };

        const sf::Vertex quad[6] = {
            { tl, white, ttl }, { tr, white, ttr }, { bl, white, tbl },
            { tr, white, ttr }, { br, white, tbr }, { bl, white, tbl }
        };

        shader.setUniform("size", sf::Glsl::Vec2(w, h));
        target.draw(quad, 6, sf::PrimitiveType::Triangles, states);
    }
}

#if defined(_DEBUG) || defined(MYRPG_MAP_EDITOR)
/**
 * @brief Renders debug overlays for trigger zones.
 * 
 * Displays translucent rectangles representing trigger areas. The outline changes
 * color based on whether the player is inside the trigger:
 * - Player inside: light yellow (255, 220, 120)
 * - Player outside: dark yellow (180, 180, 90)
 * 
 * The fill is dark yellow with 35 alpha (120, 120, 0, 35).
 * 
 * @param target SFML target used for rendering.
 * @param viewport Current viewport dimensions.
 * @param game Current game state containing the trigger list and player position.
 */
inline void RenderTriggerDebug(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    for (const TriggerZone& trigger : game.activeMap.triggers)
    {
        sf::RectangleShape area({ trigger.width, trigger.height });
        area.setPosition(WorldToScreen(trigger.x, trigger.y, game, viewport));
        area.setFillColor(sf::Color(120, 120, 0, 35)); // Dark yellow with 35% opacity.
        area.setOutlineThickness(1.0f);
        // Outline color changes according to the player's position in the trigger.
        area.setOutlineColor(trigger.isPlayerInside ? sf::Color(255, 220, 120) : sf::Color(180, 180, 90));

        target.draw(area);
    }
}
#endif

/**
 * @brief Main rendering function for the complete game scene.
 * 
 * Renders the complete scene in the following order:
 * 1. Clear the background (dark green: 22, 34, 26)
 * 2. Draw the debug grid (Debug build only)
 * 3. Draw the ground texture and tiles
 * 4. Draw map objects (walls, NPCs, and so on)
 * 5. Draw trigger zones (Debug build only)
 * 6. Draw the player character (texture or fallback rectangle)
 * 7. Draw magic effects and weather
 * 8. Optionally draw FPS and debug UI (coordinates and events are Debug-build only)
 * 9. Draw the map-transition overlay (fade-in/fade-out effect)
 * 
 * @param target SFML target used for rendering.
 * @param viewport Current viewport dimensions.
 * @param game Current game state.
 * @param fps Current frames per second for the UI display.
 * @param showFps Whether the FPS display is visible.
 */
inline sf::FloatRect RenderGame(sf::RenderTarget& target, const Viewport& viewport, const GameState& game,
                                float fps, bool showFps)
{
    sf::FloatRect debugCoordinatesBounds{};

    // Clear the background with a dark green forest/rainforest tone.
    target.clear(sf::Color(22, 34, 26));

    static sf::RenderTexture heatScene;
    static sf::Vector2u attemptedHeatSize{};
    static bool heatSizeAttempted = false;
    static bool heatSceneReady = false;
    const sf::Vector2u sceneSize{ static_cast<unsigned int>(viewport.width), static_cast<unsigned int>(viewport.height) };
    const bool heatRequested = game.weather.effectId == "heat" && GetWeatherShader("heat") != nullptr;
    if (heatRequested && (!heatSizeAttempted || sceneSize != attemptedHeatSize))
    {
        attemptedHeatSize = sceneSize;
        heatSizeAttempted = true;
        heatSceneReady = sceneSize.x > 0 && sceneSize.y > 0 && heatScene.resize(sceneSize);
        if (!heatSceneReady)
        {
            std::fprintf(stderr, "Failed to create heat scene texture (%u x %u).\n", sceneSize.x, sceneSize.y);
        }
    }
    const bool renderHeat = heatRequested && heatSceneReady && sceneSize == attemptedHeatSize;
    {
    sf::RenderTarget& sceneTarget = renderHeat ? static_cast<sf::RenderTarget&>(heatScene) : target;
    sf::RenderTarget& target = sceneTarget;
    if (renderHeat)
    {
        target.clear(sf::Color(22, 34, 26));
    }

#ifdef _DEBUG
    // Debug grid: 32x32-pixel tiles relative to the player's position.
    const int tileSize = 32;
    const int gridOffsetX = static_cast<int>(std::fmod(-game.player.x + game.cameraShake.offsetX, static_cast<float>(tileSize)));
    const int gridOffsetY = static_cast<int>(std::fmod(-game.player.y + game.cameraShake.offsetY, static_cast<float>(tileSize)));

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
#endif

    RenderDefaultGround(target, viewport, game);
    RenderMapObjects(target, viewport, game);
    RenderTriggerGlow(target, viewport, game);
#ifdef _DEBUG
    RenderTriggerDebug(target, viewport, game);
#endif

    // Render the player character (animated sprite or fallback rectangle).
    const float playerSize = static_cast<float>(game.player.size);
    struct PlayerAnimation
    {
        SpriteAnimation animation;
        sf::Texture* texture = nullptr;
        SpriteShadowData shadow;
    };
    static const auto playerAnimations = []
    {
        std::array<PlayerAnimation, 12> animations;
        PlayerAnimations clips;
        if (!LoadPlayerAnimationsFromFile("Assets/Animations/Characters/player.anim", clips)) return animations;
        for (std::size_t i = 0; i < clips.size(); ++i)
        {
            animations[i].animation = std::move(*clips[i]);
            animations[i].texture = GetCachedTexture(animations[i].animation.texturePath);
            if (animations[i].texture == nullptr)
            {
                std::fprintf(stderr, "Failed to load player sprite sheet: %s\n", animations[i].animation.texturePath.c_str());
                continue;
            }
            animations[i].shadow = BuildSpriteShadowData(animations[i].animation, *animations[i].texture, "player");
        }
        return animations;
    }();
    static constexpr std::array<PlayerDirection, 8> idleDirections = {
        PlayerDirection::Down, PlayerDirection::Up, PlayerDirection::Left, PlayerDirection::Right,
        PlayerDirection::Left, PlayerDirection::Right, PlayerDirection::Left, PlayerDirection::Right
    };
    const std::size_t direction = static_cast<std::size_t>(game.player.direction);
    const PlayerAnimation& selected = game.player.moving
        ? playerAnimations[4 + direction]
        : playerAnimations[static_cast<std::size_t>(idleDirections[direction])];
    if (selected.texture != nullptr)
    {
        static sf::Clock playerAnimationClock;
        static bool wasPlayerMoving = false;
        static PlayerDirection previousDirection = PlayerDirection::Down;
        if (game.player.moving != wasPlayerMoving || game.player.direction != previousDirection)
        {
            playerAnimationClock.restart();
            wasPlayerMoving = game.player.moving;
            previousDirection = game.player.direction;
        }

        const SpriteAnimation& playerAnimation = selected.animation;
        const std::size_t frameIndex = static_cast<std::size_t>(playerAnimationClock.getElapsedTime().asSeconds() / playerAnimation.frameDuration) % playerAnimation.frames.size();
        const sf::IntRect& frame = playerAnimation.frames[frameIndex];

        sf::Sprite playerSprite(*selected.texture);
        playerSprite.setTextureRect(frame);
        const float frameHeight = static_cast<float>(frame.size.y);
        const float frameScale = (playerSize / frameHeight) * playerAnimation.scale;
        playerSprite.setOrigin({ static_cast<float>(frame.size.x) * 0.5f, frameHeight * 0.5f });
        playerSprite.setScale({ frameScale, frameScale });
        playerSprite.setPosition({ viewport.width * 0.5f + game.cameraShake.offsetX,
                                   viewport.height * 0.5f + game.cameraShake.offsetY });
        DrawSpriteShadow(target, selected.shadow, frameIndex, frame, playerSprite.getPosition(), frameScale, frameScale);
        target.draw(playerSprite);
    }
    else
    {
        // Fallback: a bright bluish-white rectangle when no texture is available.
        sf::RectangleShape player({ playerSize, playerSize });
        player.setFillColor(sf::Color(176, 220, 255));
        player.setOrigin({ playerSize * 0.5f, playerSize * 0.5f });
        player.setPosition({ viewport.width * 0.5f + game.cameraShake.offsetX,
                             viewport.height * 0.5f + game.cameraShake.offsetY });

        target.draw(player);
    }

    RenderMagicEffects(target, game.magicEffects,
                       game.player.x - game.cameraShake.offsetX,
                       game.player.y - game.cameraShake.offsetY,
                       static_cast<float>(viewport.width), static_cast<float>(viewport.height));
    }

    const sf::Vector2f worldZero = WorldToScreen(0.0f, 0.0f, game, viewport);
    const sf::Vector2f worldOrigin{ -worldZero.x, -worldZero.y };
    if (renderHeat)
    {
        heatScene.display();
        RenderHeat(target, heatScene, game.weather, static_cast<float>(viewport.width), static_cast<float>(viewport.height),
                   worldOrigin);
    }
    else if (game.weather.effectId != "heat")
    {
        RenderWeather(target, game.weather, static_cast<float>(viewport.width), static_cast<float>(viewport.height), worldOrigin);
    }

#ifdef _DEBUG
    constexpr float side = Player::hitboxSize;
    sf::RectangleShape hitbox({ side, side });
    hitbox.setOrigin({ side * 0.5f, side * 0.5f });
    hitbox.setPosition(WorldToScreen(game.player.x, Player::HitboxCenterY(game.player.y), game, viewport));
    hitbox.setFillColor(sf::Color::Transparent);
    hitbox.setOutlineColor(sf::Color(90, 255, 215));
    hitbox.setOutlineThickness(-1.0f);
    target.draw(hitbox);

    constexpr bool showDebug = true;
#else
    constexpr bool showDebug = false;
#endif
    if (showFps || showDebug)
    {
        static sf::Font overlayFont;
        static const bool fontLoaded = []
        {
            wchar_t windowsDirectory[MAX_PATH];
            const UINT length = GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
            if (length == 0 || length >= MAX_PATH)
            {
                std::fprintf(stderr, "Failed to locate the Windows font directory for the FPS overlay.\n");
                return false;
            }
            const std::filesystem::path fontDirectory = std::filesystem::path(windowsDirectory) / L"Fonts";
            if (overlayFont.openFromFile(fontDirectory / L"consola.ttf") ||
                overlayFont.openFromFile(fontDirectory / L"arial.ttf"))
            {
                return true;
            }
            std::fprintf(stderr, "Failed to load a font for the FPS overlay.\n");
            return false;
        }();

        if (fontLoaded)
        {
            char overlayBuffer[96];
#ifdef _DEBUG
            if (showFps)
            {
                std::snprintf(overlayBuffer, sizeof(overlayBuffer), "Map: %s   FPS: %.1f", game.activeMap.id.c_str(), fps);
            }
            else
            {
                std::snprintf(overlayBuffer, sizeof(overlayBuffer), "Map: %s", game.activeMap.id.c_str());
            }
#else
            std::snprintf(overlayBuffer, sizeof(overlayBuffer), "FPS: %.1f", fps);
#endif
            sf::Text overlayText(overlayFont, overlayBuffer, 16);
            overlayText.setFillColor(sf::Color(230, 230, 230));
            const sf::FloatRect bounds = overlayText.getLocalBounds();
            overlayText.setPosition({ static_cast<float>(viewport.width) - bounds.size.x - 12.0f, 8.0f });
            target.draw(overlayText);

#ifdef _DEBUG
            char coordinatesBuffer[64];
            std::snprintf(coordinatesBuffer, sizeof(coordinatesBuffer), "X: %.1f  Y: %.1f", game.player.x, game.player.y);
            sf::Text coordinatesText(overlayFont, coordinatesBuffer, 16);
            coordinatesText.setFillColor(sf::Color(230, 230, 230));
            const sf::FloatRect coordinatesTextBounds = coordinatesText.getLocalBounds();
            coordinatesText.setPosition({ static_cast<float>(viewport.width) - coordinatesTextBounds.size.x - 12.0f, 30.0f });
            debugCoordinatesBounds = coordinatesText.getGlobalBounds();
            target.draw(coordinatesText);

            if (!game.lastTriggeredEvent.empty())
            {
                std::string triggerLine = "Trigger: " + game.lastTriggeredEvent;
                sf::Text triggerText(overlayFont, triggerLine, 16);
                triggerText.setFillColor(sf::Color(255, 230, 140));
                triggerText.setPosition({ 12.0f, 8.0f });
                target.draw(triggerText);
            }
#endif
        }
    }

    // Map-transition overlay (fade-in or fade-out effect).
    if (game.mapTransition.phase != MapTransitionPhase::None)
    {
        const float duration = game.mapTransition.duration > 0.0f ? game.mapTransition.duration : 0.001f;
        const float progress = std::clamp(game.mapTransition.timer / duration, 0.0f, 1.0f);
        // Fade-out: alpha 0 → 1 (darker); fade-in: alpha 1 → 0 (lighter).
        const float alphaFactor = game.mapTransition.phase == MapTransitionPhase::FadeOut ? progress : (1.0f - progress);
        const auto alpha = static_cast<std::uint8_t>(std::clamp(alphaFactor * 255.0f, 0.0f, 255.0f));

        sf::RectangleShape fadeOverlay({ static_cast<float>(viewport.width), static_cast<float>(viewport.height) });
        fadeOverlay.setPosition({ 0.0f, 0.0f });
        fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha)); // Black overlay with variable opacity.
        target.draw(fadeOverlay);
    }

    return debugCoordinatesBounds;
}
