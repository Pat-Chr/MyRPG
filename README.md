# MyRPG – eigene kleine RPG-Engine

MyRPG ist ein Windows-Projekt, in dem ich meine eigene kleine RPG-Engine mit C++20 und SFML 3 entwickle. Der aktuelle Stand ist ein spielbarer Prototyp mit Bewegung, Karten, Animationen, Kollisionen und einfachen Effekten.

> Ich schreibe meine eigene kleine RPG-Engine. Sie soll später auch ohne Programmierkenntnisse das Erstellen eines eigenen kleinen Adventures ermöglichen. Durch das Erstellen eigener Assets, Skripte und Maps soll man einfach seine eigene Welt erschaffen können.
>
> **Das ist das langfristige Ziel, noch nicht der aktuelle Funktionsumfang:** Der Prototyp hat derzeit weder einen visuellen Editor noch eine Skript-Laufzeit. Karten und Animationen werden momentan über Dateien erstellt; Trigger-Ereignisse müssen in der Engine unterstützt werden.

## 📚 Dokumentation

Das [DesignDocument.md](./DesignDocument.md) beschreibt die technische Architektur, Spielsysteme, Daten- und Asset-Formate sowie die offenen Entwicklungsfragen.

## ✨ Aktueller Funktionsumfang

- **Spielschleife** mit Delta-Time-basierter Bewegung und standardmäßig unbegrenzter Bildrate
- **Steuerung** mit WASD oder Pfeiltasten; diagonale Bewegung wird normalisiert
- **Einstellungsmenü** mit `Esc` öffnen oder schließen; das Spiel pausiert. Hoch/Runter wählt VSync oder das FPS-Limit (60, 120, 200 oder unbegrenzt); Links/Rechts, Enter oder Leertaste ändern den Wert. Bei VSync an ist das Limit ausgegraut und nicht auswählbar; der Wert bleibt für VSync aus erhalten. Änderungen werden in `%LOCALAPPDATA%\MyRPG\settings.json` gespeichert und beim Start geladen.
- **2D-Rendering** mit Spielerfigur, gekacheltem Kartenboden und Kartenobjekten
- **Kollisionen** mit als solide markierten Kartenobjekten
- **Karten** aus Textdateien im `.map`-Format; enthalten sind eine Testkarte und eine Waldkarte
- **Trigger** für den visuellen Magieeffekt und für Kartenwechsel zwischen den vorhandenen Karten
- **Kartenwechsel** mit Fade-out/Fade-in und festgelegter Spawn-Position
- **Spieleranimationen** in acht Richtungen, beschrieben in `.anim`-Dateien; im Stillstand bleibt die letzte Blickrichtung erhalten
- **Magieeffekte**: sieben GPU-Shader-Effekte (Sparkle, Fire, Lightning, Frost, Vortex, Heal, Shockwave) in `Assets/Shaders/Effects/`. `M` startet Sparkle an der Spielerposition; auf der Testkarte gibt es rechts vom Startpunkt für jeden Effekt eine Trigger-Zone (von links nach rechts in obiger Reihenfolge).
- **Textur-Cache** zur Wiederverwendung geladener Texturen
- **Kamera und Letterbox-Viewport** mit einer Basisauflösung von 1024 × 768; das Seitenverhältnis bleibt beim Ändern der Fenstergröße erhalten
- **Debug-Anzeigen** für Karten-ID, Position, FPS, letztes Ereignis, Raster und Triggerbereiche nur im Debug-Build. Im Release-Build bleiben diese Anzeigen verborgen; die Trigger-Funktion selbst bleibt aktiv.

## 🚀 Quick Start

1. Öffne `MyRPG.slnx` in Visual Studio.
2. Stelle die vcpkg-Abhängigkeiten aus `vcpkg.json` wieder her (SFML 3 und nlohmann-json). Führe dazu im Projektordner `vcpkg install --triplet x64-windows` aus.
3. Wähle die Konfiguration **Debug** und die Plattform **x64** und baue die Lösung über **Build → Build Solution**.
4. Starte das Programm im Debug-Modus.

Das Projekt unterstützt ausschließlich Windows x64. `vcpkg_installed/` enthält lokal erzeugte Abhängigkeiten und vcpkg-Verwaltungsdaten und wird nicht versioniert. Die Dateien können aus dem Manifest wiederhergestellt werden.

Beim Build werden die Inhalte aus `Assets/` sowie die zur Konfiguration passenden SFML-Laufzeit-DLLs automatisch neben die ausführbare Datei kopiert.

## 📁 Projektstruktur

```
MyRPG/
├── main.cpp                       # Win32-Einstiegspunkt, Spielschleife, Eingabe und Viewport
├── src/engine/
│   ├── GameTypes.h                # Spielzustand und Datenstrukturen
│   ├── GameEngine.h               # Bewegung, Kollisionen, Trigger und Kartenwechsel
│   ├── GameRenderer.h              # Rendering, Kamera und Textur-Cache
│   ├── MagicEffects.h              # Zustand und Start-API für visuelle Effekte
│   ├── MagicRenderer.h             # Darstellung der Effekte
│   ├── MapLoader.h                # Parser für .map-Dateien
│   ├── SettingsMenu.h             # Einstellungsmenü und VSync
│   ├── SettingsStore.h            # JSON-Einstellungen laden und speichern
│   └── AnimationLoader.h          # Parser für .anim-Dateien
├── Assets/
│   ├── Animations/Characters/     # Spieleranimationen im .anim-Format
│   ├── Maps/                      # 0001_TestMap.map und 0001_Forest.map
│   ├── Shaders/Effects/           # GLSL-Shader der Magieeffekte
│   └── Sprites/
│       ├── Characters/            # Spieler-Spritesheets
│       └── Environment/           # Umgebungs- und Wald-Sprites
├── MyRPG.vcxproj                  # Visual-Studio-Projekt und Asset-Kopierschritt
├── MyRPG.slnx                     # Visual-Studio-Lösung
└── vcpkg.json                     # Abhängigkeiten: SFML und nlohmann-json
```

## 🗺️ Karten erstellen (`.map`)

Der `MapLoader` liest einfache Textdateien mit den Anweisungen `MAP`, `GROUND`, `OBJECT` und `TRIGGER`. Ein Beispiel aus der Testkarte:

```text
MAP 0001_TestMap
GROUND Assets/Sprites/Environment/Forest/forest_ground.png
OBJECT rock_01 scenery -192 -64 Assets/Sprites/Environment/Forest/forest_rock.png 1
TRIGGER north_path_to_forest -64 -260 220 120 Load_0001_Forest
```

`OBJECT` verwendet die Felder `<id> <type> <x> <y> <spritePath> <solid(0|1)>`. Unterstützte Typen sind `untergrund`, `scenery`, `wall` und `npc`. `TRIGGER` verwendet `<id> <x> <y> <width> <height> <eventName>`. Aktuell sind `Effect_Magic`, `Effect_Fire`, `Effect_Lightning`, `Effect_Frost`, `Effect_Vortex`, `Effect_Heal`, `Effect_Shockwave`, `Load_0001_Forest` und `Load_0001_TestMap` in der Engine mit einer Aktion verknüpft; weitere Ereignisse benötigen noch eine Implementierung.

## 🛠️ Technologie

- **C++20**, **SFML 3** und **nlohmann-json** (Bibliotheken über vcpkg)
- **Windows** mit Win32-Einstiegspunkt
- **Visual Studio**-Projektdatei `MyRPG.vcxproj`

## 🔭 Ausblick

MyRPG ist noch im Prototyp-Stadium. Zum langfristigen Ziel gehören eine einfachere Erstellung von Inhalten ohne Programmierkenntnisse sowie weitere Adventure-Systeme wie Dialoge, Kampf und Spielstände. Neue Karten können bereits als `.map`-Dateien angelegt werden; Animationen und Sprites liegen ebenfalls getrennt von der Engine in `Assets/`.

## 🔧 Entwicklungshinweise

- Änderungen und neue Asset-Standards mit dem [Design-Dokument](./DesignDocument.md) abstimmen und dort dokumentieren.
- Bewegung weiterhin Delta-Time-basiert halten und diagonale Eingaben normalisieren.
- Debug-Informationen und Trigger-Markierungen ausschließlich im Debug-Build anzeigen.
- Neue Assets unter `Assets/` ablegen; sie werden beim Build automatisch mitkopiert.

---

*Für Details zu Architektur und Datenformaten siehe [DesignDocument.md](./DesignDocument.md).*
