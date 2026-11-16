# MyRPG – eigene kleine RPG-Engine

MyRPG ist ein Windows-Projekt, in dem ich meine eigene kleine RPG-Engine mit C++20 und SFML 3 entwickle. Der aktuelle Stand ist ein spielbarer Prototyp mit Bewegung, Karten, Animationen, Kollisionen und einfachen Effekten.

> Ich schreibe meine eigene kleine RPG-Engine. Sie soll später auch ohne Programmierkenntnisse das Erstellen eines eigenen kleinen Adventures ermöglichen. Durch das Erstellen eigener Assets, Skripte und Maps soll man einfach seine eigene Welt erschaffen können.
>
> **Das ist das langfristige Ziel, noch nicht der aktuelle Funktionsumfang:** Der Prototyp enthält einen einfachen Map-Editor im Debug-Build, aber noch keine Skript-Laufzeit. Trigger-Ereignisse müssen in der Engine unterstützt werden.

## 📚 Dokumentation

Das [DesignDocument.md](./DesignDocument.md) beschreibt die technische Architektur, Spielsysteme, Daten- und Asset-Formate sowie die offenen Entwicklungsfragen.

## ✨ Aktueller Funktionsumfang

- **Hauptmenü** mit Titelbild, `Start Game` und `Exit Game`; Bedienung mit Hoch/Runter und Enter/Leertaste oder per Maus. `Start Game` beginnt ein neues Spiel.
- **Spielschleife** mit Delta-Time-basierter Bewegung und standardmäßig unbegrenzter Bildrate
- **Steuerung** mit WASD oder Pfeiltasten; diagonale Bewegung wird normalisiert
- **Einstellungsmenü** mit `Esc` öffnen oder schließen; das Spiel pausiert. `Zurück zum Hauptmenü` verwirft den aktuellen Spielzustand und ist per Enter/Leertaste oder Mausklick erreichbar. Hoch/Runter wählt VSync, das FPS-Limit (60, 120, 200 oder unbegrenzt) oder die FPS-Anzeige; Links/Rechts, Enter, Leertaste oder ein Linksklick ändern den Wert. Ein Klick schaltet das FPS-Limit zum nächsten Wert weiter. Bei VSync an ist das Limit ausgegraut und nicht auswählbar; der Wert bleibt für VSync aus erhalten. Die FPS-Anzeige ist auch im Release verfügbar. Änderungen werden in `%LOCALAPPDATA%\MyRPG\settings.json` gespeichert und beim Start geladen.
- **2D-Rendering** mit Spielerfigur, gekacheltem Kartenboden und Kartenobjekten
- **Kollisionen** mit als solide markierten Kartenobjekten
- **Karten** aus Textdateien im `.map`-Format; enthalten sind eine Testkarte und eine Waldkarte
- **Trigger** für den visuellen Magieeffekt und für dynamische Kartenwechsel
- **Kartenwechsel** mit Fade-out/Fade-in und pro Trigger festlegbarer Spawn-Position
- **Spieleranimationen**: Laufanimationen in acht Richtungen und Stillstandsanimationen in vier Richtungen, beschrieben in `.anim`-Dateien. Diagonale Blickrichtungen verwenden im Stillstand die linke bzw. rechte Animation.
- **Magieeffekte**: sieben GPU-Shader-Effekte (Sparkle, Fire, Lightning, Frost, Vortex, Heal, Shockwave) in `Assets/Shaders/Effects/`. `M` startet Sparkle an der Spielerposition; auf der Testkarte gibt es rechts vom Startpunkt für jeden Effekt eine Trigger-Zone (von links nach rechts in obiger Reihenfolge).
- **Screen Shake**: Ein eigenständiges Trigger-Ereignis erschüttert kurz die Spielwelt. Die Testzone liegt rechts hinter den Magie-Testzonen; auch der Lightning-Trigger löst den Shake aus.
- **Wetter**: Regen und Nebel als Shader-Overlays in `Assets/Shaders/Weather/`; das Standardwetter ist pro Karte festlegbar und kann über Trigger geändert oder ausgeschaltet werden. Die Waldkarte startet mit Nebel; auf der Testkarte liegen drei Wetter-Testzonen südlich des Startpunkts.
- **Textur-Cache** zur Wiederverwendung geladener Texturen
- **Kamera und Letterbox-Viewport** mit einer Basisauflösung von 1024 × 768; das Seitenverhältnis bleibt beim Ändern der Fenstergröße erhalten
- **Debug-Anzeigen** für Karten-ID, Position, letztes Ereignis, Raster und Triggerbereiche nur im Debug-Build. Ein Linksklick auf die Koordinaten kopiert X und Y in die Zwischenablage. Im Release-Build bleiben diese Anzeigen verborgen; die Trigger-Funktion selbst bleibt aktiv.
- **Map-Editor** im Debug-Build: `F2` während des Spiels öffnet ein separates Fenster und pausiert das Spiel. Nicht während eines Kartenübergangs verfügbar.

### Map-Editor

Im Editor öffnen `Neu` und `Oeffnen` Karten aus `Assets/Maps` neben der EXE. `Neu` verwendet den Dateinamen als Karten-ID; Namen dürfen Buchstaben, Ziffern, `_` und `-` enthalten. Links erscheint `Assets/Sprites` als auf- und zuklappbarer Ordnerbaum; `Environment` und seine Unterordner sind standardmäßig geöffnet. Ein Klick auf einen Ordner klappt ihn auf oder zu, das Mausrad scrollt die Liste. Ein Sprite auswählen und auf die Karte klicken platziert es. `Boden setzen` verwendet das gewählte Sprite als Standardboden, `Wetter` schaltet das Standardwetter durch. Neue Karten benötigen vor dem Speichern einen Boden.

`Tab` bzw. `Modus` wechselt zwischen Platzieren und Auswählen: Im Auswahlmodus Objekte anklicken und ziehen. Für ein ausgewähltes Objekt erscheint rechts ein Eigenschaftenmenü, in dem die Checkbox `Solid aktivieren` die Kollision ein- oder ausschaltet. Im Platziermodus setzt dieselbe Checkbox die Vorgabe für neue Objekte. `Typ` in der Werkzeugleiste ändert weiterhin das ausgewählte Objekt oder die Vorgabe für neue Objekte. `Entf` löscht das ausgewählte Objekt oder, wenn keines ausgewählt ist bzw. im Platziermodus, das oberste Objekt unter dem Mauszeiger. Darunter liegende Objekte bleiben erhalten; wo kein Objekt mehr liegt, erscheint der Standardboden, ohne einen neuen `OBJECT`-Eintrag zu erzeugen. Mittlere Maustaste verschiebt die Ansicht, das Mausrad über der Karte zoomt. `Startpunkt` bzw. `P` und ein Klick wählen die Position für die Spielerfigur; sie ist nur für die Übernahme im Editor gültig. Spritesheets werden als ganzes Bild behandelt.

`Trigger-Werkzeug (T)` aktiviert den Trigger-Modus: Auf einer freien Stelle klicken und ziehen erstellt eine rechteckige Zone; ein einfacher Klick erzeugt eine 64 × 64 große Zone. Eine vorhandene Zone anklicken und im Inneren ziehen verschiebt sie. Die ausgewählte Zone ist cyan umrandet: Durch Ziehen ihrer Seiten oder Ecken lässt sich ihre Größe ändern; die gegenüberliegende Seite bleibt fest und die Mindestgröße beträgt eine Welteinheit pro Richtung. Mit gedrückter `Umschalt`-Taste lässt sich die ausgewählte Zone auch am Rand verschieben, etwa wenn sie sehr klein ist. Mit `Entf` wird sie gelöscht. Rechts erscheint ihr Eigenschaftenmenü mit zwei Dropdowns: **Load Map** listet `.map`-Dateien direkt unter `Assets/Maps` im Spielordner; **Effekte** bietet Screen Shake, Wetter aus sowie die beim Start erkannten Magie- und Wettereffekte. Die geöffnete Liste lässt sich mit dem Mausrad scrollen. Ein Trigger hat höchstens ein Ereignis: Eine Auswahl ersetzt das bisherige Ereignis. **Kein Ereignis** in einer der beiden Listen entfernt die bestehende Zuordnung vollständig; Glow und weitere Einstellungen bleiben erhalten. Die Checkbox `Glow aktivieren` im Eigenschaftenmenü oder `Glow` in der Werkzeugleiste schaltet das sichtbare Leuchten um; es ist auch im Release sichtbar und beeinflusst weder Ereignis noch Kollision. Neue Zonen beginnen mit `screen_shake` und ohne Glow. `Tab` oder die Auswahl eines Sprites beendet den Trigger-Modus.

Beim Zuweisen eines Kartenwechsels bleibt die vorhandene Zielposition erhalten; für neue Trigger gilt `(0, 0)`. Die Position ist im Eigenschaftenmenü sichtbar, kann dort aber noch nicht geändert werden. Vorhandene Spawn- und weitere Triggeroptionen bleiben beim Bearbeiten erhalten.

`Speichern` schreibt nur in den Spielordner. `Im Spiel uebernehmen` aktiviert die gespeicherte Karte und setzt die Figur auf den gewählten Startpunkt; bei ungespeicherten Änderungen wird vorher nach dem Speichern gefragt. Vor Kollisionen und Triggern am Startpunkt wird gewarnt. Der Editor bleibt bis zum Schließen geöffnet und das Spiel pausiert. Ungespeicherte Änderungen können beim Schließen oder Kartenwechsel verworfen werden.

Vor dem ersten Überschreiben einer vorhandenen Karte wird eine unveränderliche Sicherung `<dateiname>.map.original.bak` daneben angelegt. Sie wird nie automatisch gelöscht oder ersetzt. Änderungen und Sicherungen liegen im Build-Ausgabeverzeichnis: Ein Build kann Karten überschreiben, und Bereinigen kann den Ordner entfernen. Fertige Karten und zusätzliche Sprites daher rechtzeitig manuell ins Projekt übernehmen oder anderweitig sichern. Im Debug-Build ist der EXE-Ordner das Arbeitsverzeichnis, damit Spiel und Editor dieselben Assets verwenden.

## 🚀 Quick Start

### Voraussetzungen

- Windows x64 und Visual Studio 2026 mit der Workload **Desktopentwicklung mit C++** (MSVC v145 und Windows SDK).
- Git, eine Internetverbindung für den ersten Build und ein initialisiertes [vcpkg](https://github.com/microsoft/vcpkg). Das mit Visual Studio mitgelieferte vcpkg kann verwendet werden; eine globale vcpkg-Integration ist nicht nötig.

### Projekt starten

1. Öffne `MyRPG.slnx` in Visual Studio.
2. Wähle **Debug** und **x64**, baue über **Build → Build Solution** und starte das Programm.

Vor dem Kompilieren installiert der Build SFML 3, nlohmann-json und deren Abhängigkeiten aus dem Manifest automatisch für `x64-windows` in `vcpkg_installed/`. Der erste Build kann wegen Downloads und Kompilierung länger dauern. Bei späteren Builds prüft vcpkg die Abhängigkeiten erneut und übernimmt auch Änderungen am Manifest oder an der Registry-Konfiguration; bereits aktuelle Pakete werden nicht erneut installiert. `vcpkg_installed/` wird nicht versioniert.

Das Projekt sucht vcpkg zuerst unter `VCPKG_ROOT`, dann in der Visual-Studio-Installation und unter `%USERPROFILE%\vcpkg`; andernfalls muss `vcpkg.exe` im `PATH` liegen. Für MSBuild kann der Pfad zur ausführbaren Datei auch mit `/p:MyRPGVcpkgExe="C:\Pfad\zu\vcpkg.exe"` angegeben werden. Ist kein initialisiertes vcpkg vorhanden, kann es einmalig außerhalb des Projekts eingerichtet werden:

```powershell
git clone https://github.com/microsoft/vcpkg.git "$env:USERPROFILE\vcpkg"
& "$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat"
```

Schlägt die Installation fehl, zeigt die Build-Ausgabe den vcpkg-Fehler; prüfe insbesondere Internetverbindung, Git und den gewählten vcpkg-Pfad und starte den Build erneut. Das Projekt unterstützt ausschließlich Windows x64.

### So bindet das Projekt SFML ein

Die Abhängigkeiten sind im Manifest `vcpkg.json` deklariert; SFML muss nicht separat heruntergeladen oder manuell in den Visual-Studio-Projekteigenschaften eingetragen werden. Ein MSBuild-Schritt installiert das Manifest vor dem Kompilieren; die Projektdatei verwendet die installierten Dateien unter `vcpkg_installed/x64-windows/`:

- Header werden aus `include/` eingebunden, zum Beispiel mit `#include <SFML/Graphics.hpp>`.
- Im **Debug**-Build werden die Debug-Bibliotheken aus `debug/lib/` gelinkt (`sfml-graphics-d.lib`, `sfml-window-d.lib` und `sfml-system-d.lib`). Im **Release**-Build werden die entsprechenden Release-Bibliotheken aus `lib/` verwendet.
- Nach dem Build kopiert ein MSBuild-Schritt automatisch die passenden SFML-Laufzeit-DLLs aus `debug/bin/` (Debug) beziehungsweise `bin/` (Release) neben die ausführbare Datei. Auch die Inhalte von `Assets/` werden in das Ausgabeverzeichnis kopiert.

Damit lässt sich das Programm direkt aus dem jeweiligen Build-Ausgabeverzeichnis starten. Für eine andere vcpkg-Architektur oder ein anderes Triplet müsste neben der Installation auch die derzeit auf `x64-windows` festgelegte Projektkonfiguration angepasst werden.

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
│   ├── Weather.h                   # Wetterzustand und Steuerung
│   ├── WeatherRenderer.h           # Wetter-Shader-Overlay
│   ├── MapLoader.h                # Parser für .map-Dateien
│   ├── MainMenu.h                 # Titelbildschirm und Hauptmenü
│   ├── MenuFont.h                 # Gemeinsame Systemschrift für Menüs
│   ├── SettingsMenu.h             # Einstellungsmenü und VSync
│   ├── SettingsStore.h            # JSON-Einstellungen laden und speichern
│   └── AnimationLoader.h          # Parser für .anim-Dateien
├── Assets/
│   ├── Animations/Characters/     # Spieleranimationen im .anim-Format
│   ├── Maps/                      # 0001_TestMap.map und 0001_Forest.map
│   ├── Shaders/Effects/           # GLSL-Shader der Magieeffekte
│   ├── Shaders/Weather/           # GLSL-Shader für Regen und Nebel
│   ├── UI/                        # Titelbildschirm
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
WEATHER <none|shaderName>
OBJECT <id> <type> <x> <y> <spritePath> solid=<0|1>
TRIGGER <id> <x> <y> <width> <height> <eventName> [glow=0|1] [spawn_x=<x> spawn_y=<y>]
```

- `MAP` setzt die erforderliche Karten-ID. `GROUND` legt das Standard-Bodensprite fest, das den sichtbaren Kartenbereich gekachelt ausfüllt.
- `WEATHER` ist optional und setzt das Standardwetter auf `none` oder den Dateinamen eines Wetter-Shaders ohne `.frag`, zum Beispiel `rain` für `rain.frag`; ohne Angabe gilt `none`. Alle `.frag`-Dateien unter `Assets/Shaders/Weather/` werden beim Start automatisch erkannt und vorgeladen. Ein neuer Effekt benötigt keinen Codeeintrag; er ist nach dem nächsten Build und Spielstart verfügbar. Ungültige oder mehrfache Angaben verhindern das Laden der Karte. Das Wetter wird über der Spielwelt, aber unter UI und Kartenübergang gezeichnet und pausiert im Einstellungsmenü. Wenn ein Shader nicht verfügbar ist, wird der Fehler auf stderr gemeldet und der Effekt nicht gezeichnet.
- `OBJECT` platziert ein Sprite in Weltkoordinaten. Typen sind `untergrund`, `scenery`, `wall` und `npc`; Positionen von `untergrund` und `scenery` werden auf das 64-Pixel-Raster gerundet.
- `solid` legt fest, ob das Objekt den Spieler blockiert: `solid=1` aktiviert die Kollision, `solid=0` lässt den Spieler hindurch. Das Flag ist erforderlich und unabhängig vom Typ – auch ein `scenery`-Objekt kann also entweder blockieren oder nicht. Für Boden- und Deko-Objekte, die begehbar sein sollen, wird üblicherweise `solid=0` verwendet.
- `TRIGGER` definiert ein rechteckiges Ereignisgebiet in Weltkoordinaten. Das Ereignis wird beim Betreten ausgelöst; nach Verlassen kann es bei erneutem Betreten wieder ausgelöst werden. Der Ereignisname `none` bedeutet, dass beim Betreten nichts ausgelöst wird; er unterscheidet sich von `weather_none`, das das Wetter ausschaltet. `glow` ist optional und standardmäßig `0`. Mit `glow=1` erhält das Gebiet einen pulsierenden, additiven Shader-Glow. Das ist nur Darstellung und ändert weder Auslösung noch Kollision; der Glow ist auch im Release-Build sichtbar. Wenn Shader nicht unterstützt werden, wird stattdessen ein einfaches Rechteck gezeichnet. Debug-Markierungen der Trigger bleiben auf Debug-Builds beschränkt.

Beispiel:

```text
MAP 0001_TestMap
GROUND Assets/Sprites/Environment/Forest/forest_ground.png
WEATHER rain
OBJECT rock_01 scenery -192 -64 Assets/Sprites/Environment/Forest/forest_rock.png solid=1
OBJECT path_01 untergrund -64 0 Assets/Sprites/Environment/Forest/forest_path.png solid=0
TRIGGER magic_fire 448 -32 64 64 magic_fire glow=1
TRIGGER screen_shake_test 1408 -32 64 64 screen_shake glow=1
TRIGGER clear_weather -32 544 64 64 weather_none glow=1
TRIGGER north_path_to_forest -64 -260 220 120 Load_0001_Forest glow=1 spawn_x=-128 spawn_y=1024
```

`magic_<name>` startet beim Betreten den vorhandenen Effekt gleichen Namens an der Spielerposition, zum Beispiel `magic_fire` für `Assets/Shaders/Effects/magic_fire.frag`. Der Lightning-Trigger startet zusätzlich einen Screen Shake.

`screen_shake` startet unabhängig von einem Magieeffekt einen kurzen Screen Shake (Stärke 8 Pixel, Dauer 0,32 Sekunden). Nach Verlassen und erneutem Betreten der Zone wird er erneut ausgelöst. Die Testzone liegt bei x=1408, y=-32. Wetter-Overlay und UI bleiben bildschirmfest.

Trigger-Ereignisse der Form `weather_<shaderName>` schalten auf den gleichnamigen automatisch erkannten Shader um; `weather_none` schaltet das Wetter aus. Beispielsweise aktiviert `weather_rain` die Datei `rain.frag`. Das Wetter bleibt bis zur nächsten Änderung aktiv. Beim erfolgreichen Kartenwechsel übernimmt die Zielkarte ihr eigenes Standardwetter; schlägt der Wechsel fehl, bleibt das bisherige Wetter erhalten. Auf der Testkarte liegen die Wetterzonen südlich des Startpunkts bei x=-32 und y=224, 384 und 544 (Regen, Nebel, aus).

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
