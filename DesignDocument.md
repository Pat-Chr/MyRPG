# MyRPG - Spielmechanik Dokumentation

## Überblick

Dieses Dokument beschreibt die Funktionsweise des MyRPG Spiels. Es dient als Referenz für zukünftige Entwicklungen und Wartung.

---

## Architektur

Das Spiel besteht aus drei Hauptkomponenten:

1. **GameState** - Der globale Spielzustand
2. **GameEngine** - Logik-Update-Funktionen
3. **GameRenderer** - Rendering-Funktionen (noch nicht implementiert)

---

## Datenstrukturen

### GameState

Der globale Spielzustand, der in `main.cpp` als globale Variable `g_game` deklariert ist:

```cpp
struct GameState
{
    Player player;      // Der Spieler
    InputState input;   // Tastatur-Eingabezustand
    ULONGLONG lastTick; // Zeitstempel für Delta-Zeit-Berechnung
};
```

### Player

Spieler-Struktur mit Position und Eigenschaften:

```cpp
struct Player
{
    float x = 0.0f;        // X-Koordinate (Zentriert im Fenster)
    float y = 0.0f;        // Y-Koordinate (Zentriert im Fenster)
    float speed = 220.0f;  // Bewegungsgeschwindigkeit in Pixel/Sekunde
    int size = 24;         // Spielergröße in Pixel (für Kollisionserkennung)
};
```

### InputState

Verfolgt den Tastaturzustand für Bewegungssteuerung:

```cpp
struct InputState
{
    bool up = false;       // W oder Pfeil hoch
    bool down = false;     // S oder Pfeil runter
    bool left = false;     // A oder Pfeil links
    right = false;         // D oder Pfeil rechts
};
```

---

## Spielablauf (Game Loop)

Der Hauptschleifen-Code befindet sich in `main.cpp`, Funktion `WndProc`:

### 1. WM_TIMER Nachricht (ca. alle 16ms)

Jede Frame wird der folgende Zyklus ausgeführt:

```cpp
case WM_TIMER:
    if (wParam == GAME_TIMER_ID)
    {
        const ULONGLONG now = GetTickCount64();
        const float deltaTime = static_cast<float>(now - g_game.lastTick) / 1000.0f;
        g_game.lastTick = now;

        RECT clientRect{};
        GetClientRect(hWnd, &clientRect);

        UpdateGame(g_game, clientRect, deltaTime);
        InvalidateRect(hWnd, nullptr, FALSE);
    }
    break;
```

**Schritte:**
1. Aktuelle Zeit ermitteln (`GetTickCount64`)
2. Delta-Zeit berechnen (in Sekunden)
3. `lastTick` aktualisieren
4. Spiellogik aktualisieren (`UpdateGame`)
5. Fenster neu zeichnen anfordern (`InvalidateRect`)

---

## UpdateLogik

### InitializeGameState

Initialisiert den Spielzustand beim Start:

```cpp
inline void InitializeGameState(GameState& game, const RECT& clientRect)
{
    (void)clientRect;  // Client-Rect wird nicht benötigt
    game.player.x = 0.0f;     // Spieler zentriert
    game.player.y = 0.0f;
    game.lastTick = GetTickCount64();
}
```

**Hinweis:** Der Spieler wird immer bei (0, 0)初始isiert - dies entspricht dem Zentrum des Client-Bereichs im Windows-koordinatensystem.

### SetMovementKey

Verarbeitet Tastatureingaben und aktualisiert den InputState:

```cpp
inline void SetMovementKey(InputState& input, WPARAM key, bool isPressed)
{
    switch (key)
    {
    case 'W':
    case VK_UP:
        input.up = isPressed;
        break;
    case 'S':
    case VK_DOWN:
        input.down = isPressed;
        break;
    case 'A':
    case VK_LEFT:
        input.left = isPressed;
        break;
    case 'D':
    case VK_RIGHT:
        input.right = isPressed;
        break;
    default:
        break;
    }
}
```

**Unterstützte Tasten:**
- WASD (ASCII Codes)
- Pfeiltasten (VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT)

### UpdateGame

Führt die Spiellogik aus und bewegt den Spieler basierend auf der Eingabe:

```cpp
inline void UpdateGame(GameState& game, const RECT& clientRect, float deltaTime)
{
    (void)clientRect;  // Client-Rect wird aktuell nicht benötigt

    float moveX = 0.0f;
    float moveY = 0.0f;

    // Bewegung berechnen
    if (game.input.left)  moveX -= 1.0f;
    if (game.input.right) moveX += 1.0f;
    if (game.input.up)    moveY -= 1.0f;
    if (game.input.down)  moveY += 1.0f;

    // Normalisieren für diagonale Bewegung (gleiche Geschwindigkeit in alle Richtungen)
    const float length = std::sqrt((moveX * moveX) + (moveY * moveY));
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    // Position aktualisieren
    game.player.x += moveX * game.player.speed * deltaTime;
    game.player.y += moveY * game.player.speed * deltaTime;
}
```

**Wichtige Aspekte:**
- **Normalisierung:** Wenn diagonale Bewegung erfolgt (z.B. W+D), wird die Geschwindigkeit normalisiert, damit die Diagonale nicht schneller ist als gerade Linien.
- **Delta-Zeit-basiert:** Die Positionsberechnung berücksichtigt `deltaTime`, was eine konsistente Geschwindigkeit unabhängig von der Frame-Rate gewährleistet.
- **ClientRect:** Aktuell nicht verwendet - könnte für Kollisionsabfrage mit Fensterbegrenzungen genutzt werden.

---

## Rendering (noch zu implementieren)

Die Render-Funktion wird in `WM_PAINT` aufgerufen:

```cpp
case WM_PAINT:
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);

    RenderGame(hdc, clientRect, g_game);  // TODO: Implementieren

    EndPaint(hWnd, &ps);
}
break;
```

**TODO:** Implementierung von `RenderGame` erforderlich.

---

## Fensterinitialisierung

### InitInstance

Stellt das Hauptfenster und initialisiert den Spielzustand:

```cpp
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 960, 540, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    InitializeGameState(g_game, clientRect);

    SetTimer(hWnd, GAME_TIMER_ID, TARGET_FRAME_MS, nullptr);  // ~60 FPS

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}
```

**Fenstergröße:** 960x540 Pixel

---

## Konstanten und Makros

Definiert in `main.cpp`:

| Konstante | Wert | Beschreibung |
|-----------|------|--------------|
| `MAX_LOADSTRING` | 100 | Maximale Länge für Strings |
| `GAME_TIMER_ID` | 1 | Timer-Handle für den Game Loop |
| `TARGET_FRAME_MS` | 16ms | Ziel-Framerate (~60 FPS) |

---

## Globale Variablen

| Variable | Typ | Beschreibung |
|----------|-----|--------------|
| `hInst` | HINSTANCE | Handle der aktuellen Instanz |
| `szTitle` | WCHAR[] | Fenstertiteltext |
| `szWindowClass` | WCHAR[] | Klassenname des Hauptfensters |
| `g_game` | GameState | Globale Spielzustandsvariable |

---

## Event-Handling

### WM_KEYDOWN / WM_KEYUP

Verarbeitet Tastatureingaben:

```cpp
case WM_KEYDOWN:
    if ((lParam & (1 << 30)) == 0)  // Nur linke Taste
    {
        SetMovementKey(g_game.input, wParam, true);
    }
    break;
case WM_KEYUP:
    SetMovementKey(g_game.input, wParam, false);
    break;
```

**Hinweis:** Die Prüfung `(lParam & (1 << 30)) == 0` stellt sicher, dass nur linke Tasten verarbeitet werden (verhindert Doppelaktivierung durch rechte Taste).

### WM_TIMER

Startet den Game Loop (siehe oben).

### WM_PAINT

Zeichnet das Spiel (TODO: Implementieren).

### WM_DESTROY

Beendet den Timer und schließt das Programm.

---

## Erweiterungsmöglichkeiten

### 1. Kollisionsdetektion mit Fensterrand

Aktuell wird der Client-Rect in `UpdateGame` ignoriert (`(void)clientRect`). Um den Spieler im Fenster zu halten:

```cpp
// Beispiel: Abfrage von Fensterrändern
if (game.player.x + game.player.size > clientRect.right - 10)
{
    game.player.x = static_cast<float>(clientRect.right - 10) - game.player.size;
}
```

### 2. Hindernisse oder Gegner

Hinzufügen von Objekten mit Kollisionsprüfung:

```cpp
struct GameObject
{
    float x, y;
    int size;
};

// In UpdateGame für jedes Objekt prüfen...
```

### 3. Spielstände speichern

Persistenz des Spielzustands implementieren (z.B. in `GameState` einen `saved` Flag hinzufügen).

---

## Zusammenfassung des Flusses

```
WM_CREATE → InitInstance → InitializeGameState → SetTimer
                                              ↓
                                      WM_TIMER (alle 16ms)
                                              ↓
                                GetTickCount64 → deltaTime berechnen
                                              ↓
                                         UpdateGame
                                              ↓
                                    InvalidateRect (WM_PAINT)
                                              ↓
                                   BeginPaint → RenderGame
                                              ↓
                                     EndPaint
```

---

## Referenzen

- Windows SDK Dokumentationen für `GetClientRect`, `BeginPaint`, etc.
- Game Loop Best Practices in der DirectX/OpenGL Community

---

## 📚 Projekt-Dokumentation

Alle wichtigen Informationen sind im [DOKUMENTATION](./DOKUMENTATION/) Ordner zusammengefasst:

| Dokument | Zweck |
|----------|-------|
| [DesignDocument.md](./DesignDocument.md) | **Hauptdokument** - Technische Architektur, Datenstrukturen, Game Loop, Event Handling |
| [README.md](./README.md) | Projekt-Übersicht mit Quick Start und Tech Stack |
| [DOKUMENTATION/README.md](./DOKUMENTATION/README.md) | Wie die Dokumentation zu nutzen ist |
| [DOKUMENTATION/AI-Nutzung-Leitfaden.md](./DOKUMENTATION/AI-Nutzung-Leitfaden.md) | Guide für AI-Assistenten (Copilot, etc.) - Wie sich an DesignDocument.md halten |
| [DOKUMENTATION/INDEX.md](./DOKUMENTATION/INDEX.md) | Übersicht aller Dokumentationen |

## Nutzungsempfehlungen für AI-Assistenten

**Wichtig:** Wenn du AI-Tools (wie Copilot) verwendest:

1. **Immer DesignDocument.md öffnen** - Das ist die Single Source of Truth
2. **Relevante Sektionen markieren** - z.B. "UpdateGame", "GameState", "Datenstrukturen"
3. **Kontext im Prompt geben** - "Basierend auf DesignDocument.md Sektion X..."
4. **Generierten Code prüfen** - Entspricht er dem Design?
5. **Änderungen dokumentieren** - Neue Features in "Erweiterungsmöglichkeiten" aufnehmen

## Referenzen

- Windows SDK Dokumentationen für `GetClientRect`, `BeginPaint`, etc.
- Game Loop Best Practices in der DirectX/OpenGL Community
