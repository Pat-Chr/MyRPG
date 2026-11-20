// main.cpp : Entry Point MyRPG
//

#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"
#include "src/engine/MainMenu.h"
#include "src/engine/SaveGameStore.h"
#include "src/engine/SettingsMenu.h"
#ifdef MYRPG_MAP_EDITOR
#include "src/editor/MapEditor.h"
#endif

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

#ifdef _DEBUG
    if (!AllocConsole())
    {
        OutputDebugStringA("MyRPG: Failed to create the debug console.\n");
    }
    else
    {
        FILE* consoleStream = nullptr;
        if (freopen_s(&consoleStream, "CONOUT$", "w", stdout) != 0)
        {
            OutputDebugStringA("MyRPG: Failed to redirect stdout to the debug console.\n");
        }
        if (freopen_s(&consoleStream, "CONOUT$", "w", stderr) != 0)
        {
            OutputDebugStringA("MyRPG: Failed to redirect stderr to the debug console.\n");
        }
    }
#endif
#if defined(_DEBUG) || defined(MYRPG_MAP_EDITOR)
    wchar_t executablePath[32768];
    const DWORD executableLength = GetModuleFileNameW(nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
    if (executableLength == 0 || executableLength >= std::size(executablePath) ||
        !SetCurrentDirectoryW(std::filesystem::path(executablePath).parent_path().c_str()))
    {
        MessageBoxW(nullptr, L"Der Spielordner konnte nicht als Arbeitsverzeichnis eingerichtet werden.", L"MyRPG", MB_OK | MB_ICONERROR);
        return 1;
    }
#endif

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
    SaveGameStore saveGameStore;
    std::optional<SaveGameSlot::Id> activeSave;
    std::string activeSaveName;
    bool inMainMenu = true;
    PreloadMagicShaders();
    PreloadWeatherShaders();

    sf::Clock frameClock;
    sf::FloatRect debugCoordinatesBounds{};
    const auto showSaveGameError = [&](const std::string& error)
    {
        std::fprintf(stderr, "Savegame: %s\n", error.c_str());
        MessageBoxA(window.getNativeHandle(), error.c_str(), "MyRPG - Spielstand", MB_OK | MB_ICONERROR);
        frameClock.restart();
    };
    const auto getSavePosition = [](const GameState& state, const std::string& name)
    {
        return SaveGamePosition{ std::filesystem::path(state.activeMapPath).filename().string(),
            state.player.x, state.player.y, name };
    };
    const auto beginGame = [&](SaveGameSlot::Id id, std::string name)
    {
        activeSave = id;
        activeSaveName = std::move(name);
        mainMenu.Reset();
        inMainMenu = false;
        frameClock.restart();
    };
    const auto handleMainMenuAction = [&](MainMenu::Action action)
    {
        std::string error;
        if (action == MainMenu::Action::ExitGame)
        {
            window.close();
        }
        else if (action == MainMenu::Action::OpenNewGame)
        {
            mainMenu.OpenNewGame();
        }
        else if (action == MainMenu::Action::StartNewGame)
        {
            auto name = mainMenu.NewGameName();
            game = GameState{};
            InitializeGameState(game);
            SaveGameSlot::Id id{};
            if (IsSolidObjectBlockingPosition(game, game.player.x, game.player.y))
            {
                showSaveGameError("Die Startposition liegt in einem Hindernis und kann nicht gespeichert werden.");
                return;
            }
            if (!saveGameStore.Create(getSavePosition(game, name), id, error))
            {
                showSaveGameError(error);
                return;
            }
            beginGame(id, std::move(name));
        }
        else if (action == MainMenu::Action::OpenSaves)
        {
            std::vector<SaveGameSlot> slots;
            if (saveGameStore.List(slots, error)) mainMenu.OpenSaves(std::move(slots));
            else showSaveGameError(error);
        }
        else if (action == MainMenu::Action::LoadSelected || action == MainMenu::Action::DeleteSelected)
        {
            const SaveGameSlot* slot = mainMenu.SelectedSave();
            if (!slot) return;
            if (action == MainMenu::Action::LoadSelected)
            {
                SaveGamePosition position;
                if (!saveGameStore.Load(slot->id, position, error) ||
                    !TryRestorePlayerPosition(game, (std::filesystem::path(L"Assets\\Maps") / position.mapFile).generic_string(),
                        position.x, position.y, error))
                {
                    showSaveGameError(error);
                    return;
                }
                beginGame(slot->id, std::move(position.name));
                return;
            }

            const std::string label = slot->Label();
            const std::wstring question = L"„" + sf::String::fromUtf8(label.begin(), label.end()).toWideString() + L"“ endgültig löschen?";
            if (MessageBoxW(window.getNativeHandle(), question.c_str(), L"MyRPG - Spielstand löschen",
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
            if (saveGameStore.Delete(slot->id, error)) mainMenu.RemoveSelectedSave();
            else showSaveGameError(error);
        }
    };
    const auto saveCurrentGame = [&]() -> bool
    {
        if (IsSolidObjectBlockingPosition(game, game.player.x, game.player.y))
        {
            showSaveGameError("Die Spielerposition liegt in einem Hindernis und kann nicht gespeichert werden.");
            return false;
        }

        if (!activeSave)
        {
            showSaveGameError("Dem laufenden Spiel ist kein Spielstand zugeordnet.");
            return false;
        }
        std::string error;
        if (!saveGameStore.Save(*activeSave, getSavePosition(game, activeSaveName), error))
        {
            showSaveGameError(error);
            return false;
        }
        return true;
    };
    const auto confirmGameClose = [&]() -> bool
    {
        if (inMainMenu) return true;

        const int choice = MessageBoxW(window.getNativeHandle(), L"Möchten Sie vor dem Beenden speichern?",
            L"MyRPG - Spiel beenden", MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON3);
        frameClock.restart();
        if (choice == 0)
        {
            showSaveGameError("Die Speicherabfrage konnte nicht angezeigt werden.");
            return false;
        }
        return choice == IDNO || (choice == IDYES && saveCurrentGame());
    };
    const auto saveAndReturnToMainMenu = [&]()
    {
        if (!saveCurrentGame()) return;

        activeSave.reset();
        activeSaveName.clear();
        settingsMenu.Close();
        mainMenu.Reset();
        inMainMenu = true;
        frameClock.restart();
    };
    const auto handleSettingsAction = [&](SettingsMenu::Action action)
    {
        if (action == SettingsMenu::Action::SaveAndMainMenu)
        {
            saveAndReturnToMainMenu();
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
                if (confirmGameClose())
                {
                    window.close();
                    break;
                }
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                sf::View view;
                view.setSize({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
                view.setCenter({ static_cast<float>(baseResolution.x) * 0.5f, static_cast<float>(baseResolution.y) * 0.5f });
                view.setViewport(ComputeLetterboxViewport(resized->size, baseResolution));
                window.setView(view);
            }
            else if (const auto* text = event->getIf<sf::Event::TextEntered>())
            {
                if (inMainMenu && window.hasFocus()) mainMenu.HandleText(text->unicode);
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
#ifdef MYRPG_MAP_EDITOR
                else if (key->code == sf::Keyboard::Key::F2 && !IsMapTransitionActive(game)
#ifndef _DEBUG
                    && settingsMenu.GetSettings().mapEditorEnabled
#endif
                    )
                {
                    SetMovementState(game.input, false, false, false, false);
                    game.player.moving = false;
                    MapEditor editor;
                    editor.Run(window, game, confirmGameClose);
                    frameClock.restart();
                }
#endif
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
