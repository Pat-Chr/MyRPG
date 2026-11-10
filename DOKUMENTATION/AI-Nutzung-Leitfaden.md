# 🤖 AI-Assistenten (Copilot, GitHub Copilot, etc.) - Nutzungsguide für MyRPG

Dieser Leitfaden zeigt, wie du AI-Tools effektiv mit dem Design-Dokument verwendest, um konsistenten und architekturkonformen Code zu generieren.

## 🎯 Grundprinzip

**Regel:** Alle AI-generierten Änderungen müssen sich an `DesignDocument.md` halten.

---

## 📋 Standard-Prozedur für AI-Entwicklungen

### Schritt 1: Design-Dokument öffnen
```
1. Öffne Visual Studio
2. Öffne DesignDocument.md (im Root-Ordner)
3. Navigiere zur relevanten Sektion (z.B. "UpdateGame", "GameState")
4. Achte darauf, dass das Dokument im aktiven Tab ist
```

### Schritt 2: Kontext für AI-Assistent bereitstellen

**Schlechtes Prompt:**
> "Füge Kollisionsdetektion hinzu"

**Gutes Prompt (mit Design-Dokument-Kontext):**
> "Basierend auf der Architektur in DesignDocument.md Sektion 'Erweiterungsmöglichkeiten' - Punkt 1 (Kollisionsdetektion mit Fensterrand):
> 
> - Implementiere Kollisionsprüfung gegen Fensterbegrenzung in UpdateGame()
> - Behalte die existing Datenstrukturen (GameState, Player) bei
> - Nutze clientRect für Grenzwert-Prüfung wie im Design dokumentiert
> - Füge nach der Positionsabfrage den Fensterrand-Check hinzu"

### Schritt 3: Code generieren und prüfen

1. AI-Candidate erhält den Kontext aus dem offenen Design-Dokument
2. Prüfe, dass die generierten Änderungen konsistent mit DesignDocument.md sind
3. Verifiziere Datenstrukturen (GameState, Player, InputState) bleiben unverändert
4. Stelle sicher, dass neue Code-Teile in "Erweiterungsmöglichkeiten" dokumentiert werden

---

## 🎮 Spezifische Szenarien für AI-Nutzung

### Scenario 1: UpdateGame() erweitern

**Prompt:**
```
Basierend auf DesignDocument.md Sektion "UpdateGame":
- Die aktuelle Implementierung bewegt nur den Spieler
- Ich möchte Kollisionsdetektion hinzufügen (siehe "Erweiterungsmöglichkeiten" Punkt 1)
- Verwende clientRect für Fensterbegrenzung
- Behalte die Normalisierung bei diagonalen Bewegungen bei

Hier ist der aktuelle UpdateGame Code:
[Paste current code]

Bitte erweitere ihn gemäß DesignDocument.md.
```

### Scenario 2: Neue Datenstruktur hinzufügen

**Prompt:**
```
DesignDocument.md dokumentiert folgende Datenstrukturen:
- GameState (globale Spielvariable)
- Player (Spieler-Position und Eigenschaften)
- InputState (Tastatur-Eingaben)

Ich möchte eine neue Struktur 'GameObject' für Hindernisse/Efegegn hinzufügen.
Bitte:
1. Stelle die Struktur so auf, dass sie zum bestehenden Design passt
2. Dokumentiere sie im DesignDocument.md unter "Erweiterungsmöglichkeiten" Punkt 2
3. Erstelle Initialisierungscode in InitializeGameState()
4. Füge Kollisionsprüfung in UpdateGame() hinzu

Hier ist der aktuelle GameState:
[Paste current code]
```

### Scenario 3: Rendering implementieren

**Prompt:**
```
DesignDocument.md Sektion "Rendering" beschreibt die WM_PAINT Handhabung:
- BeginPaint → RenderGame → EndPaint
- clientRect wird an RenderGame übergeben
- g_game als Parameter erhält

Aktueller WM_PAINT Handler:
[Paste current code]

Bitte implementiere RenderGame gemäß diesem Design, beginnend mit:
1. Grundlegendes Rendering des Spielers
2. Optional: Hintergrund oder UI Elemente
```

---

## ⚠️ Wichtige Hinweise für AI-Assistenten

### ❌ Vermeide diese Fehler:

1. **Datenstrukturen ändern ohne Dokumentation**
   - AI darf `GameState` nicht ohne Grund erweitern
   - Jede Änderung muss in DesignDocument.md erklärt werden

2. **Architektur ignorieren**
   - Nicht einfach neue Funktionen hinzufügen, die den bestehenden Flow stören
   - Halte dich an den im Design beschriebenen Game Loop

3. **Delta-Time vergessen**
   - AI sollte immer deltaTime verwenden (wie im Design)
   - Keine frame-rate-abhängige Logik ohne explizite Erlaubnis

4. **Normalisierung ignorieren**
   - Bei diagonalen Bewegungen: Geschwindigkeit normalisieren
   - AI muss die sqrt(x² + y²) Berechnung einbeziehen

### ✅ Richtiges Vorgehen:

1. **Design-Dokument immer im Kontext haben**
2. **AI-Befehle mit "gemäß DesignDocument.md" präzisieren**
3. **Generierten Code gegen Design-Regeln prüfen**
4. **Änderungen in Dokumentationen aufnehmen**

---

## 📝 Beispiel: Kompletter Workflow

### Aufgabe: Diagonale Bewegung verbessern

1. **Vorbereitung:**
   ```
   - DesignDocument.md öffnen (Sektion "UpdateGame")
   - Aktuelle Implementierung markieren
   - AI-Assistent aktivieren
   ```

2. **Prompt an AI:**
   ```
   Basierend auf DesignDocument.md Sektion "UpdateGame":
   
   Der aktuelle Code normalisiert bereits diagonale Bewegungen:
   - Berechnet Länge mit sqrt(moveX² + moveY²)
   - Teilt moveX und moveY durch Länge
   
   Bitte optimiere diese Normalisierung, indem du:
   a) Eine Konstante für die Bewegungsgeschwindigkeit einführest
   b) Die Geschwindigkeit in Player.speed auslagert
   c) Den Code lesbarer machst
   
   Wichtig: Behalte die Architektur bei!
   ```

3. **Überprüfung:**
   - AI-Code gegen DesignDocument.md prüfen
   - Normalisierungslogik erhalten? ✅
   - GameState-Struktur unverändert? ✅
   - Delta-Time noch verwendet? ✅

4. **Dokumentation aktualisieren:**
   - Optimierungen in "Erweiterungsmöglichkeiten" dokumentieren
   - Neue Konstanten/Strukturen in DesignDocument.md hinzufügen

---

## 🔍 Code-Review mit Design-Dokument

Bevor du AI-generierten Code commitest:

**Checkliste:**
- [ ] Entspricht der Code dem Design in DesignDocument.md?
- [ ] Sind alle Datenstrukturen konsistent?
- [ ] Wird deltaTime für zeitunabhängige Bewegung verwendet?
- [ ] Ist Normalisierung bei diagonalen Bewegungen implementiert?
- [ ] Gibt es neue Funktionen, die dokumentiert werden müssen?
- [ ] Passt der Code zur bestehenden Architektur?

---

## 📚 Weiterführende Ressourcen

- **DesignDocument.md**: Hauptarchitektur und Design
- **DOKUMENTATION/README.md**: Projekt-Dokumentations-Struktur
- **DOKUMENTATION/AI-Nutzung-Leitfaden.md**: Dieser Guide

**Regel:** Wenn du unsicher bist, lies zuerst DesignDocument.md!
