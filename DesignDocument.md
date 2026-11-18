# Game Design Document (GDD) – Ausfüllbare Vorlage

## 0. Dokument-Info
- **Projektname:** MyRPG
- **Version:** v1.0
- **Stand:** 11.11.2026
- **Autor(en):** Ihr Name
- **Status:** Prototyp

---

## 1. Vision & Elevator Pitch
### Kurzbeschreibung (1–3 Sätze)
MyRPG ist ein minimalistisches RPG-Spiel für Windows, geschrieben in C++ mit SFML. Der aktuelle Fokus liegt auf einer stabilen Game-Loop- und Input-Basis als Fundament für spätere Gameplay-Systeme.

### Genre & Perspektive
- **Genre:** RPG (aktuell als Framework/Grundgerüst)
- **Perspektive (z. B. Top-Down, 2D-Side, First Person):** Top-Down (2D)

### Zielgruppe
- **Alter:** 16+
- **Spielertyp:** Spieler mit Interesse an klassischem RPG-Gameplay
- **Erfahrung (Casual/Hardcore):** Casual

### Plattform(en)
- PC (Windows)

### USP (Alleinstellungsmerkmal)
Technisch sauberes, erweiterbares RPG-Grundgerüst mit Delta-Time-basierter Bewegung und klar dokumentierter Architektur.

---

## 2. Core Gameplay Loop
### Hauptschleife
1. Eingabe lesen (WASD/Pfeiltasten über SFML Keyboard-State)
2. Spielzustand mit Delta-Time aktualisieren (`UpdateGame`)
3. Szene pro Frame rendern (`RenderGame`)
4. Frame präsentieren (`window.display`)

### Session-Ziel (pro 5–15 Minuten)
Aktuell: Bewegung testen, Input prüfen und technische Stabilität des Loops validieren.

### Langfristiges Ziel (Metaprogression)
Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

---

## 3. Spielerfantasie & Spielgefühl
### Spielerrolle
Steuerung eines einzelnen Spieler-Avatars im Fensterraum.

### Gewünschtes Gefühl
- Spannung: Ruhig mit punktuellen Peaks
- Entdeckung: Mittel
- Machtfantasie: Langsames Wachstum
- Risiko/Belohnung: Ausgewogen

---

## 4. Kernsysteme
### 4.1 Bewegung
- Laufgeschwindigkeit: 220 px/s (`Player.speed`)
- Sprint/Ausweichen: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Kollision: Bewusst offen (Prototyp-Phase), als nächster technischer Schritt geplant

### 4.2 Kampf
- Echtzeit / Rundenbasiert: Echtzeit
- Primärwaffen: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Sekundärfähigkeiten: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Schadenstypen: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Trefferfeedback: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

### 4.3 Ressourcen
- Gesundheit: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Mana/Stamina: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Währung: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Verbrauchsgegenstände: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

### 4.4 Progression
- Levelsystem: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Skilltree: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Ausrüstung: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Freischaltungen: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

### 4.5 Gegner/KI
- Gegnertypen: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Verhaltensmuster: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Boss-Design: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

---

## 5. Regeln, Sieg/Niederlage, Balancing
### Siegbedingungen
Hauptstory abschließen

### Niederlagebedingungen
- HP auf 0
- Fortsetzung über letztes Savegame
- Optional geplant: Checkpoint-System mit Penalty (ähnlich Diablo 2)

### Schwierigkeit
- Modus(e): Ein Modus vorerst
- Skalierung: Keine Skalierung vorerst

### Balancing-Prinzipien
- Delta-Time-basierte Bewegung für konsistentes Verhalten über verschiedene Framerates
- Normalisierung diagonaler Bewegung, damit keine Richtungs-Vorteile entstehen

---

## 6. Welt-, Level- und Content-Design
### Weltstruktur
- Lineare Regionen

### Biome/Regionen
- Noch keine Biome definiert

### Level-Pacing
- Intro: Bewegungs- und Input-Basis steht
- Midgame: Freieres Regionen-Fortschreiten
- Endgame: Story-Finale mit Bossfokus

### Checkpoints/Respawn
Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

### Quests/Inhalte
- Hauptquests: Start mit Hauptstory und Welt; weitere Inhalte folgen schrittweise
- Nebenquests: Ergänzungen kommen nach und nach dazu
- Zufallsereignisse: Nein, vorerst keine

---

## 7. Narrative Design
### Prämisse
Story-driven Action RPG

### Hauptfiguren
- Name: Start ohne festen Namen; spätere freie Benennung durch Spieler
- Rolle: Steuerbare Hauptfigur
- Motivation: Noch nicht festgelegt (Story steht noch nicht fest)

### Story-Struktur (3 Akte oder Kapitel)
- Akt/Kapitel 1: Ruhiger Start mit Mysterium
- Akt/Kapitel 2: Man bekommt eine vorher noch unbekannte Fähigkeit
- Akt/Kapitel 3: Mehrdeutiges Ende

### Dialog- und Tonalitätsregeln
Mischung aus ernst und leicht humorvoll

---

## 8. UX, UI und Steuerung
### Eingabe
- Tastatur/Maus: WASD + Pfeiltasten für Bewegung; `M` startet testweise einen Magieeffekt an der aktuellen Spielerposition.
- Spieleingaben werden nur angenommen, solange das Fenster den Fokus hat. Bei Fokusverlust wird die Bewegung gestoppt; das Spiel läuft ansonsten weiter.
- Controller: Später geplant

### HUD
- Muss anzeigen: Health, Ausdauer, XP, Level
- Optional: Weitere Elemente kommen später hinzu
- Die optionale FPS-Anzeige ist in Debug- und Release-Builds verfügbar und im Einstellungsmenü abschaltbar.
- Nur im Debug-Build: Karten-ID, Spielerkoordinaten und letztes Trigger-Ereignis; zusätzlich werden Trigger-Bereiche und das Debug-Raster sichtbar dargestellt. Ein Linksklick auf die angezeigten Spielerkoordinaten kopiert X und Y in die Zwischenablage. Im Release-Build entfallen diese Debug-Anzeigen und die Kopierfunktion, die Trigger-Funktion bleibt erhalten.
- Diese Trennung ist eine dauerhafte Vorgabe und soll bei zukünftigen Änderungen beibehalten werden: Debug-Informationen und Trigger-Markierungen sind nur in Debug-Builds sichtbar; Release-Builds bleiben frei von diesen Anzeigen. Die FPS-Anzeige ist eine Spieleroption und keine Debug-Anzeige.

### Menüs
- Hauptmenü: Beim Start erscheint ein vorläufiger Titelbildschirm mit `Start Game` und `Exit Game`. Hoch/Runter und Enter/Leertaste oder ein Mausklick bedienen die Schaltflächen. `Start Game` beginnt ein neues Spiel auf der Startkarte; `Exit Game` beendet das Programm. Die Rückkehr aus den Einstellungen verwirft den aktuellen Spielzustand; Spielstände sind noch nicht implementiert.
- Pause: Das Spiel pausiert, solange das Einstellungsmenü oder der Map-Editor geöffnet ist; Bewegung, Trigger, Effekte und Kartenwechsel laufen in dieser Zeit nicht weiter.
- Map-Editor: `F2` öffnet während des Spiels außerhalb von Kartenübergängen ein separates, in der Größe veränderbares Fenster. Die feste Editorfläche von 1280×800 (16:10) behält wie das Spielfenster ihr Seitenverhältnis durch zentriertes Letterboxing; Layout, Kartenansicht und Mausbedienung skalieren gemeinsam. Im Debug-Build ist er direkt verfügbar, im vorläufigen Release-Build erst nach Aktivierung der standardmäßig ausgeschalteten, gespeicherten Einstellung `Map-Editor`. Der Build-Schalter `MYRPG_MAP_EDITOR_ENABLED` in `src/BuildConfig.h` schließt bei `0` Editor und Release-Einstellung vollständig aus. Die Debug-Anzeigen im Spielfenster bleiben unabhängig davon auf Debug beschränkt. Karten können geöffnet, neu erstellt, gespeichert und erst über `Im Spiel uebernehmen` als aktuelle Karte mit gewählter Spielerposition aktiviert werden. Im Auswahlmodus erscheint für ausgewählte Sprite-Objekte ein Eigenschaftenmenü mit Solid-Checkbox; im Platziermodus legt die Checkbox die Solid-Vorgabe für neue Objekte fest. `Map Eigenschaften` öffnet einen Dialog zur Festlegung von Standardboden (aus dem im Asset-Browser ausgewählten Sprite) und Standardwetter (Auswahl aus `none` und den erkannten Wettereffekten); die separaten Boden- und Wetter-Buttons der Werkzeugleiste entfallen. Die Kartenansicht lässt sich mit WASD oder den Pfeiltasten verschieben; die mittlere Maustaste bleibt als Alternative verfügbar. `Entf` entfernt das ausgewählte Objekt oder das oberste Objekt unter dem Mauszeiger; darunter liegende Objekte bleiben erhalten, ansonsten wird der Standardboden sichtbar, ohne ein neues Bodenobjekt anzulegen.
- Inventar: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Settings: `Esc` öffnet und schließt das Einstellungsmenü. Hoch/Runter wählt VSync, das FPS-Limit, die FPS-Anzeige oder `Zurück zum Hauptmenü`; Links/Rechts, Enter oder Leertaste ändern den Wert. Ein Linksklick auf eine Einstellung ändert sie ebenfalls; beim FPS-Limit wird dabei der nächste Wert gewählt. Enter/Leertaste oder ein Mausklick aktiviert die Rückkehr zum Hauptmenü. Das FPS-Limit bietet 60, 120, 200 und unbegrenzt; standardmäßig ist VSync aus und das Limit unbegrenzt. Bei aktiviertem VSync ist das FPS-Limit ausgesetzt, ausgegraut und weder mit Tastatur noch Maus auswählbar; der gewählte Wert bleibt gespeichert und gilt wieder bei VSync aus. Die FPS-Anzeige ist in beiden Builds standardmäßig an und unabhängig von den Debug-Anzeigen abschaltbar. Änderungen werden dauerhaft gespeichert und beim Start geladen. Das Menü verwendet eine Windows-Systemschrift; falls diese nicht geladen werden kann, wird ein Fehler angezeigt und das Spiel läuft weiter.

### Feedback
- Visuell: Rendering-Pipeline vorgesehen, Detailausgestaltung bewusst offen (Prototyp-Phase)
- Audio: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Haptisch (falls relevant): Nicht relevant für aktuelle Windows-Basis

---

## 9. Art Direction
### Stil
- Pixel Art

### Referenzen
- Referenz 1: Secret of Evermore
- Referenz 2: Secret of Mana

### Farbpalette & Lesbarkeit
Warm/erdig

### Animation
- Spieler: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Gegner: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- VFX: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

---

## 10. Audio Direction
### Musik
- Stil: Atmosphärisch orchestral
- Dynamik (kampf-/explorationsabhängig): Nein, vorerst statisch

### Soundeffekte
- Schritte: Bewusst offen (Prototyp-Phase), erste Priorität
- Treffer: Bewusst offen (Prototyp-Phase), erste Priorität
- UI: Bewusst offen (Prototyp-Phase), nachgelagert

### Audio-Hinweise fürs Gameplay
Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

---

## 11. Technik & Architektur
### Engine/Framework
- SFML 3, C++20 (Custom Game Loop)
- Die unterstützte Build-Plattform ist Windows x64; die vcpkg-Abhängigkeiten verwenden das Triplet `x64-windows`.
- Ein MSBuild-Schritt installiert vor dem Kompilieren die im vcpkg-Manifest angegebenen Abhängigkeiten automatisch in `vcpkg_installed/`. Ein initialisiertes vcpkg und beim ersten Build eine Internetverbindung sind erforderlich; eine globale vcpkg-Integration ist nicht nötig.
- Beim Build werden die zur Konfiguration passenden SFML-Laufzeit-DLLs aus vcpkg neben die ausführbare Datei kopiert; so kann das Spiel direkt aus dem Build-Ausgabeverzeichnis gestartet werden.
- Debug-Builds öffnen beim Start ein Konsolenfenster und leiten `stdout`/`stderr` dorthin um; Meldungen über `printf`/`fprintf` und die synchronisierten C++-Standardstreams werden dort ausgegeben. Zusätzlich protokolliert Debug die Auslösung von Trigger- und Magie-Events sowie Kartenladeversuche, -ergebnisse und Kartenwechsel. Release-Builds bleiben ohne Konsolenfenster und ohne diese Info-Ausgaben.
- Debug-Builds erzeugen PDB-Debugsymbole; Release-Builds erzeugen keine PDB-Datei.

### Ziel-Performance
- FPS-Limit: Standardmäßig unbegrenzt; im Menü sind 60, 120 und 200 FPS wählbar. Bei aktiviertem VSync ist das Software-Limit ausgesetzt und die Bildrate richtet sich nach der Synchronisierung.
- Auflösung: Fenstergröße 960x540
- Hardware-Minimum: Aktueller Windows-PC mit einfacher Grafikkarte

### Speicher-/Ladeverhalten
- Save-System: Bewusst offen (Prototyp-Phase), nach Rendering/Kollision geplant
- Einstellungen: `%LOCALAPPDATA%\MyRPG\settings.json`, JSON-Objekt mit `version` (1), `vsync` (Boolean), `fps_limit` (0 für unbegrenzt, sonst 60, 120 oder 200) und `show_fps` (Boolean). Ältere Dateien ohne `show_fps` verwenden `true`. Bei fehlender Datei gelten die Standardwerte; jede Änderung wird gespeichert. Fehler beim Laden oder Speichern werden angezeigt; eine fehlerhafte oder nicht unterstützte Datei wird nicht überschrieben. Unbekannte Felder bleiben beim Speichern erhalten.
- Ladezeiten: Noch kein Ziel

### Daten-/Dateistruktur (modular)
- Aktuell zentrale Basis in `main.cpp` / `MyRPG.h`
- Zielstruktur: Systeme getrennt nach Modulen (z. B. Combat, Inventory, Dialog)
- Zielstruktur: Inhalte getrennt nach Typen (z. B. Sounds, Sprites, Story-Skripte)
- Asset-Ordner für Sprites: `Assets/Sprites/Characters/` und `Assets/Sprites/Environment/`
- Asset-Ordner für Animationen: `Assets/Animations/Characters/`; Animationen liegen getrennt vom Renderer als `.anim`-Dateien vor
- `.anim`-Format Version 1: `ANIM 1`, `texture <spritePath>`, `frame_duration <seconds>` und eine `frame <x> <y> <width> <height>`-Zeile pro Spritesheet-Ausschnitt. Optional legt `scale <faktor>` einen positiven, endlichen visuellen Skalierungsfaktor fest; ohne Angabe gilt `1.0`. Die Anzahl und Reihenfolge der `frame`-Zeilen bestimmen die Animation; mindestens ein Frame ist erforderlich. Ein separates Anzahlfeld ist nicht nötig.
- Asset-Ordner für Maps: `Assets/Maps/`
- Asset-Ordner für UI-Grafiken: `Assets/UI/`; `title_screen.png` ist ein vorläufiges Titelbild im 1024x768-Basisformat. Schaltflächen werden separat gezeichnet.
- Asset-Ordner für Shader: `Assets/Shaders/`, unterteilt nach Verwendung (`Effects/` für Magie und `Weather/` für Wetter; später z. B. `PostProcess/` für Unschärfe). Shader werden beim Build wie alle Assets ins Ausgabeverzeichnis kopiert.
- Wetter: `Weather.h` verwaltet den aktuellen Zustand, `WeatherRenderer.h` zeichnet Wetter normalerweise als bildschirmfüllendes transparentes Shader-Overlay über der Spielwelt und unter der UI und dem Karten-Fade. Bei `rain` ziehen zusätzlich zu den bildschirmfesten Regentropfen weiche, in Weltkoordinaten verankerte Wolkenschatten über Boden, Objekte und Spielfigur; sie verdunkeln auch Magieeffekte, aber nicht UI oder Karten-Fade. Der Effekt `heat` ist eine Ausnahme: Die gesamte Spielwelt einschließlich Sprites, Spieler und Magieeffekten wird zunächst in eine Szenentextur gezeichnet und anschließend durch den Hitze-Shader verzerrt; UI und Karten-Fade bleiben unverzerrt. Das animierte Verzerrungsmuster ist in Weltkoordinaten verankert und bewegt sich mit der Welt einschließlich Kamera-Shake; die Kamerabewegung beschleunigt seine Animation relativ zur Umgebung nicht. Alle `.frag`-Dateien unter `Assets/Shaders/Weather/` werden beim Start automatisch erkannt; ihr Dateiname ohne Endung ist die Effekt-ID (z. B. `rain.frag` → `rain`). Der gemeinsame Vertex-Shader `weather.vert` und alle gefundenen Fragment-Shader werden nach Erzeugen des SFML-Render-Kontexts vorgeladen. Neue Overlay-Wettereffekte benötigen keinen Engine-Codeeintrag und sind nach dem nächsten Build und Spielstart verfügbar. `none` bleibt für das Ausschalten reserviert. Die Spielzeit steuert die Animation; bei Pause steht sie still. Bei nicht unterstützten oder fehlerhaften Shadern oder fehlgeschlagener Szenentextur wird der Fehler auf stderr gemeldet und der Effekt nicht gezeichnet.
- Magieeffekte: `MagicEffects.h` verwaltet auslöserunabhängig zeitlich begrenzte Effektinstanzen; `MagicRenderer.h` zeichnet sie. `StartMagicEffect` kann auch von zukünftigen Cutscenes oder anderen Systemen aufgerufen werden. Alle Effekte werden automatisch aus `Assets/Shaders/Effects/magic_<name>.frag` erkannt und nach dem Erzeugen des SFML-Render-Kontexts beim Start vorgeladen. Sie verwenden den gemeinsamen Vertex-Shader `magic_effect.vert`, additives Blending und die Uniform `progress`. In `magic_effects.cfg` definiert jede Zeile eine Effekt-ID und positive Quadgröße, gefolgt von optional `screen_shake=0|1` und `mode=world|screen` in beliebiger Reihenfolge. Ohne Option gelten `screen_shake=0` und `mode=world`; nicht aufgeführte Effekte verwenden zusätzlich 224 Pixel. `mode=world` zeichnet das quadratische Quad an der Weltposition; `mode=screen` ignoriert Position und Quadgröße und zeichnet bildschirmfest über den gesamten sichtbaren Spielbereich, unter Wetter, UI und Karten-Fade. Bildschirm-Shader erhalten zusätzlich `resolution` als Größe des Spielbereichs und müssen diese Uniform verwenden; Letterbox-Balken bleiben frei. Die Effekte benötigen keine Textur-Assets und sind keine Gameplay-Zauber. Lässt sich ein Shader nicht laden, wird dieser Effekt nicht gezeichnet und eine Meldung auf stderr ausgegeben. Beim erfolgreichen Kartenwechsel werden aktive Effekte und der Kamera-Shake entfernt.
- Kamera-Shake: Das Map-Event `screen_shake` löst beim Betreten der Triggerzone unabhängig von Magie einen kurzen, abklingenden Shake mit Stärke 8 Pixel und Dauer 0,32 Sekunden aus. Magieeffekte können denselben Shake über `screen_shake=1` in `magic_effects.cfg` auslösen. Nach Verlassen und erneutem Betreten wird er erneut gestartet; während einer Pause steht er still. Timing und Offset liegen auf der CPU und werden im Rendering auf die Spielwelt, das weltverankerte Hitzeverzerrungsmuster und die Wolkenschatten bei Regen angewandt; Regentropfen, andere Wetter-Overlays und UI bleiben bildschirmfest. Die Testzone `screen_shake_test` liegt bei x=1408, y=-32 und ist 64x64 Pixel groß.
- Map-Events `magic_<name>` starten den gleichnamigen, vorhandenen Effekt beim Betreten an der Spielerposition; nach Verlassen und erneutem Betreten kann er erneut auslösen. Ob ein Effekt zusätzlich den Kamera-Shake auslöst, wird pro Effekt über `screen_shake=0|1` in `magic_effects.cfg` festgelegt. Die Testmap enthält die bisherigen 64x64-Magiezonen bei y=-32, ab x=288 im Abstand von 160 Pixeln bis x=1248; die Zone `magic_screen_test` bei x=1568, y=-32 demonstriert den bildschirmfüllenden Modus.
- Sprite-Standardgröße: `64x64` Pixel (einheitliches Basisraster)
- Spieler-Sprite: acht transparente HeroFox-Lauf-Spritesheets und vier Stillstands-Spritesheets in `Assets/Sprites/Characters/`. Die zugehörigen `player_walk_<richtung>.anim`- und `player_idle_<richtung>.anim`-Dateien legen die Frames und ihre Dauer fest.
- Die aktuellen HeroFox-Sheets sind `512x512` Pixel groß und verwenden ein 4x4-Raster mit `128x128` Pixeln pro Frame. Die Laufanimationen der vier geraden Richtungen und die Stillstandsanimationen enthalten je 16 Frames, die diagonalen Laufanimationen jeweils 13; sie werden zeilenweise abgespielt. Der gesamte Frame wird auf die Spielergröße (standardmäßig 64 Pixel Höhe) skaliert und anschließend mit dem `scale`-Faktor der Animation multipliziert. Die Skalierung ändert die Kollisionsgröße nicht.
- Bei Stillstand wird die zugehörige Stillstandsanimation der letzten Blickrichtung abgespielt; für diagonale Blickrichtungen wird die linke bzw. rechte Stillstandsanimation verwendet. Gegenläufige Eingaben heben sich auf; bei Kollisionen bestimmt die tatsächliche Bewegung die Laufrichtung.
- Map-Format (Prototyp): textbasiert, eine Datei pro Map (z. B. `0001_TestMap.map`)
- Zeilenformat: `MAP`, `GROUND`, `WEATHER`, `OBJECT`, `TRIGGER`; `GROUND <spritePath>` füllt den sichtbaren Kartenbereich gekachelt als Standardboden
- Map-Editor: Bei eingebautem Editor ist der EXE-Ordner das Arbeitsverzeichnis; Karten werden ausschließlich in dessen `Assets/Maps` bearbeitet. Vor dem ersten Überschreiben einer bestehenden Karte wird `<dateiname>.map.original.bak` angelegt und nicht automatisch gelöscht oder ersetzt. Build und Bereinigung können Arbeitskopien und Sicherungen gefährden; die Übernahme ins Projekt erfolgt manuell. Sprite-PNGs werden rekursiv aus `Assets/Sprites` als aufklappbarer Ordnerbaum aufgelistet; `Environment` und seine Unterordner sind zunächst geöffnet. Eine Ausschlussliste blendet `Assets/Sprites/Characters` samt Unterordnern im Editor aus; andere und neu hinzukommende Ordner werden weiterhin automatisch erkannt. Die Asset-Dateien selbst bleiben unverändert und für das Spiel verfügbar. Die Sheet-Ansicht (Button "Sheet-Ansicht (G)") öffnet ein eigenes, verschiebbares Fenster mit dem gewählten Sprite in Rasteransicht; die Rasterzelle (Standard 16 px, einstellbar per -/+ und Voreinstellungen 8/16/32/64) wird auf 64 px Spielgröße skaliert. Ein Klick wählt ein Tile als Pinsel, Ziehen aus dem Sheet-Fenster auf die Karte im Editorfenster setzt es. Im Sheet-Fenster bewegen WASD/Pfeiltasten und die mittlere Maustaste die Ansicht (wie bei der Karte), das Mausrad zoomt um den Mauszeiger. Ein Sprite ohne gewähltes Tile wird als vollständiges Bild behandelt. Ein Sprite wird im Browser angeklickt und hängt danach als halbtransparente Vorschau am Mauszeiger; ein Klick platziert ein Exemplar, Ziehen mit gedrückter linker Maustaste kann mehrere Exemplare entlang des Weges setzen. Kartengrenzen sind nicht festgelegt. Im Trigger-Modus können Zonen erstellt, verschoben, durch Ziehen ihrer Seiten und Ecken in der Größe verändert und gelöscht sowie Glow und ein Ereignis zugewiesen werden. Die gegenüberliegende Seite bleibt beim Vergrößern oder Verkleinern fest; die Mindestgröße beträgt eine Welteinheit pro Richtung. Mit gedrückter Umschalttaste kann die ausgewählte Zone auch am Rand verschoben werden. Das Eigenschaftenmenü bietet getrennte scrollbare Dropdowns für Kartenwechsel aus `Assets/Maps` und vorhandene Magie-, Wetter- und Screen-Shake-Ereignisse sowie eine Glow-Checkbox mit Erklärung. Eine Auswahl ersetzt das bisherige Ereignis; vorhandene Spawn- und weitere Triggeroptionen bleiben erhalten. Die Zielposition eines Kartenwechsels ist sichtbar, aber noch nicht editierbar; ohne vorhandene Spawn-Angaben gilt `(0, 0)`.
- `WEATHER none|<shaderName>` legt optional das Standardwetter fest; `<shaderName>` entspricht dem Dateinamen ohne `.frag` eines automatisch erkannten Wetter-Shaders. Ohne Angabe gilt `none`. Ungültige oder mehrfache Angaben verhindern das Laden. Beim erfolgreichen Kartenwechsel gilt das Standardwetter der Zielkarte; bei fehlgeschlagenem Wechsel bleibt das aktuelle Wetter erhalten. `weather_<shaderName>` schaltet beim Betreten einer Triggerzone auf den gleichnamigen Effekt, `weather_none` schaltet ihn aus. Wetter-Shader werden aus `Assets/Shaders/Weather/` beim Start erkannt. Die Testkarte hat drei Zonen südlich des Startpunkts; die Waldkarte startet mit Nebel.
- `OBJECT <id> <type> <x> <y> <spritePath> solid=<0|1> [tile=<x>,<y>,<breite>,<höhe>]`: `solid=1` blockiert den Spieler (Kollision), `solid=0` nicht. Flags werden ausschließlich als `key=value` geschrieben. Das optionale `tile=` wählt einen Ausschnitt (Texturpixel) aus einem Spritesheet; er wird wie ein ganzes Sprite auf 64x64 Welteinheiten skaliert. Ohne `tile=` gilt die ganze Textur.
- `TRIGGER <id> <x> <y> <width> <height> <eventName> [glow=0|1] [spawn_x=<x> spawn_y=<y>]`: `none` als Ereignisname löst beim Betreten nichts aus. In beiden Editor-Dropdowns entfernt „Kein Ereignis“ das gesamte bisherige Ereignis, da pro Trigger höchstens eines gespeichert wird; Glow und weitere Optionen bleiben erhalten. `weather_none` schaltet dagegen das Wetter aus. Mit `glow=1` leuchtet die Zone sichtbar (pulsierender Glow-Fragment-Shader, additiv, auch im Release-Build; ohne Shader-Unterstützung Fallback auf ein einfaches Rechteck). Standard ist 0.
- Kartenwechsel: `Load_<dateiname>` lädt `Assets/Maps/<dateiname>.map` ohne Registrierung im Code. Der Dateiname ohne Endung darf keine Leerzeichen oder Pfadtrennzeichen enthalten. Die vorhandene `MAP`-ID muss nicht dem Dateinamen entsprechen. Die optionale Zielposition wird pro Trigger durch beide Angaben `spawn_x` und `spawn_y` festgelegt; ohne Angaben gilt `(0, 0)`. Bei fehlender oder ungültiger Zielkarte bleibt die aktuelle Karte aktiv und der Fehler wird auf stderr gemeldet. Die Startkarte bleibt separat in der Engine festgelegt.
- Abweichende Bodenkacheln werden als `OBJECT` vom Typ `untergrund` eingetragen; weitere Map-Objekte und Trigger bleiben separat
- Aktuelle Platzhalter: `basic_rock.png` und `basic_grass.png`
- Map-Objektkategorie `untergrund`: Bodenflächen ohne Kollision (z. B. Gras)

---

## 12. Produktion & Scope
### Must / Should / Could
- **Must:** `RenderGame` implementieren, Kollisionsdetektion ergänzen, Save-System hinzufügen
- **Should:** Gegner/Hindernisse integrieren
- **Could:** Erweiterte Systeme (Inventar, Fähigkeiten, Story-Content)

### Meilensteine
- Aktueller Stand: Prototyp
- Weitere Meilensteine und Inhalte werden auf dem Entwicklungsweg ergänzt

### Risiken
- Technik: Rendering noch nicht umgesetzt, Architektur wächst aktuell aus zentralen Dateien
- Zeit: Scope kann schnell wachsen, wenn mehrere Systeme parallel starten
- Content: Narrative/Art/Audio sind noch offen

---

## 13. QA & Erfolgskriterien
### Testfokus
- Gameplay: Bewegungsverhalten inkl. diagonaler Normalisierung
- Bugs: Input-Verarbeitung und Timer-Loop
- Performance: Stabilität bei ~60 FPS
- UX: Direktes Input-Feedback

### KPI/Erfolgskriterien
- Flüssige Steuerung
- Stabiler Loop ohne Frame-abhängige Bewegung
- Korrekte Input-Reaktion (WASD + Pfeiltasten)
- Keine Richtungsbevorzugung bei diagonalem Input

---

## 14. Entscheidungslog (Decision Log)
| Datum | Entscheidung | Grund | Auswirkung |
|------|---------------|-------|------------|
| 10.11.2026 | Win32 + C++17 als Basis | Schlankes, kontrolliertes Grundgerüst | Direkte Kontrolle über Loop/Event-Handling |
| 10.11.2026 | Delta-Time-basierte Bewegung | Framerate-unabhängiges Verhalten | Konsistente Bewegung auf verschiedenen Systemen |
| 10.11.2026 | Modulare Zielstruktur für Systeme/Assets | Bessere Erweiterbarkeit und Wartbarkeit | Spätere Aufteilung in getrennte Engine-/Content-Module |
| 10.11.2026 | Map-Dateien pro Level (`Assets/Maps/*.map`) mit `OBJECT`- und `TRIGGER`-Einträgen | Weltinhalte und Trigger sollen datengetrieben statt hartkodiert sein | Testmap `0001_TestMap` kann Objekte platzieren und Bereichs-Events auslösen |
| 10.11.2026 | Map-Direktive `GROUND` für gekachelten Standardboden; abweichende Kacheln bleiben `OBJECT untergrund` | Wiederholte Bodeneinträge sparen und Kartenflächen automatisch füllen | Testmap und Forest-Map definieren je eine Standard-Bodentextur |
| 10.11.2026 | Neue Objektkategorie `untergrund` für Boden-Sprites (z. B. `basic_grass.png`) | Bodenflächen sollen als eigene Layer-Kategorie ohne Kollision pflegbar sein | Maps können kleinen Grasboden-Bereich datengetrieben definieren (`solid=0`) |
| 10.11.2026 | Forest-Biome als eigener Asset-Unterordner (`Assets/Sprites/Environment/Forest/`) mit 64x64-Sprites | Modulare Erweiterbarkeit und klare Trennung pro Content-Thema | Neue Waldkarte kann eigenständig mit eigener Sprite-Familie gestaltet und gepflegt werden |
| 10.11.2026 | Trigger-Events `Load_*` starten Mapwechsel mit kurzem Fade-to-Black | Kartenübergänge sollen klar lesbar und visuell weicher sein | Startmap kann per Trigger zur Forest-Map wechseln; Rendering zeichnet dynamisches Dunkel-Overlay während Transition |
| 11.11.2026 | Spieler-Spritesheet `jump_man.png` mit 23 Animationsposen | Die Hauptfigur erhält eine vollständige Bewegungsanimation; das Schachbrett des Sheets dient nicht als Spielhintergrund | Das Rendering entfernt den grauen Hintergrund, schneidet die Posen aus und spielt sie bei Bewegung pixelgenau in einer Schleife ab |
| 11.11.2026 | Animationsdaten in versionierten `.anim`-Dateien statt im Renderer | Frames und Wiedergabeparameter sollen unabhängig von Engine-Code erweiterbar sein | `player_walk.anim` liefert Sheet-Pfad, Framedauer und Frame-Rechtecke an den Renderer |
| 11.11.2026 | Schachbretthintergrund aus `jump_man.png` entfernt und als PNG-Transparenz gespeichert | Sprite-Assets sollen ohne Laufzeit-Farbfilter direkt verwendbar sein | Der Renderer lädt die Textur mit transparentem Hintergrund direkt aus der PNG |
| 13.11.2026 | HeroFox-Lauf-Spritesheets auf 4x4 Frames à `64x64` Pixel normiert und Figurengrößen angeglichen | Einheitliches Basisraster und konsistente Figurengröße in allen Laufrichtungen | Die drei HeroFox-Sheets verwenden direkt passende 64x64-Framebereiche |
| 13.11.2026 | Acht HeroFox-Lauf-Sheets ersetzen `jump_man.png` als Spieleranimation | Bewegungen sollen in allen acht Richtungen dargestellt werden | Separate Animationsdateien nutzen die tatsächlichen 128x128-Framebereiche der vorhandenen Sheets |
| 13.11.2026 | Sieben Magieeffekte als GPU-Shader unter `Assets/Shaders/Effects/`, je eine Testzone auf der Testmap | Grundlage für erweiterbare GPU-Effekte (später Wetter, Unschärfe) in eigenem Shader-Ordner | Effektlogik/Timing bleibt auf der CPU, Darstellung läuft im Fragment-Shader |
| 15.11.2026 | Lightning löst einen kurzen Kamera-Shake aus | Ein sichtbarer Impakt-Test für kamerabezogene Effekte ohne Shader-Zuständigkeit für die Kamera | Zeit und Offset werden auf der CPU aktualisiert; nur die Spielwelt wackelt, UI und Wetter-Overlay bleiben fest |

---

## 15. Offene Fragen
1. Spielperspektive: Top-Down (2D) (festgelegt)
2. Kampfmodell: Echtzeit (festgelegt)
3. Welche Mindest-Hardware soll offiziell unterstützt werden?

---

## 16. Änderungsverlauf
| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| v0.1    |       | Initiale Vorlage | |
| v0.2    | 10.11.2026 | Bekannte Projektinfos aus README und DesignDocument vorbefüllt | Ihr Name |
| v0.3    | 10.11.2026 | Prototyp von Win32/GDI auf SFML-Loop, Input und Rendering migriert | Ihr Name |
| v0.4    | 10.11.2026 | Asset-Struktur für Sprites (`Characters`/`Environment`) und 64x64-Standard dokumentiert | Ihr Name |
| v0.5    | 10.11.2026 | Testmap-Dateiformat (`Assets/Maps/0001_TestMap.map`) und Triggerzonen-Ansatz dokumentiert | Ihr Name |
| v0.6    | 10.11.2026 | Neues Environment-Sprite `basic_grass.png` und Map-Kategorie `untergrund` für kollisionslosen Boden ergänzt | Ihr Name |
| v0.7    | 10.11.2026 | Neue große Zusatzkarte `0001_Forest.map` (ca. 40x40 Tiles) plus Forest-Sprite-Familie (`ground/path/tree/bush/rock/stump/flower`) ergänzt | Ihr Name |
| v0.8    | 10.11.2026 | Trigger-gesteuerter Kartenwechsel zwischen `0001_TestMap` und `0001_Forest` mit kurzem Dunkel-Übergang integriert | Ihr Name |
| v0.9    | 11.11.2026 | Spieler-Sprite `main_character.png` als transparentes Pixel-Art im 64x64-Raster überarbeitet | Ihr Name |
| v1.0    | 11.11.2026 | Animiertes Spieler-Spritesheet `jump_man.png` mit 23 Posen eingebunden; Schachbretthintergrund wird beim Laden entfernt | Ihr Name |
| v1.1    | 11.11.2026 | Spieler-Frame-Koordinaten und Framedauer in `Assets/Animations/Characters/player_walk.anim` ausgelagert | Ihr Name |
| v1.2    | 11.11.2026 | Schachbretthintergrund direkt in `jump_man.png` transparent gemacht; Laufzeit-Farbfilter entfernt | Ihr Name |
| v1.3    | 13.11.2026 | HeroFox-Lauf-Spritesheets auf einheitliche 64x64-Frames und Figurengröße angepasst | Ihr Name |

---

## 17. Migrationsnotiz (Win32/GDI -> SFML)
- Eingabe, Update und Rendering laufen jetzt in einem SFML-Hauptloop.
- Engine-Daten/Logik (`GameTypes`, `GameEngine`) wurden von Win32-Typen entkoppelt.
- Raster- und Player-Rendering wurden auf SFML (`VertexArray`, `RectangleShape`) portiert.

### Nächste sinnvolle Schritte
1. Animations-Spritesheet für den Spieler ergänzen.
2. Kamera/Viewport als eigenes Modul unter `src/engine` auslagern.
3. Erstes Asset- und Content-Verzeichnis für Texturen/Sounds anlegen.
