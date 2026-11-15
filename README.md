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
- **Einstellungsmenü** mit `Esc` öffnen oder schließen; das Spiel pausiert. Hoch/Runter wählt VSync, das FPS-Limit (60, 120, 200 oder unbegrenzt) oder die FPS-Anzeige; Links/Rechts, Enter oder Leertaste ändern den Wert. Bei VSync an ist das Limit ausgegraut und nicht auswählbar; der Wert bleibt für VSync aus erhalten. Die FPS-Anzeige ist auch im Release verfügbar. Änderungen werden in `%LOCALAPPDATA%\MyRPG\settings.json` gespeichert und beim Start geladen.
- **2D-Rendering** mit Spielerfigur, gekacheltem Kartenboden und Kartenobjekten
- **Kollisionen** mit als solide markierten Kartenobjekten
- **Karten** aus Textdateien im `.map`-Format; enthalten sind eine Testkarte und eine Waldkarte
- **Trigger** für den visuellen Magieeffekt und für dynamische Kartenwechsel
- **Kartenwechsel** mit Fade-out/Fade-in und pro Trigger festlegbarer Spawn-Position
- **Spieleranimationen**: Laufanimationen in acht Richtungen und Stillstandsanimationen in vier Richtungen, beschrieben in `.anim`-Dateien. Diagonale Blickrichtungen verwenden im Stillstand die linke bzw. rechte Animation.
- **Magieeffekte**: sieben GPU-Shader-Effekte (Sparkle, Fire, Lightning, Frost, Vortex, Heal, Shockwave) in `Assets/Shaders/Effects/`. `M` startet Sparkle an der Spielerposition; auf der Testkarte gibt es rechts vom Startpunkt für jeden Effekt eine Trigger-Zone (von links nach rechts in obiger Reihenfolge).
- **Textur-Cache** zur Wiederverwendung geladener Texturen
- **Kamera und Letterbox-Viewport** mit einer Basisauflösung von 1024 × 768; das Seitenverhältnis bleibt beim Ändern der Fenstergröße erhalten
- **Debug-Anzeigen** für Karten-ID, Position, letztes Ereignis, Raster und Triggerbereiche nur im Debug-Build. Im Release-Build bleiben diese Anzeigen verborgen; die Trigger-Funktion selbst bleibt aktiv.

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

Karten liegen als Textdateien unter `Assets/Maps/`. Leerzeilen und Zeilen, die mit `#` beginnen, werden ignoriert. Die Karte besteht aus diesen Anweisungen:

```text
MAP <mapId>
GROUND <spritePath>
OBJECT <id> <type> <x> <y> <spritePath> solid=<0|1>
TRIGGER <id> <x> <y> <width> <height> <eventName> [glow=0|1] [spawn_x=<x> spawn_y=<y>]
```

- `MAP` setzt die erforderliche Karten-ID. `GROUND` legt das Standard-Bodensprite fest, das den sichtbaren Kartenbereich gekachelt ausfüllt.
- `OBJECT` platziert ein Sprite in Weltkoordinaten. Typen sind `untergrund`, `scenery`, `wall` und `npc`; Positionen von `untergrund` und `scenery` werden auf das 64-Pixel-Raster gerundet.
- `solid` legt fest, ob das Objekt den Spieler blockiert: `solid=1` aktiviert die Kollision, `solid=0` lässt den Spieler hindurch. Das Flag ist erforderlich und unabhängig vom Typ – auch ein `scenery`-Objekt kann also entweder blockieren oder nicht. Für Boden- und Deko-Objekte, die begehbar sein sollen, wird üblicherweise `solid=0` verwendet.
- `TRIGGER` definiert ein rechteckiges Ereignisgebiet in Weltkoordinaten. Das Ereignis wird beim Betreten ausgelöst; nach Verlassen kann es bei erneutem Betreten wieder ausgelöst werden. `glow` ist optional und standardmäßig `0`. Mit `glow=1` erhält das Gebiet einen pulsierenden, additiven Shader-Glow. Das ist nur Darstellung und ändert weder Auslösung noch Kollision; der Glow ist auch im Release-Build sichtbar. Wenn Shader nicht unterstützt werden, wird stattdessen ein einfaches Rechteck gezeichnet. Debug-Markierungen der Trigger bleiben auf Debug-Builds beschränkt.

Beispiel:

```text
MAP 0001_TestMap
GROUND Assets/Sprites/Environment/Forest/forest_ground.png
OBJECT rock_01 scenery -192 -64 Assets/Sprites/Environment/Forest/forest_rock.png solid=1
OBJECT path_01 untergrund -64 0 Assets/Sprites/Environment/Forest/forest_path.png solid=0
TRIGGER magic_fire 448 -32 64 64 magic_fire glow=1
TRIGGER north_path_to_forest -64 -260 220 120 Load_0001_Forest glow=1 spawn_x=-128 spawn_y=1024
```

`magic_<name>` startet beim Betreten den vorhandenen Effekt gleichen Namens an der Spielerposition, zum Beispiel `magic_fire` für `Assets/Shaders/Effects/magic_fire.frag`.

`Load_<dateiname>` lädt `Assets/Maps/<dateiname>.map`, zum Beispiel lädt `Load_0002_Cave` die Datei `Assets/Maps/0002_Cave.map`. Der Dateiname ohne Endung darf keine Leerzeichen oder Pfadtrennzeichen enthalten; die `MAP`-ID in der Datei muss vorhanden sein, aber nicht dem Dateinamen entsprechen. Neue Karten benötigen keinen Codeeintrag. Die optionalen Trigger-Angaben `spawn_x` und `spawn_y` legen gemeinsam die Zielposition fest; ohne sie gilt `(0, 0)`. So können verschiedene Übergänge unterschiedliche Einstiegspunkte verwenden. Die Zielposition sollte frei von Hindernissen und nicht innerhalb eines sofort zurückführenden Triggers liegen. Wenn das Laden fehlschlägt, bleibt die bisherige Karte aktiv und ein Fehler wird auf stderr ausgegeben. Die Startkarte ist weiterhin in der Engine festgelegt.

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
