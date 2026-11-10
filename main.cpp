// main.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include <Windows.h>

#include <optional>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    sf::RenderWindow window(sf::VideoMode({ 960u, 540u }), "MyRPG");
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

        const sf::Vector2u windowSize = window.getSize();
        const Viewport viewport{
            static_cast<int>(windowSize.x),
            static_cast<int>(windowSize.y)
        };

        RenderGame(window, viewport, game, fps);
        window.display();
    }

    return 0;
}
