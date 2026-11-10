# Game Design Document (GDD) – Ausfüllbare Vorlage

## 0. Dokument-Info
- **Projektname:** MyRPG
- **Version:** v0.2
- **Stand:** 10.11.2026
- **Autor(en):** Ihr Name
- **Status:** Prototyp

---

## 1. Vision & Elevator Pitch
### Kurzbeschreibung (1–3 Sätze)
MyRPG ist ein minimalistisches RPG-Spiel für Windows, geschrieben in C++ mit Win32 API. Der aktuelle Fokus liegt auf einer stabilen Game-Loop- und Input-Basis als Fundament für spätere Gameplay-Systeme.

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
1. Eingabe lesen (WASD/Pfeiltasten)
2. Spielzustand mit Delta-Time aktualisieren (`UpdateGame`)
3. Redraw anfordern (`InvalidateRect`)
4. Render-Pfad in `WM_PAINT` ausführen (`RenderGame`, aktuell TODO)

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
- Tastatur/Maus: WASD + Pfeiltasten für Bewegung
- Controller: Später geplant

### HUD
- Muss anzeigen: Health, Ausdauer, XP, Level
- Optional: Weitere Elemente kommen später hinzu

### Menüs
- Hauptmenü: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Pause: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Inventar: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf
- Settings: Bewusst offen (Prototyp-Phase), Ergänzung im Entwicklungsverlauf

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
- Win32 API, C++17 (Custom Game Loop)

### Ziel-Performance
- FPS-Ziel: ~60 FPS (`TARGET_FRAME_MS = 16`)
- Auflösung: Fenstergröße 960x540
- Hardware-Minimum: Aktueller Windows-PC mit einfacher Grafikkarte

### Speicher-/Ladeverhalten
- Save-System: Bewusst offen (Prototyp-Phase), nach Rendering/Kollision geplant
- Ladezeiten: Noch kein Ziel

### Daten-/Dateistruktur (modular)
- Aktuell zentrale Basis in `main.cpp` / `MyRPG.h`
- Zielstruktur: Systeme getrennt nach Modulen (z. B. Combat, Inventory, Dialog)
- Zielstruktur: Inhalte getrennt nach Typen (z. B. Sounds, Sprites, Story-Skripte)

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
