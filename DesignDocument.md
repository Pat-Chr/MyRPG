# Game Design Document (GDD) – Ausfüllbare Vorlage

## 0. Dokument-Info
- **Projektname:** MyRPG
- **Version:** v0.2
- **Stand:** TBD
- **Autor(en):** TBD
- **Status:** In Arbeit

---

## 1. Vision & Elevator Pitch
### Kurzbeschreibung (1–3 Sätze)
MyRPG ist ein minimalistisches RPG-Spiel für Windows, geschrieben in C++ mit Win32 API. Der aktuelle Fokus liegt auf einer stabilen Game-Loop- und Input-Basis als Fundament für spätere Gameplay-Systeme.

### Genre & Perspektive
- **Genre:** RPG (aktuell als Framework/Grundgerüst)
- **Perspektive (z. B. Top-Down, 2D-Side, First Person):** TBD

### Zielgruppe
- **Alter:** TBD
- **Spielertyp:** Spieler mit Interesse an klassischem RPG-Gameplay
- **Erfahrung (Casual/Hardcore):** TBD

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
TBD (System noch nicht implementiert)

---

## 3. Spielerfantasie & Spielgefühl
### Spielerrolle
Steuerung eines einzelnen Spieler-Avatars im Fensterraum.

### Gewünschtes Gefühl
- Spannung: TBD
- Entdeckung: TBD
- Machtfantasie: TBD
- Risiko/Belohnung: TBD

---

## 4. Kernsysteme
### 4.1 Bewegung
- Laufgeschwindigkeit: 220 px/s (`Player.speed`)
- Sprint/Ausweichen: Noch nicht implementiert
- Kollision: Noch nicht implementiert (Fensterrand-Kollision als geplanter nächster Schritt)

### 4.2 Kampf
- Echtzeit / Rundenbasiert: TBD
- Primärwaffen: Noch nicht implementiert
- Sekundärfähigkeiten: Noch nicht implementiert
- Schadenstypen: Noch nicht implementiert
- Trefferfeedback: Noch nicht implementiert

### 4.3 Ressourcen
- Gesundheit: Noch nicht implementiert
- Mana/Stamina: Noch nicht implementiert
- Währung: Noch nicht implementiert
- Verbrauchsgegenstände: Noch nicht implementiert

### 4.4 Progression
- Levelsystem: Noch nicht implementiert
- Skilltree: Noch nicht implementiert
- Ausrüstung: Noch nicht implementiert
- Freischaltungen: Noch nicht implementiert

### 4.5 Gegner/KI
- Gegnertypen: Noch nicht implementiert
- Verhaltensmuster: Noch nicht implementiert
- Boss-Design: Noch nicht implementiert

---

## 5. Regeln, Sieg/Niederlage, Balancing
### Siegbedingungen
Noch nicht definiert

### Niederlagebedingungen
Noch nicht definiert

### Schwierigkeit
- Modus(e): TBD
- Skalierung: TBD

### Balancing-Prinzipien
- Delta-Time-basierte Bewegung für konsistentes Verhalten über verschiedene Framerates
- Normalisierung diagonaler Bewegung, damit keine Richtungs-Vorteile entstehen

---

## 6. Welt-, Level- und Content-Design
### Weltstruktur
- TBD (derzeit technischer Prototyp ohne Welt-Content)

### Biome/Regionen
1. TBD
2. TBD
3. TBD

### Level-Pacing
- Intro: Bewegungs- und Input-Basis steht
- Midgame: TBD
- Endgame: TBD

### Checkpoints/Respawn
Noch nicht implementiert

### Quests/Inhalte
- Hauptquests: Noch nicht implementiert
- Nebenquests: Noch nicht implementiert
- Zufallsereignisse: Noch nicht implementiert

---

## 7. Narrative Design
### Prämisse
Noch nicht definiert

### Hauptfiguren
- Name: Spielercharakter (TBD)
- Rolle: Steuerbare Hauptfigur
- Motivation: TBD

### Story-Struktur (3 Akte oder Kapitel)
- Akt/Kapitel 1: TBD
- Akt/Kapitel 2: TBD
- Akt/Kapitel 3: TBD

### Dialog- und Tonalitätsregeln
Noch nicht definiert

---

## 8. UX, UI und Steuerung
### Eingabe
- Tastatur/Maus: WASD + Pfeiltasten für Bewegung
- Controller: Noch nicht implementiert

### HUD
- Muss anzeigen: TBD
- Optional: TBD

### Menüs
- Hauptmenü: Noch nicht implementiert
- Pause: Noch nicht implementiert
- Inventar: Noch nicht implementiert
- Settings: Noch nicht implementiert

### Feedback
- Visuell: Rendering-Pipeline vorgesehen, Detailausgestaltung TBD
- Audio: Noch nicht implementiert
- Haptisch (falls relevant): Nicht relevant für aktuelle Windows-Basis

---

## 9. Art Direction
### Stil
- TBD

### Referenzen
- Referenz 1: TBD
- Referenz 2: TBD

### Farbpalette & Lesbarkeit
TBD

### Animation
- Spieler: Noch nicht implementiert
- Gegner: Noch nicht implementiert
- VFX: Noch nicht implementiert

---

## 10. Audio Direction
### Musik
- Stil: TBD
- Dynamik (kampf-/explorationsabhängig): TBD

### Soundeffekte
- Schritte: Noch nicht implementiert
- Treffer: Noch nicht implementiert
- UI: Noch nicht implementiert

### Audio-Hinweise fürs Gameplay
Noch nicht implementiert

---

## 11. Technik & Architektur
### Engine/Framework
- Win32 API, C++17 (Custom Game Loop)

### Ziel-Performance
- FPS-Ziel: ~60 FPS (`TARGET_FRAME_MS = 16`)
- Auflösung: Fenstergröße 960x540
- Hardware-Minimum: TBD

### Speicher-/Ladeverhalten
- Save-System: Noch nicht implementiert (als nächster Schritt vorgesehen)
- Ladezeiten: TBD

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
- Prototyp: Basis-Loop + Input + Bewegung (erreicht)
- Vertical Slice: Rendern + Kollision + einfache Interaktion (TBD)
- Alpha: Kernsysteme spielbar (TBD)
- Beta: Content-Ausbau + Balancing (TBD)
- Release: TBD

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
- UX: Reaktionsgefühl der Steuerung

### KPI/Erfolgskriterien
- Stabiler Loop ohne Frame-abhängige Bewegung
- Korrekte Input-Reaktion (WASD + Pfeiltasten)
- Keine Richtungsbevorzugung bei diagonalem Input

---

## 14. Entscheidungslog (Decision Log)
| Datum | Entscheidung | Grund | Auswirkung |
|------|---------------|-------|------------|
| TBD  | Win32 + C++17 als Basis | Schlankes, kontrolliertes Grundgerüst | Direkte Kontrolle über Loop/Event-Handling |
| TBD  | Delta-Time-basierte Bewegung | Framerate-unabhängiges Verhalten | Konsistente Bewegung auf verschiedenen Systemen |
| TBD  | Modulare Zielstruktur für Systeme/Assets | Bessere Erweiterbarkeit und Wartbarkeit | Spätere Aufteilung in getrennte Engine-/Content-Module |

---

## 15. Offene Fragen
1. Welche Spielperspektive wird final genutzt (Top-Down, Side-View, etc.)?
2. Welches Kampfmodell wird gewählt (Echtzeit vs. rundenbasiert)?
3. Welche Mindest-Hardware soll offiziell unterstützt werden?

---

## 16. Änderungsverlauf
| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| v0.1    |       | Initiale Vorlage | |
| v0.2    | TBD   | Bekannte Projektinfos aus README und DesignDocument vorbefüllt | Copilot |
