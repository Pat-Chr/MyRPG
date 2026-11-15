// main.cpp : Entry Point MyRPG
//

#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <optional>
#include <string>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"
#include "src/engine/MainMenu.h"
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

bool CopyTextToClipboard(HWND owner, const std::wstring& text)
{
    const SIZE_T byteCount = static_cast<SIZE_T>((text.size() + 1) * sizeof(wchar_t));
    HGLOBAL clipboardMemory = GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if (clipboardMemory == nullptr)
    {
        std::fprintf(stderr, "Failed to allocate clipboard memory (Win32 error %lu).\n", GetLastError());
        return false;
    }

    void* memory = GlobalLock(clipboardMemory);
    if (memory == nullptr)
    {
        const DWORD error = GetLastError();
        GlobalFree(clipboardMemory);
        std::fprintf(stderr, "Failed to lock clipboard memory (Win32 error %lu).\n", error);
        return false;
    }

    std::memcpy(memory, text.c_str(), byteCount);
    GlobalUnlock(clipboardMemory);

    if (!OpenClipboard(owner))
    {
        const DWORD error = GetLastError();
        GlobalFree(clipboardMemory);
        std::fprintf(stderr, "Failed to open the clipboard (Win32 error %lu).\n", error);
        return false;
    }

    if (!EmptyClipboard())
    {
        const DWORD error = GetLastError();
        CloseClipboard();
        GlobalFree(clipboardMemory);
        std::fprintf(stderr, "Failed to clear the clipboard (Win32 error %lu).\n", error);
        return false;
    }

    if (SetClipboardData(CF_UNICODETEXT, clipboardMemory) == nullptr)
    {
        const DWORD error = GetLastError();
        CloseClipboard();
        GlobalFree(clipboardMemory);
        std::fprintf(stderr, "Failed to set clipboard text (Win32 error %lu).\n", error);
        return false;
    }

    CloseClipboard();
    return true;
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

    MainMenu mainMenu;
    if (!mainMenu.Load())
    {
        MessageBoxW(window.getNativeHandle(), L"Der Titelbildschirm oder die Menueschrift konnte nicht geladen werden.",
                    L"MyRPG", MB_OK | MB_ICONERROR);
        return 1;
    }

    GameState game{};
    bool inMainMenu = true;
    PreloadMagicShaders();
    PreloadWeatherShaders();

    sf::Clock frameClock;
    sf::FloatRect debugCoordinatesBounds{};
    const auto handleMainMenuAction = [&](MainMenu::Action action)
    {
        if (action == MainMenu::Action::StartGame)
        {
            game = GameState{};
            InitializeGameState(game);
            inMainMenu = false;
            frameClock.restart();
        }
        else if (action == MainMenu::Action::ExitGame)
        {
            window.close();
        }
    };
    const auto returnToMainMenu = [&]()
    {
        settingsMenu.Close();
        mainMenu.Reset();
        inMainMenu = true;
        frameClock.restart();
    };
    const auto handleSettingsAction = [&](SettingsMenu::Action action)
    {
        if (action == SettingsMenu::Action::MainMenu)
        {
            returnToMainMenu();
        }
        else if (action == SettingsMenu::Action::SettingsChanged &&
                 !settingsStore.Save(settingsMenu.GetSettings(), settingsError))
        {
            std::fprintf(stderr, "Failed to save settings: %s\n", settingsError.c_str());
            MessageBoxA(window.getNativeHandle(), settingsError.c_str(), "MyRPG - Einstellungen", MB_OK | MB_ICONERROR);
            frameClock.restart();
        }
    };

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                sf::View view;
                view.setSize({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
                view.setCenter({ static_cast<float>(baseResolution.x) * 0.5f, static_cast<float>(baseResolution.y) * 0.5f });
                view.setViewport(ComputeLetterboxViewport(resized->size, baseResolution));
                window.setView(view);
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (!window.hasFocus())
                {
                    continue;
                }

                if (inMainMenu)
                {
                    handleMainMenuAction(mainMenu.HandleKey(key->code));
                }
                else if (key->code == sf::Keyboard::Key::Escape)
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
                    handleSettingsAction(settingsMenu.HandleKey(key->code, window));
                }
                else if (key->code == sf::Keyboard::Key::M && !IsMapTransitionActive(game))
                {
                    StartMagicEffect(game.magicEffects, "sparkle", game.player.x, game.player.y);
                }
            }
            else if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButton->button == sf::Mouse::Button::Left && window.hasFocus())
                {
                    const sf::Vector2f position = window.mapPixelToCoords(mouseButton->position);
                    if (inMainMenu)
                    {
                        handleMainMenuAction(mainMenu.HandleClick(position, baseResolution));
                    }
                    else if (settingsMenu.IsOpen())
                    {
                        handleSettingsAction(settingsMenu.HandleClick(position, baseResolution, window));
                    }
#ifdef _DEBUG
                    else if (debugCoordinatesBounds.contains(position))
                    {
                        char coordinates[64];
                        std::snprintf(coordinates, sizeof(coordinates), "X: %.1f  Y: %.1f", game.player.x, game.player.y);
                        const std::wstring clipboardText(coordinates, coordinates + std::strlen(coordinates));
                        CopyTextToClipboard(window.getNativeHandle(), clipboardText);
                    }
#endif
                }
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        const bool hasFocus = window.hasFocus() && !inMainMenu && !settingsMenu.IsOpen();
        SetMovementState(
            game.input,
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)));

        const float deltaTime = frameClock.restart().asSeconds();
        const float fps = deltaTime > 0.0f ? (1.0f / deltaTime) : 0.0f;
        if (!inMainMenu && !settingsMenu.IsOpen())
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

        if (inMainMenu)
        {
            window.clear(sf::Color::Black);
            mainMenu.Draw(window, baseResolution);
        }
        else
        {
            debugCoordinatesBounds = RenderGame(window, viewport, game, fps, settingsMenu.GetSettings().showFps);
            if (settingsMenu.IsOpen())
            {
                settingsMenu.Draw(window, baseResolution);
            }
        }
        window.display();
    }

    return 0;
}
