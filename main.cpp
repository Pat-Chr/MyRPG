// main.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include <Windows.h>

#include <optional>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"

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
    window.setFramerateLimit(60);

    GameState game{};
    InitializeGameState(game);

    sf::Clock frameClock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        SetMovementState(
            game.input,
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left),
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right));

        const float deltaTime = frameClock.restart().asSeconds();
        const float fps = deltaTime > 0.0f ? (1.0f / deltaTime) : 0.0f;
        UpdateGame(game, deltaTime);

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

        RenderGame(window, viewport, game, fps);
        window.display();
    }

    return 0;
}
