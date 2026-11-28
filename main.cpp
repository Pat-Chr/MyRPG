// main.cpp : Entry Point MyRPG
//

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "src/engine/DialogueBox.h"
#include "src/engine/DialogueLoader.h"
#include "src/engine/GameEngine.h"
#include "src/engine/GameRenderer.h"
#include "src/engine/MainMenu.h"
#include "src/engine/SaveGameStore.h"
#include "src/engine/SettingsMenu.h"
#ifdef MYRPG_MAP_EDITOR
#include "src/editor/MapEditor.h"
#endif

// Calculates the normalized viewport while preserving the base aspect ratio.
// Unused space outside the viewport is filled with black letterbox bars.
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

    // Diagnostics go to a console only in Debug builds; Release remains without a console window.
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
    // Many asset and save paths are relative. Therefore, set the working directory independently
    // of the EXE, regardless of where the game was launched from.
    wchar_t executablePath[32768];
    const DWORD executableLength = GetModuleFileNameW(nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
    if (executableLength == 0 || executableLength >= std::size(executablePath) ||
        !SetCurrentDirectoryW(std::filesystem::path(executablePath).parent_path().c_str()))
    {
        MessageBoxW(nullptr, L"Der Spielordner konnte nicht als Arbeitsverzeichnis eingerichtet werden.", L"MyRPG", MB_OK | MB_ICONERROR);
        return 1;
    }

    // The game world and UI use a fixed logical resolution; the window may be resized
    // without changing the displayed aspect ratio.
    constexpr sf::Vector2u baseResolution{ 1280u, 960u };

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
    DialogueBox dialogue;
    SaveGameStore saveGameStore;
    std::optional<SaveGameSlot::Id> activeSave;
    std::string activeSaveName;
    double playtimeSeconds = 0.0;
    bool playtimeComplete = true;
    bool inMainMenu = true;
    bool pendingSavepoint = false;
    std::string savepointSpeaker;
    bool saveListOpen = false;
    bool nightFadeActive = false;
    float nightFadeTime = 0.0f;
    constexpr float nightFadeDuration = 1.2f;
    // Load shaders after creating the window and render context so they are ready for first use
    // and are not loaded isn the middle of a game effect.
    PreloadMagicShaders();
    PreloadWeatherShaders();
    PreloadDamageNumberRenderer();

    sf::Clock frameClock;
    sf::Clock playtimeClock;
    // Shared save-game error path: emit diagnostics, show a dialogue to the player, and
    // restart the clock so dialogue time is not applied as a large update step.
    const auto showSaveGameError = [&](const std::string& error)
    {
        std::fprintf(stderr, "Savegame: %s\n", error.c_str());
        MessageBoxA(window.getNativeHandle(), error.c_str(), "MyRPG - Spielstand", MB_OK | MB_ICONERROR);
        frameClock.restart();
        playtimeClock.restart();
    };
    const auto getSavePosition = [](const GameState& state, const std::string& name, double seconds, bool complete)
    {
        return SaveGamePosition{ std::filesystem::path(state.activeMapPath).filename().string(),
            state.player.x, state.player.y, name, seconds, complete };
    };
    // Activates an existing or loaded slot and resets the menu selection to a defined
    // state for a later return to the menu.
    const auto beginGame = [&](SaveGameSlot::Id id, std::string name, double seconds, bool complete)
    {
        activeSave = id;
        activeSaveName = std::move(name);
        playtimeSeconds = seconds;
        playtimeComplete = complete;
        mainMenu.Reset();
        inMainMenu = false;
        frameClock.restart();
        playtimeClock.restart();
    };
    const auto deleteSelectedSave = [&]()
    {
        const SaveGameSlot* slot = mainMenu.SelectedSave();
        if (!slot) return;
        const auto id = slot->id;
        const std::string label = slot->Label();
        const std::wstring question = L"„" + sf::String::fromUtf8(label.begin(), label.end()).toWideString() + L"“ endgültig löschen?";
        const int choice = MessageBoxW(window.getNativeHandle(), question.c_str(), L"MyRPG - Spielstand löschen",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        frameClock.restart();
        if (choice == 0) { showSaveGameError("Die Loeschbestaetigung konnte nicht angezeigt werden."); return; }
        if (choice != IDYES) return;
        std::string error;
        if (!saveGameStore.Delete(id, error)) { showSaveGameError(error); return; }
        if (activeSave && *activeSave == id) activeSave.reset();
        mainMenu.RemoveSelectedSave();
    };
    const auto loadSelectedSave = [&]() -> bool
    {
        const SaveGameSlot* slot = mainMenu.SelectedSave();
        if (!slot) return false;
        const auto id = slot->id;
        SaveGamePosition position;
        std::string error;
        if (!saveGameStore.Load(id, position, error) ||
            !TryRestorePlayerPosition(game, (std::filesystem::path(L"Assets\\Maps") / position.mapFile).generic_string(),
                position.x, position.y, error))
        {
            showSaveGameError(error);
            return false;
        }
        beginGame(id, std::move(position.name), position.playtimeSeconds.value_or(0.0), position.playtimeComplete);
        return true;
    };
    // Converts menu actions into game-state changes and delegates file operations to the store.
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
            if (!saveGameStore.Create(getSavePosition(game, name, 0.0, true), id, error))
            {
                showSaveGameError(error);
                return;
            }
            beginGame(id, std::move(name), 0.0, true);
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
            if (action == MainMenu::Action::LoadSelected) loadSelectedSave();
            else deleteSelectedSave();
        }
    };
    const auto handleSaveListAction = [&](MainMenu::Action action)
    {
        if (action == MainMenu::Action::CloseSaveList)
        {
            mainMenu.CloseSaveList();
            saveListOpen = false;
            frameClock.restart();
            playtimeClock.restart();
            return;
        }
        if (action == MainMenu::Action::DeleteSelected)
        {
            deleteSelectedSave();
            return;
        }
        if (action == MainMenu::Action::LoadSelected)
        {
            if (!mainMenu.SelectedSave()) return;
            const int choice = MessageBoxW(window.getNativeHandle(),
                L"Der aktuelle ungespeicherte Fortschritt geht verloren. Spielstand laden?",
                L"MyRPG - Spielstand laden", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            frameClock.restart();
            playtimeClock.restart();
            if (choice == 0) { showSaveGameError("Die Ladebestaetigung konnte nicht angezeigt werden."); return; }
            if (choice == IDYES && loadSelectedSave()) saveListOpen = false;
            return;
        }
        if (action != MainMenu::Action::SaveSelected && action != MainMenu::Action::CreateSave) return;

        SaveGameSlot::Id id{};
        if (action == MainMenu::Action::SaveSelected)
        {
            const SaveGameSlot* slot = mainMenu.SelectedSave();
            if (!slot) return;
            id = slot->id;
            const std::string label = slot->Label();
            const std::wstring question = L"„" + sf::String::fromUtf8(label.begin(), label.end()).toWideString() +
                L"“ überschreiben? Der bisherige Spielstand geht verloren.";
            const int choice = MessageBoxW(window.getNativeHandle(), question.c_str(), L"MyRPG - Spielstand überschreiben",
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            frameClock.restart();
            if (choice == 0) { showSaveGameError("Die Ueberschreibbestaetigung konnte nicht angezeigt werden."); return; }
            if (choice != IDYES) return;
        }
        if (IsSolidObjectBlockingPosition(game, game.player.x, game.player.y))
        {
            showSaveGameError("Die Spielerposition liegt in einem Hindernis und kann nicht gespeichert werden.");
            return;
        }
        std::string error;
        const auto name = action == MainMenu::Action::CreateSave ? mainMenu.NewGameName() : activeSaveName;
        const auto position = getSavePosition(game, name, playtimeSeconds, playtimeComplete);
        const bool saved = action == MainMenu::Action::CreateSave ? saveGameStore.Create(position, id, error) :
            saveGameStore.Save(id, position, error);
        if (!saved) { showSaveGameError(error); return; }
        activeSave = id;
        activeSaveName = name;
        mainMenu.CloseSaveList();
        saveListOpen = false;
        nightFadeTime = 0.0f;
        nightFadeActive = true;
        frameClock.restart();
        playtimeClock.restart();
    };
    const auto confirmDiscardProgress = [&](const wchar_t* title) -> bool
    {
        const int choice = MessageBoxW(window.getNativeHandle(),
            L"Nicht gespeicherter Fortschritt geht verloren. Möchtest du fortfahren?",
            title, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        frameClock.restart();
        playtimeClock.restart();
        if (choice == 0)
        {
            showSaveGameError("Die Warnung konnte nicht angezeigt werden.");
            return false;
        }
        return choice == IDYES;
    };
    const auto confirmGameClose = [&]() -> bool
    {
        return inMainMenu || confirmDiscardProgress(L"MyRPG - Spiel beenden");
    };
    const auto returnToMainMenu = [&]()
    {
        if (!confirmDiscardProgress(L"MyRPG - Hauptmenü")) return;

        activeSave.reset();
        activeSaveName.clear();
        settingsMenu.Close();
        mainMenu.Reset();
        inMainMenu = true;
        frameClock.restart();
    };
    // Settings are persisted independently of game progress.
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

    // SFML events (key presses, text, and clicks) are processed first;
    // Held movement keys are queried separately per frame below.
    while (window.isOpen())
    {
        const auto elapsedPlayMicroseconds = playtimeClock.restart().asMicroseconds();
        if (!inMainMenu && !settingsMenu.IsOpen() && !saveListOpen && !dialogue.IsOpen() && !nightFadeActive)
            playtimeSeconds += static_cast<double>(elapsedPlayMicroseconds) / 1000000.0;
        bool dialogueClosedThisFrame = false;
        const auto finishDialogue = [&](bool completed)
        {
            if (dialogue.IsOpen()) return;
            if (pendingSavepoint && completed)
            {
                pendingSavepoint = false;
                if (dialogue.OpenSaveChoice(savepointSpeaker)) return;
                showSaveGameError("Der Speicherpunkt-Dialog konnte nicht geoeffnet werden.");
            }
            pendingSavepoint = false;
            EndPlayerMonologue(game);
            dialogueClosedThisFrame = true;
            frameClock.restart();
        };
        const auto finishSaveChoice = [&](const std::optional<bool>& choice)
        {
            if (!choice) return;
            finishDialogue(false);
            if (*choice)
            {
                std::vector<SaveGameSlot> slots;
                std::string error;
                if (!saveGameStore.List(slots, error)) { showSaveGameError(error); return; }
                mainMenu.OpenSaves(std::move(slots), true, activeSave);
                saveListOpen = true;
            }
        };
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
            else if (nightFadeActive)
            {
                continue;
            }
            else if (const auto* text = event->getIf<sf::Event::TextEntered>())
            {
                if ((inMainMenu || saveListOpen) && window.hasFocus()) mainMenu.HandleText(text->unicode);
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                // Do not execute game or menu actions when the window is inactive.
                if (!window.hasFocus())
                {
                    continue;
                }

                if (inMainMenu)
                {
                    handleMainMenuAction(mainMenu.HandleKey(key->code));
                }
                else if (saveListOpen)
                {
                    handleSaveListAction(mainMenu.HandleKey(key->code));
                    if (!saveListOpen) dialogueClosedThisFrame = true;
                }
                else if (dialogue.IsOpen())
                {
                    if (dialogue.IsChoice()) finishSaveChoice(dialogue.HandleChoiceKey(key->code));
                    else
                    {
                        dialogue.HandleKey(key->code);
                        finishDialogue(key->code != sf::Keyboard::Key::Escape);
                    }
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
                        playtimeClock.restart();
                    }
                }
                else if (settingsMenu.IsOpen())
                {
                    handleSettingsAction(settingsMenu.HandleKey(key->code, window));
                }
#ifdef MYRPG_MAP_EDITOR
                // The editor is available directly in Debug; in Release, saved authorization
                // must also be active. It remains closed during a map transition.
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
                    playtimeClock.restart();
                }
#endif
                else if (key->code == sf::Keyboard::Key::M && !IsMapTransitionActive(game))
                {
                    StartMagicEffect(game.magicEffects, "sparkle", game.player.x, game.player.y);
                }
                else if (key->code == sf::Keyboard::Key::Space && !dialogueClosedThisFrame)
                {
                    StartPlayerAttack(game);
                }
                else if (key->code == sf::Keyboard::Key::E)
                {
                    // First, the game logic determines whether an NPC is in front of the player;
                    // only then are that NPC's dialogue data loaded and movement stopped.
                    MapObject* npc = InteractWithNpc(game);
                    if (npc)
                    {
#ifdef _DEBUG
                        std::printf("[Dialogue] Speaking to NPC '%s'.\n",
                                    npc->npcName.empty() ? "<unnamed>" : npc->npcName.c_str());
#endif
                        std::vector<std::string> greeting;
                        std::string error;
                        if (!LoadNpcGreeting(npc->npcName, greeting, error))
                        {
                            std::fprintf(stderr, "NPC dialogue: %s\n", error.c_str());
                            const std::wstring message = L"Dialogue fuer „" +
                                sf::String::fromUtf8(npc->npcName.begin(), npc->npcName.end()).toWideString() +
                                L"“: " + sf::String::fromUtf8(error.begin(), error.end()).toWideString();
                            MessageBoxW(window.getNativeHandle(), message.c_str(), L"MyRPG - Dialogue", MB_OK | MB_ICONERROR);
                            frameClock.restart();
                            playtimeClock.restart();
                        }
                        else if (dialogue.Open(greeting, npc->npcName))
                        {
#ifdef _DEBUG
                            std::printf("[Dialogue] Opened greeting for NPC '%s' (%zu paragraph(s)).\n",
                                        npc->npcName.empty() ? "<unnamed>" : npc->npcName.c_str(), greeting.size());
#endif
                            pendingSavepoint = npc->savepoint;
                            savepointSpeaker = npc->npcName;
                            SetMovementState(game.input, false, false, false, false);
                            game.player.moving = false;
                        }
                        else
                        {
                            MessageBoxW(window.getNativeHandle(), L"Die Schrift fuer den Dialogue konnte nicht geladen werden.",
                                        L"MyRPG", MB_OK | MB_ICONERROR);
                            frameClock.restart();
                            playtimeClock.restart();
                        }
                    }
                }
            }
            else if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButton->button == sf::Mouse::Button::Left && window.hasFocus())
                {
                    // Converts window pixels to logical game coordinates so clicks
                    // are evaluated correctly with scaling and letterbox borders.
                    const sf::Vector2f position = window.mapPixelToCoords(mouseButton->position);
                    if (inMainMenu)
                    {
                        handleMainMenuAction(mainMenu.HandleClick(position, baseResolution));
                    }
                    else if (saveListOpen)
                    {
                        handleSaveListAction(mainMenu.HandleClick(position, baseResolution));
                        if (!saveListOpen) dialogueClosedThisFrame = true;
                    }
                    else if (dialogue.IsOpen())
                    {
                        if (dialogue.IsChoice()) finishSaveChoice(dialogue.HandleChoiceClick(position, baseResolution));
                        else
                        {
                            dialogue.Advance();
                            finishDialogue(true);
                        }
                    }
                    else if (settingsMenu.IsOpen())
                    {
                        handleSettingsAction(settingsMenu.HandleClick(position, baseResolution, window));
                    }
                }
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        // Movement is possible only in active gameplay with menus and dialogues closed.
        // When focus is lost, all directions become false and the character stops immediately.
        const bool hasFocus = window.hasFocus() && !inMainMenu && !settingsMenu.IsOpen() && !saveListOpen &&
            !dialogue.IsOpen() && !dialogueClosedThisFrame && !nightFadeActive;
        SetMovementState(
            game.input,
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)));
        game.input.sprint = hasFocus &&
            (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift));

        // Delta time makes the simulation independent of frame rate. Since the same clock
        // is restarted after dialogues and error messages, no time jumps occur afterward.
        const float deltaTime = frameClock.restart().asSeconds();
        const float fps = deltaTime > 0.0f ? (1.0f / deltaTime) : 0.0f;
        if (nightFadeActive && window.hasFocus())
        {
            nightFadeTime += (std::min)(deltaTime, 0.05f);
            if (nightFadeTime >= nightFadeDuration) nightFadeActive = false;
        }
        if (!inMainMenu && !settingsMenu.IsOpen() && !saveListOpen && !dialogue.IsOpen() && !dialogueClosedThisFrame && !nightFadeActive)
        {
            UpdateGame(game, deltaTime);
            if (!game.pendingMonologue.empty())
            {
                const std::string name = std::exchange(game.pendingMonologue, {});
                std::vector<std::string> paragraphs;
                std::string error;
                if (!LoadTriggerMonologue(name, paragraphs, error))
                {
                    std::fprintf(stderr, "Trigger dialogue '%s': %s\n", name.c_str(), error.c_str());
                    const std::wstring message = L"Dialogue fuer Trigger " +
                        sf::String::fromUtf8(name.begin(), name.end()).toWideString() + L": " +
                        sf::String::fromUtf8(error.begin(), error.end()).toWideString();
                    MessageBoxW(window.getNativeHandle(), message.c_str(), L"MyRPG - Dialogue", MB_OK | MB_ICONERROR);
                    frameClock.restart();
                    playtimeClock.restart();
                }
                else if (dialogue.Open(paragraphs, ""))
                {
                    BeginPlayerMonologue(game);
                }
                else
                {
                    MessageBoxW(window.getNativeHandle(), L"Die Schrift fuer den Dialogue konnte nicht geladen werden.",
                                L"MyRPG", MB_OK | MB_ICONERROR);
                    frameClock.restart();
                    playtimeClock.restart();
                }
            }
        }

        const Viewport viewport{
            static_cast<int>(baseResolution.x),
            static_cast<int>(baseResolution.y)
        };

        const sf::Vector2u windowSize = window.getSize();

        // The logical view remains constant; only its normalized window area changes.
        // This keeps the game world, mouse coordinates, and UI at the same base resolution.
        sf::View letterboxView;
        letterboxView.setSize({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
        letterboxView.setCenter({ static_cast<float>(baseResolution.x) * 0.5f, static_cast<float>(baseResolution.y) * 0.5f });
        letterboxView.setViewport(ComputeLetterboxViewport(windowSize, baseResolution));
        window.setView(letterboxView);

        // Each frame renders either the main menu or the game world. Menus and dialogues
        // overlay the game image; simulation was paused above while they were active.
        if (inMainMenu)
        {
            window.clear(sf::Color::Black);
            mainMenu.Draw(window, baseResolution);
        }
        else
        {
            RenderGame(window, viewport, game, fps, settingsMenu.GetSettings().showFps);
            if (saveListOpen)
            {
                sf::RectangleShape overlay({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
                overlay.setFillColor(sf::Color(0, 0, 0, 170));
                window.draw(overlay);
                mainMenu.DrawSaveList(window, baseResolution);
            }
            else if (settingsMenu.IsOpen())
            {
                settingsMenu.Draw(window, baseResolution);
            }
            else if (dialogue.IsOpen())
            {
                dialogue.Draw(window, baseResolution);
            }
            if (nightFadeActive)
            {
                const float progress = nightFadeTime / nightFadeDuration;
                const float opacity = progress < 0.5f ? progress * 2.0f : (1.0f - progress) * 2.0f;
                sf::RectangleShape darkness({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
                darkness.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>((std::clamp)(opacity, 0.0f, 1.0f) * 255.0f)));
                window.draw(darkness);
            }
        }
        window.display();
    }

    return 0;
}
