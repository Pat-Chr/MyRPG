#pragma once

#include <Windows.h>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "AnimationLoader.h"
#include "GameTypes.h"
#include "MagicRenderer.h"
#include "WeatherRenderer.h"

/**
 * @brief Wandelt Weltkoordinaten in Bildschirmkoordinaten um.
 * 
 * Da SFML die Kamera nicht nativ unterstützt, wird der Spieler als virtuelles
 * Kamera-Zentrum verwendet. Die Weltkoordinaten werden relativ zum Spieler
 * verschoben und dann auf den Bildschirm zentriert (Viewport-Hälfte).
 * 
 * @param worldX X-Koordinate in der Spielwelt
 * @param worldY Y-Koordinate in der Spielwelt
 * @param game Aktueller Spielszustand (enthält Spieler-Position)
 * @param viewport Aktuelle Viewport-Dimensionen
 * @return sf::Vector2f Bildschirmposition für den gegebenen Weltkoordinaten
 */
inline sf::Vector2f WorldToScreen(float worldX, float worldY, const GameState& game, const Viewport& viewport)
{
    const float halfWidth = static_cast<float>(viewport.width) * 0.5f;
    const float halfHeight = static_cast<float>(viewport.height) * 0.5f;

    return sf::Vector2f((worldX - game.player.x) + halfWidth + game.cameraShake.offsetX,
                        (worldY - game.player.y) + halfHeight + game.cameraShake.offsetY);
}

/**
 * @brief Lädt und cached SFML-Texturen für effizientes Rendern.
 * 
 * Verwendet einen Thread-sicheren UnorderedMap zur Caching von geladenen Texturen.
 * Vermeidet redundante Dateiladungen und reduziert so GPU-Overhead bei vielen
 * Texturzugriffen. Die Texturen werden mit setSmooth(false) konfiguriert für
 * pixelgenaues Rendern (keine Naiv-Glättung).
 * 
 * @param path Pfad zur Texturdatei im Dateisystem
 * @return sf::Texture* Pointer zur geladenen Textur, nullptr bei Lade-/Cache-Fehler
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

/**
 * @brief Rendert den Hintergrund-Grund (Boden/Karte).
 * 
 * Lädt das Grundtextur vom angegebenen Pfad und rendert alle Kacheln, die im
 * Sichtbereich des Spielers liegen. Der Berechnungsbereich erstreckt sich um
 * 2 Kacheln über/unter dem Viewport-Rahmen hinaus für nahtloses Rendern an den Rändern.
 * 
 * Falls keine Textur angegeben ist oder das Laden fehlschlägt, wird ein einfaches
 * grünes Rechteck (sf::Color(72, 152, 76)) als Fallback verwendet.
 * 
 * @param target SFML-Ziel für das Rendern (z.B. Fenster, RenderTarget)
 * @param viewport Aktuelle Viewport-Dimensionen
 * @param game Aktueller Spielszustand (enthält den Pfad zur Grundtextur)
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
    // Berechnet den Kachelbereich um den Spieler herum, erweitert um 1 Kachel in jede Richtung
    const int firstTileX = static_cast<int>(std::floor((game.player.x - halfWidth) / tileSize)) - 1;
    const int lastTileX = static_cast<int>(std::ceil((game.player.x + halfWidth) / tileSize)) + 1;
    const int firstTileY = static_cast<int>(std::floor((game.player.y - halfHeight) / tileSize)) - 1;
    const int lastTileY = static_cast<int>(std::ceil((game.player.y + halfHeight) / tileSize)) + 1;
    
    sf::Texture* texture = GetCachedTexture(game.activeMap.defaultGroundPath);

    if (texture != nullptr)
    {
        sf::Sprite ground(*texture);
        const sf::Vector2u textureSize = texture->getSize();

        // Zentriert den Sprite-Origin und skaliert auf die gewünschte Kachelgröße
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

    // Fallback: einfaches grünes Rechteck wenn keine Textur verfügbar ist
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
 * @brief Rendert alle Map-Objekte (Wände, NPCs, Interaktive Elemente).
 * 
 * Iteriert über alle Objekte in der aktiven Karte und rendert jedes entweder als
 * Textur-Sprite oder als farbiges Rechteck-Fallback je nach Objekttyp:
 * - Wall: Grau (140, 140, 140)
 * - Npc: Beige (225, 180, 130)
 * - Untergrund: Grün (72, 152, 76)
 * - Sonstiges: Hellgrün (110, 138, 110)
 * 
 * @param target SFML-Ziel für das Rendern
 * @param viewport Aktuelle Viewport-Dimensionen
 * @param game Aktueller Spielszustand (enthält die Liste der Map-Objekte)
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
            if (object.hasTile)
            {
                sprite.setTextureRect(sf::IntRect({ object.tileX, object.tileY }, { object.tileWidth, object.tileHeight }));
                textureSize = { static_cast<unsigned>(object.tileWidth), static_cast<unsigned>(object.tileHeight) };
            }

            // Zentriert und skaliert das Sprite auf die Standard-Objektgröße (64x64)
            if (textureSize.x > 0u && textureSize.y > 0u)
            {
                sprite.setOrigin({ textureSize.x * 0.5f, textureSize.y * 0.5f });
                sprite.setScale({ objectSize / textureSize.x, objectSize / textureSize.y });
            }

            sprite.setPosition(screenPos);
            target.draw(sprite);
            continue;
        }

        // Fallback-Rechteck wenn keine Textur verfügbar ist
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
        else if (object.type == MapObjectType::Untergrund)
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
 * Funktioniert auch im Release-Build. Ohne Shader-Unterstützung wird ein einfaches
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
 * @brief Rendert Debug-Debugging-Overlays für Trigger-Zonen.
 * 
 * Zeigt halbtransparente Rechtecke an, die die Auslösebereiche (Trigger Zones) darstellen.
 * Der Rahmen ändert die Farbe basierend darauf, ob der Spieler sich innerhalb des Triggers befindet:
 * - Spieler drinnen: Hellgelb (255, 220, 120)
 * - Spieler draußen: Dunkelgelb (180, 180, 90)
 * 
 * Die Füllfarbe ist dunkel mit leichtem Gelbton (120, 120, 0, 35).
 * 
 * @param target SFML-Ziel für das Rendern
 * @param viewport Aktuelle Viewport-Dimensionen
 * @param game Aktueller Spielszustand (enthält Trigger-Liste und Spielerposition)
 */
inline void RenderTriggerDebug(sf::RenderTarget& target, const Viewport& viewport, const GameState& game)
{
    for (const TriggerZone& trigger : game.activeMap.triggers)
    {
        sf::RectangleShape area({ trigger.width, trigger.height });
        area.setPosition(WorldToScreen(trigger.x, trigger.y, game, viewport));
        area.setFillColor(sf::Color(120, 120, 0, 35)); // Dunkles Gelb mit 35% Opazität
        area.setOutlineThickness(1.0f);
        // Rahmenfarbe ändert sich je nach Spielerposition im Trigger
        area.setOutlineColor(trigger.isPlayerInside ? sf::Color(255, 220, 120) : sf::Color(180, 180, 90));

        target.draw(area);
    }
}
#endif

/**
 * @brief Haupt-Rendern-Funktion für den kompletten Spielszenen.
 * 
 * Rendert die gesamte Szene in der folgenden Reihenfolge:
 * 1. Hintergrund löschen (dunkelgrün: 22, 34, 26)
 * 2. Grid-Overlay für Debugging (nur Debug-Build)
 * 3. Grundtextur/Kacheln
 * 4. Map-Objekte (Wände, NPCs, etc.)
 * 5. Trigger-Zonen (nur Debug-Build)
 * 6. Spieler-Charakter (Textur oder Fallback-Rechteck)
 * 7. Magieeffekte und Wetter
 * 8. Optionale FPS-Anzeige und Debug-UI (Koordinaten und Ereignisse nur im Debug-Build)
 * 9. Kartenübergangs-Overlay (Fade-In/Fade-Out Effekt)
 * 
 * @param target SFML-Ziel für das Rendern
 * @param viewport Aktuelle Viewport-Dimensionen
 * @param game Aktueller Spielszustand
 * @param fps Aktuelle Frames pro Sekunde für die UI-Anzeige
 * @param showFps Ob die FPS-Anzeige sichtbar ist
 */
inline sf::FloatRect RenderGame(sf::RenderTarget& target, const Viewport& viewport, const GameState& game,
                                float fps, bool showFps)
{
    sf::FloatRect debugCoordinatesBounds{};

    // Hintergrund mit dunkelgrünem Farbton löschen (Wald/Regenwald-Atmosphäre)
    target.clear(sf::Color(22, 34, 26));

#ifdef _DEBUG
    // Grid für Debugging: 32x32 Pixel Kacheln, relativ zur Spielerposition
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

    // Spieler-Charakter rendern (animiertes Sprite oder Fallback-Rechteck)
    const float playerSize = static_cast<float>(game.player.size);
    struct PlayerAnimation
    {
        SpriteAnimation animation;
        sf::Texture* texture = nullptr;
    };
    const auto loadPlayerAnimations = []<std::size_t N>(const std::array<const char*, N>& paths)
    {
        std::array<PlayerAnimation, N> animations;
        for (std::size_t i = 0; i < paths.size(); ++i)
        {
            if (LoadAnimationFromFile(paths[i], animations[i].animation))
            {
                animations[i].texture = GetCachedTexture(animations[i].animation.texturePath);
                if (animations[i].texture == nullptr)
                {
                    std::fprintf(stderr, "Failed to load player sprite sheet: %s\n", animations[i].animation.texturePath.c_str());
                }
            }
        }
        return animations;
    };
    static const auto walkAnimations = loadPlayerAnimations(std::array<const char*, 8>{
        "Assets/Animations/Characters/player_walk_down.anim",
        "Assets/Animations/Characters/player_walk_up.anim",
        "Assets/Animations/Characters/player_walk_left.anim",
        "Assets/Animations/Characters/player_walk_right.anim",
        "Assets/Animations/Characters/player_walk_northwest.anim",
        "Assets/Animations/Characters/player_walk_northeast.anim",
        "Assets/Animations/Characters/player_walk_southwest.anim",
        "Assets/Animations/Characters/player_walk_southeast.anim"
    });
    static const auto idleAnimations = loadPlayerAnimations(std::array<const char*, 4>{
        "Assets/Animations/Characters/player_idle_down.anim",
        "Assets/Animations/Characters/player_idle_up.anim",
        "Assets/Animations/Characters/player_idle_left.anim",
        "Assets/Animations/Characters/player_idle_right.anim"
    });
    static constexpr std::array<PlayerDirection, 8> idleDirections = {
        PlayerDirection::Down, PlayerDirection::Up, PlayerDirection::Left, PlayerDirection::Right,
        PlayerDirection::Left, PlayerDirection::Right, PlayerDirection::Left, PlayerDirection::Right
    };
    const std::size_t direction = static_cast<std::size_t>(game.player.direction);
    const PlayerAnimation& selected = game.player.moving
        ? walkAnimations[direction]
        : idleAnimations[static_cast<std::size_t>(idleDirections[direction])];
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
        target.draw(playerSprite);
    }
    else
    {
        // Fallback: leuchtendes bläulich-weißes Rechteck wenn keine Textur verfügbar ist
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
    RenderWeather(target, game.weather, static_cast<float>(viewport.width), static_cast<float>(viewport.height));

    #ifdef _DEBUG
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

    // Kartenübergangs-Overlay (Fade-In oder Fade-Out Effekt)
    if (game.mapTransition.phase != MapTransitionPhase::None)
    {
        const float duration = game.mapTransition.duration > 0.0f ? game.mapTransition.duration : 0.001f;
        const float progress = std::clamp(game.mapTransition.timer / duration, 0.0f, 1.0f);
        // Fade-Out: alpha von 0 → 1 (wird dunkler), Fade-In: alpha von 1 → 0 (wird heller)
        const float alphaFactor = game.mapTransition.phase == MapTransitionPhase::FadeOut ? progress : (1.0f - progress);
        const auto alpha = static_cast<std::uint8_t>(std::clamp(alphaFactor * 255.0f, 0.0f, 255.0f));

        sf::RectangleShape fadeOverlay({ static_cast<float>(viewport.width), static_cast<float>(viewport.height) });
        fadeOverlay.setPosition({ 0.0f, 0.0f });
        fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha)); // Schwarzes Overlay mit variabler Opazität
        target.draw(fadeOverlay);
    }

    return debugCoordinatesBounds;
}
