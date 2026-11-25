// main.cpp : Entry Point MyRPG
//

#include <Windows.h>

#include <cstdio>
#include <cstring>
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

// Berechnet den normierten Viewport so, dass das Basis-Seitenverhältnis erhalten bleibt.
// Der ungenutzte Rand wird durch die schwarzen Balken außerhalb des Viewports gefüllt.
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

// Übergibt Unicode-Text über die Win32-Zwischenablage. Nach erfolgreichem SetClipboardData
// gehört der Speicher dem Betriebssystem; bei Fehlern wird er hier wieder freigegeben.
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

    // Diagnoseausgaben gehen nur im Debug-Build an eine Konsole; Release bleibt ohne Konsolenfenster.
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
    // Viele Asset- und Speicherpfade sind relativ. Deshalb wird das Arbeitsverzeichnis unabhängig
    // davon, von wo aus das Spiel gestartet wurde, auf den Ordner der EXE gesetzt.
    wchar_t executablePath[32768];
    const DWORD executableLength = GetModuleFileNameW(nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
    if (executableLength == 0 || executableLength >= std::size(executablePath) ||
        !SetCurrentDirectoryW(std::filesystem::path(executablePath).parent_path().c_str()))
    {
        MessageBoxW(nullptr, L"Der Spielordner konnte nicht als Arbeitsverzeichnis eingerichtet werden.", L"MyRPG", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Spielwelt und UI arbeiten in einer festen logischen Auflösung; das Fenster darf später
    // anders groß sein, ohne dass sich das Seitenverhältnis der Darstellung ändert.
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
    DialogueBox dialogue;
    SaveGameStore saveGameStore;
    std::optional<SaveGameSlot::Id> activeSave;
    std::string activeSaveName;
    bool inMainMenu = true;
    // Shader nach dem Erzeugen des Fensters/Render-Kontexts laden, damit sie beim ersten Einsatz
    // bereits verfügbar sind und nicht mitten in einem Spieleffekt geladen werden müssen.
    PreloadMagicShaders();
    PreloadWeatherShaders();

    sf::Clock frameClock;
    sf::FloatRect debugCoordinatesBounds{};
    // Gemeinsamer Fehlerpfad für Spielstände: Ausgabe für Diagnose, Dialogue für den Spieler und
    // Neustart der Uhr, damit die Dialoguezeit nicht als großer Update-Schritt ins Spiel gelangt.
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
    // Aktiviert einen bereits angelegten oder geladenen Slot und lässt die Menüauswahl für
    // einen späteren Rückweg in einen definierten Ausgangszustand zurückkehren.
    const auto beginGame = [&](SaveGameSlot::Id id, std::string name)
    {
        activeSave = id;
        activeSaveName = std::move(name);
        mainMenu.Reset();
        inMainMenu = false;
        frameClock.restart();
    };
    // Übersetzt Menü-Aktionen in Spielzustandswechsel und delegiert Dateioperationen an den Store.
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
    // Gemeinsamer Speicherweg für Menü-Rückkehr und Beenden. Ungültige Positionen oder ein
    // fehlgeschlagener Schreibvorgang verhindern den jeweiligen Zustandswechsel.
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
    // Liefert true nur, wenn das Schließen erlaubt ist: im Spiel kann der Spieler speichern,
    // ohne Speichern beenden oder abbrechen; im Hauptmenü ist keine Rückfrage nötig.
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
    // Änderungen werden sofort dauerhaft gespeichert; der Rückweg ins Hauptmenü erfolgt erst,
    // wenn auch das Speichern des aktiven Spielstands erfolgreich war.
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

    // SFML-Ereignisse (einmalige Tastendrücke, Text und Klicks) werden zuerst abgearbeitet;
    // gehaltene Bewegungstasten werden weiter unten pro Frame separat abgefragt.
    while (window.isOpen())
    {
        bool dialogueClosedThisFrame = false;
        const auto finishDialogue = [&]()
        {
            if (dialogue.IsOpen()) return;
            EndPlayerMonologue(game);
            dialogueClosedThisFrame = true;
            frameClock.restart();
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
            else if (const auto* text = event->getIf<sf::Event::TextEntered>())
            {
                if (inMainMenu && window.hasFocus()) mainMenu.HandleText(text->unicode);
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                // Keine Spiel- oder Menüaktion ausführen, wenn das Fenster nicht aktiv ist.
                if (!window.hasFocus())
                {
                    continue;
                }

                if (inMainMenu)
                {
                    handleMainMenuAction(mainMenu.HandleKey(key->code));
                }
                else if (dialogue.IsOpen())
                {
                    dialogue.HandleKey(key->code);
                    finishDialogue();
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
                // Der Editor ist im Debug direkt verfügbar; im Release muss zusätzlich die
                // gespeicherte Freigabe aktiv sein. Während eines Kartenwechsels bleibt er zu.
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
                else if (key->code == sf::Keyboard::Key::E)
                {
                    // Erst die Spiellogik ermittelt, ob ein ansprechbarer NPC vor dem Spieler ist;
                    // nur dann werden dessen Dialoguedaten geladen und die Bewegung angehalten.
                    MapObject* npc = InteractWithNpc(game);
                    if (npc)
                    {
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
                        }
                        else if (dialogue.Open(greeting, npc->npcName))
                        {
                            SetMovementState(game.input, false, false, false, false);
                            game.player.moving = false;
                        }
                        else
                        {
                            MessageBoxW(window.getNativeHandle(), L"Die Schrift fuer den Dialogue konnte nicht geladen werden.",
                                        L"MyRPG", MB_OK | MB_ICONERROR);
                            frameClock.restart();
                        }
                    }
                }
            }
            else if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButton->button == sf::Mouse::Button::Left && window.hasFocus())
                {
                    // Wandelt Fensterpixel in die logischen Spielkoordinaten um, damit Klicks
                    // auch bei Skalierung und Letterbox-Rändern korrekt ausgewertet werden.
                    const sf::Vector2f position = window.mapPixelToCoords(mouseButton->position);
                    if (inMainMenu)
                    {
                        handleMainMenuAction(mainMenu.HandleClick(position, baseResolution));
                    }
                    else if (dialogue.IsOpen())
                    {
                        dialogue.Advance();
                        finishDialogue();
                    }
                    else if (settingsMenu.IsOpen())
                    {
                        handleSettingsAction(settingsMenu.HandleClick(position, baseResolution, window));
                    }
#ifdef _DEBUG
                    else if (!dialogue.IsOpen() && debugCoordinatesBounds.contains(position))
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

        // Bewegung ist nur im aktiven Spiel und bei geschlossenem Menü/Dialogue möglich.
        // Bei Fokusverlust werden alle Richtungen false, wodurch die Figur sofort anhält.
        const bool hasFocus = window.hasFocus() && !inMainMenu && !settingsMenu.IsOpen() &&
            !dialogue.IsOpen() && !dialogueClosedThisFrame;
        SetMovementState(
            game.input,
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)),
            hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)));

        // Delta-Time macht die Spielsimulation unabhängig von der Bildrate. Da dieselbe Uhr
        // auch durch Dialogues/Fehlermeldungen neu gestartet wird, entstehen danach keine Zeitsprünge.
        const float deltaTime = frameClock.restart().asSeconds();
        const float fps = deltaTime > 0.0f ? (1.0f / deltaTime) : 0.0f;
        if (!inMainMenu && !settingsMenu.IsOpen() && !dialogue.IsOpen() && !dialogueClosedThisFrame)
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
                }
            }
        }

        const Viewport viewport{
            static_cast<int>(baseResolution.x),
            static_cast<int>(baseResolution.y)
        };

        const sf::Vector2u windowSize = window.getSize();

        // Die logische View bleibt konstant; nur ihr normierter Ausschnitt im Fenster ändert
        // sich. Dadurch behalten Spielwelt, Mauskoordinaten und UI dieselbe Basisauflösung.
        sf::View letterboxView;
        letterboxView.setSize({ static_cast<float>(baseResolution.x), static_cast<float>(baseResolution.y) });
        letterboxView.setCenter({ static_cast<float>(baseResolution.x) * 0.5f, static_cast<float>(baseResolution.y) * 0.5f });
        letterboxView.setViewport(ComputeLetterboxViewport(windowSize, baseResolution));
        window.setView(letterboxView);

        // Pro Frame wird entweder das Hauptmenü oder die Spielwelt gezeichnet. Menüs und Dialogues
        // liegen über dem Spielbild; die Simulation selbst wurde oben währenddessen angehalten.
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
            else if (dialogue.IsOpen())
            {
                dialogue.Draw(window, baseResolution);
            }
        }
        window.display();
    }

    return 0;
}
