// main.cpp : Entry Point MyRPG
//

#include <Windows.h>

#include <optional>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"
#include "src/engine/SettingsMenu.h"

sf::FloatRect ComputeLetterboxViewport(sf::Vector2u windowSize, sf::Vector2u baseResolution)
{
    if (windowSize.x == 0u || windowSize.y == 0u || baseResolution.x == 0u || baseResolution.y == 0u)
    {
        return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
    }

    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float baseRatio = static_cast<float>(baseResolution.x) / static_cast<float>(baseResolution.y);

    if (windowRatio > baseRatio)
    {
        const float width = baseRatio / windowRatio;
        const float left = (1.0f - width) * 0.5f;
        return { { left, 0.0f }, { width, 1.0f } };
    }

    if (windowRatio < baseRatio)
    {
        const float height = windowRatio / baseRatio;
        const float top = (1.0f - height) * 0.5f;
        return { { 0.0f, top }, { 1.0f, height } };
    }

    return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    constexpr sf::Vector2u baseResolution{ 1024u, 768u };

    sf::RenderWindow window(sf::VideoMode(baseResolution), "MyRPG");
    window.setKeyRepeatEnabled(false);

    SettingsStore settingsStore;
    GameSettings settings;
    std::string settingsError;
    if (!settingsStore.Load(settings, settingsError))
    {
        std::fprintf(stderr, "Failed to load settings: %s\n", settingsError.c_str());
        const std::string message = settingsError + "\nStandardeinstellungen werden verwendet. Die Datei wird nicht ueberschrieben.";
        MessageBoxA(window.getNativeHandle(), message.c_str(), "MyRPG - Einstellungen", MB_OK | MB_ICONERROR);
    }
    SettingsMenu settingsMenu;
    settingsMenu.ApplySettings(settings, window);

    GameState game{};
    InitializeGameState(game);
    PreloadMagicShaders();

    sf::Clock frameClock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (!window.hasFocus())
                {
                    continue;
                }

                if (key->code == sf::Keyboard::Key::Escape)
                {
                    if (settingsMenu.IsOpen())
                    {
                        settingsMenu.Close();
                    }
                    else if (settingsMenu.Open())
                    {
                        SetMovementState(game.input, false, false, false, false);
                        game.player.moving = false;
                    }
                    else
                    {
                        MessageBoxW(window.getNativeHandle(), L"Die Schrift fuer das Einstellungsmenue konnte nicht geladen werden.",
                                    L"MyRPG", MB_OK | MB_ICONERROR);
                        frameClock.restart();
                    }
                }
                else if (settingsMenu.IsOpen())
                {
                    if (settingsMenu.HandleKey(key->code, window) &&
                        !settingsStore.Save(settingsMenu.GetSettings(), settingsError))
                    {
                        std::fprintf(stderr, "Failed to save settings: %s\n", settingsError.c_str());
                        MessageBoxA(window.getNativeHandle(), settingsError.c_str(), "MyRPG - Einstellungen", MB_OK | MB_ICONERROR);
                        frameClock.restart();
                    }
                }
                else if (key->code == sf::Keyboard::Key::M && !IsMapTransitionActive(game))
                {
                    StartMagicEffect(game.magicEffects, "sparkle", game.player.x, game.player.y);
                }
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        const bool hasFocus = window.hasFocus() && !settingsMenu.IsOpen();
        SetMovementState(
            game.input,
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)));

        const float deltaTime = frameClock.restart().asSeconds();
        const float fps = deltaTime > 0.0f ? (1.0f / deltaTime) : 0.0f;
        if (!settingsMenu.IsOpen())
        {
            UpdateGame(game, deltaTime);
        }

        const Viewport viewport{
            static_cast<int>(baseResolution.x),
            static_cast<int>(baseResolution.y)
        };

        const sf::Vector2u windowSize = window.getSize();

        sf::View letterboxView;
        letterboxView.setSize({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
        letterboxView.setCenter({ static_cast<float>(baseResolution.x) * 0.5f, static_cast<float>(baseResolution.y) * 0.5f });
        letterboxView.setViewport(ComputeLetterboxViewport(windowSize, baseResolution));
        window.setView(letterboxView);

        RenderGame(window, viewport, game, fps, settingsMenu.GetSettings().showFps);
        if (settingsMenu.IsOpen())
        {
            settingsMenu.Draw(window, baseResolution);
        }
        window.display();
    }

    return 0;
}
