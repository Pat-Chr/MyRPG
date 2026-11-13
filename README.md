# MyRPG - RPG Spiel Framework

Ein minimalistisches Top-Down RPG-Spiel für Windows, geschrieben in C++ mit SFML (2D-Rendering).

## 📚 Dokumentation

Das Design-Dokument enthält alle technischen Details zur Architektur und Implementierung:

- **[DesignDocument.md](./DesignDocument.md)** - Vollständige technische Dokumentation
  - Spielmechanik
  - Datenstrukturen (GameState, Player, InputState, MapData, ...)
  - Game Loop Implementation
  - Update-Logik
  - Rendering-Struktur
  - Event Handling
  - Erweiterungsmöglichkeiten

## ✨ Features

- **Game Loop** mit Delta-Time-basierter, zeitunabhängiger Bewegung (60 FPS, Framerate-Limit)
- **Spieler-Steuerung** über WASD oder Pfeiltasten
- **2D-Rendering** mit SFML: Spieler-Sprite, Kachelboden, Scenery-Objekte
- **Kollisionsdetektion** gegen solide Map-Objekte (Wände, Felsen, ...)
- **Kartensystem** mit eigenem Text-Map-Format (`.map`): Boden, Objekte, Trigger-Zonen
- **Trigger-Zonen** mit Event-Namen (z. B. `Trigger_SpawnArea`, `Load_0001_Forest`)
- **Map-Transitions** mit Fade-Out/Fade-In und automatischem Kartenwechsel + Spawn-Position
- **Sprite-Animationen** über einfache `.anim`-Dateien (Texture, Frame-Dauer, Frame-Rects); die Hauptfigur hat acht Laufrichtungen und behält im Stand ihre Blickrichtung
- **Textur-Cache** für effizientes Rendern (keine redundanten Ladevorgänge)
- **Letterbox-Viewport** (Basisauflösung 1024×768, Seitenverhältnis bleibt beim Resize erhalten)
- **Kamera** folgt dem Spieler (Spieler als virtuelles Kamera-Zentrum)

## 🚀 Quick Start

1. Abhängigkeiten installieren (SFML via vcpkg): `vcpkg install`
2. Öffne die Lösung in Visual Studio: `MyRPG.slnx`
3. Stelle sicher, dass du Debug-Konfiguration verwendest
4. Baue das Projekt: `Build → Build Solution`
5. Führe aus im Debug-Modus

## 📁 Projektstruktur

```
MyRPG/
├── main.cpp                    # Entry Point, Game Loop, Input, Letterbox-Viewport
├── src/
│   └── engine/
│       ├── GameTypes.h         # Kern-Datenstrukturen (Player, MapData, GameState, ...)
│       ├── GameEngine.h        # Update-Logik, Kollision, Trigger, Map-Transitions
│       ├── GameRenderer.h      # SFML-Rendering, Kamera, Textur-Cache
│       ├── MapLoader.h         # Parser für das .map-Format
│       └── AnimationLoader.h   # Parser für das .anim-Format
├── Assets/
│   ├── Animations/Characters/  # .anim-Dateien (z. B. player_walk.anim)
│   ├── Maps/                   # .map-Dateien (TestMap, Forest)
│   └── Sprites/                # Charakter- und Umgebungs-Sprites (PNG)
├── vcpkg.json                  # Paket-Abhängigkeiten (sfml)
└── [Visual Studio Projektdateien]
```

## 🗺️ Map-Format (`.map`)

Einfaches Textformat, das von `MapLoader.h` eingelesen wird:

```
# MAP <mapId>
# GROUND <defaultSpritePath>
# OBJECT <id> <type> <x> <y> <spritePath> <solid(0|1)>
# TRIGGER <id> <x> <y> <width> <height> <eventName>

MAP 0001_TestMap
GROUND Assets/Sprites/Environment/Forest/forest_ground.png
OBJECT rock_01 scenery -192 -64 Assets/Sprites/Environment/basic_rock.png 1
TRIGGER north_path_to_forest -64 -260 220 120 Load_0001_Forest
```

Objekt-Typen: `untergrund`, `scenery`, `wall`, `npc`.
Trigger-Events werden in `GameEngine.h` (`TryResolveMapTransitionTarget`) aufgelöst – z. B. führt `Load_0001_Forest` zu einem Kartenwechsel mit Fade-Transition.

## 🛠️ Technologie-Stack

- **C++17** oder neuer
- **SFML** (2D-Rendering, Fenster, Input) – via vcpkg
- **Windows** (Win32-Host)

## 📝 Nächste Schritte

1. Integriere Gegner/Hindernisse (NPC-Typ im Map-Format bereits vorhanden)
2. Implementiere Spielstände (Speichern/Laden)
3. Erweitere das Animationssystem (weitere Animationen)
4. Füge Gameplay-Systeme hinzu (Kampf, Inventar, Dialoge)

## 🔧 Entwicklungshinweise

- Alle Änderungen müssen mit dem Design-Dokument abgestimmt sein
- Neue Features sollten in die Dokumentation aufgenommen werden
- Verwende Delta-Time für zeitunabhängige Bewegung
- Behalte Normalisierung bei diagonalen Bewegungen bei
- Neue Karten einfach als `.map`-Datei unter `Assets/Maps/` anlegen und im Trigger-Resolver registrieren

---

*Für detaillierte technische Informationen: [DesignDocument.md](./DesignDocument.md)*
