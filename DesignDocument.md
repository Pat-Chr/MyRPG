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
- Nur im Debug-Build: Karten-ID, Spielerkoordinaten, FPS und letztes Trigger-Ereignis; zusätzlich werden Trigger-Bereiche und das Debug-Raster sichtbar dargestellt. Im Release-Build entfallen diese Debug-Anzeigen, die Trigger-Funktion bleibt erhalten.
- Diese Trennung ist eine dauerhafte Vorgabe und soll bei zukünftigen Änderungen beibehalten werden: Debug-Informationen und Trigger-Markierungen sind nur in Debug-Builds sichtbar; Release-Builds bleiben frei von diesen Anzeigen.

### Menüs
- Hauptmenü: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Pause: Das Spiel pausiert, solange das Einstellungsmenü geöffnet ist; Bewegung, Trigger, Effekte und Kartenwechsel laufen in dieser Zeit nicht weiter.
- Inventar: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Settings: `Esc` öffnet und schließt ein einfaches Tastaturmenü. Hoch/Runter wählt VSync oder das FPS-Limit; Links/Rechts, Enter oder Leertaste ändern den Wert. Das FPS-Limit bietet 60, 120, 200 und unbegrenzt; standardmäßig ist VSync aus und das Limit unbegrenzt. Bei aktiviertem VSync ist das FPS-Limit ausgesetzt, ausgegraut und nicht auswählbar; der gewählte Wert bleibt gespeichert und gilt wieder bei VSync aus. Die Einstellungen gelten nur für die aktuelle Sitzung. Das Menü verwendet eine Windows-Systemschrift; falls diese nicht geladen werden kann, wird ein Fehler angezeigt und das Spiel läuft weiter.

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
- Beim Build werden die zur Konfiguration passenden SFML-Laufzeit-DLLs aus vcpkg neben die ausführbare Datei kopiert; so kann das Spiel direkt aus dem Build-Ausgabeverzeichnis gestartet werden.

### Ziel-Performance
- FPS-Limit: Standardmäßig unbegrenzt; im Menü sind 60, 120 und 200 FPS wählbar. Bei aktiviertem VSync ist das Software-Limit ausgesetzt und die Bildrate richtet sich nach der Synchronisierung.
- Auflösung: Fenstergröße 960x540
- Hardware-Minimum: Aktueller Windows-PC mit einfacher Grafikkarte

### Speicher-/Ladeverhalten
- Save-System: Bewusst offen (Prototyp-Phase), nach Rendering/Kollision geplant
- Ladezeiten: Noch kein Ziel

### Daten-/Dateistruktur (modular)
- Aktuell zentrale Basis in `main.cpp` / `MyRPG.h`
- Zielstruktur: Systeme getrennt nach Modulen (z. B. Combat, Inventory, Dialog)
- Zielstruktur: Inhalte getrennt nach Typen (z. B. Sounds, Sprites, Story-Skripte)
- Asset-Ordner für Sprites: `Assets/Sprites/Characters/` und `Assets/Sprites/Environment/`
- Asset-Ordner für Animationen: `Assets/Animations/Characters/`; Animationen liegen getrennt vom Renderer als `.anim`-Dateien vor
- `.anim`-Format Version 1: `ANIM 1`, `texture <spritePath>`, `frame_duration <seconds>` und eine `frame <x> <y> <width> <height>`-Zeile pro Spritesheet-Ausschnitt. Die Anzahl und Reihenfolge der `frame`-Zeilen bestimmen die Animation; mindestens ein Frame ist erforderlich. Ein separates Anzahlfeld ist nicht nötig.
- Asset-Ordner für Maps: `Assets/Maps/`
- Asset-Ordner für Shader: `Assets/Shaders/`, unterteilt nach Verwendung (aktuell `Effects/`; später z. B. `PostProcess/` für Wetter, Unschärfe). Shader werden beim Build wie alle Assets ins Ausgabeverzeichnis kopiert.
- Magieeffekte: `MagicEffects.h` verwaltet auslöserunabhängig zeitlich begrenzte Effektinstanzen in Weltkoordinaten; `MagicRenderer.h` zeichnet sie. `StartMagicEffect` kann auch von zukünftigen Cutscenes oder anderen Systemen aufgerufen werden. Alle Effekte werden automatisch aus `Assets/Shaders/Effects/magic_<name>.frag` erkannt und nach dem Erzeugen des SFML-Render-Kontexts beim Start vorgeladen; damit wird das Laden/Kompilieren nicht erst beim ersten Auslösen während des Spiels angestoßen. Sie werden mit dem gemeinsamen Vertex-Shader `magic_effect.vert` auf einem Quad mit additivem Blending berechnet; die CPU liefert nur Position und Fortschritt (`progress` als Uniform). Eine neue Effektdatei plus ein Map-Event `magic_<name>` genügen, ein Neu-Compile oder eine Engine-Zuordnung ist nicht nötig. `magic_effects.cfg` enthält optionale Quadgrößen; nicht aufgeführte Effekte verwenden 224 Pixel. Die Effekte benötigen keine Textur-Assets und sind keine Gameplay-Zauber. Lässt sich ein Shader nicht laden, wird dieser Effekt nicht gezeichnet und eine Meldung auf stderr ausgegeben. Beim erfolgreichen Kartenwechsel werden aktive Effekte entfernt.
- Map-Events `magic_<name>` starten den gleichnamigen, vorhandenen Effekt beim Betreten an der Spielerposition; nach Verlassen und erneutem Betreten kann er erneut auslösen. Die Testmap enthält je eine 64x64-Zone bei y=-32, ab x=288 im Abstand von 160 Pixeln nach rechts (288, 448, 608, 768, 928, 1088, 1248).
- Sprite-Standardgröße: `64x64` Pixel (einheitliches Basisraster)
- Spieler-Sprite: acht transparente HeroFox-Lauf-Spritesheets in `Assets/Sprites/Characters/`, je eines pro Bildschirmrichtung. Die zugehörigen `player_walk_<richtung>.anim`-Dateien legen die Frames und ihre Dauer fest.
- Die aktuellen HeroFox-Sheets sind `512x512` Pixel groß und verwenden ein 4x4-Raster mit `128x128` Pixeln pro Frame. Die vier geraden Richtungen enthalten 16 Frames, die vier diagonalen jeweils 13; sie werden zeilenweise abgespielt und auf 64 Pixel Höhe skaliert.
- Bei Stillstand bleibt der erste Frame der letzten Blickrichtung sichtbar. Gegenläufige Eingaben heben sich auf; bei Kollisionen bestimmt die tatsächliche Bewegung die Laufrichtung.
- Map-Format (Prototyp): textbasiert, eine Datei pro Map (z. B. `0001_TestMap.map`)
- Zeilenformat: `MAP`, `GROUND`, `OBJECT`, `TRIGGER`; `GROUND <spritePath>` füllt den sichtbaren Kartenbereich gekachelt als Standardboden
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
