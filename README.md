# MyRPG - RPG Spiel Framework

Ein minimalistisches RPG-Spiel für Windows, geschrieben in C++ mit Win32 API.

## 📚 Dokumentation

Das Design-Dokument enthält alle technischen Details zur Architektur und Implementierung:

- **[DesignDocument.md](./DesignDocument.md)** - Vollständige technische Dokumentation
  - Spielmechanik
  - Datenstrukturen (GameState, Player, InputState)
  - Game Loop Implementation
  - Update-Logik
  - Rendering-Struktur
  - Event Handling
  - Erweiterungsmöglichkeiten

## 🚀 Quick Start

1. Öffne die Lösung in Visual Studio: `MyRPG.slnx`
2. Stelle sicher, dass du Debug-Konfiguration verwendest
3. Baue das Projekt: `Build → Build Solution`
4. Führe aus im Debug-Modus

## 📁 Projektstruktur

```
MyRPG/
├── main.cpp              # Hauptprogramm mit Game Loop
├── MyRPG.h               # Header für globale Variablen
├── framework.h           # Windows Framework Header
├── Resource.h            # Ressourcen-IDs
├── targetver.h           # Versionsinformationen
└── [Visual Studio Projektdateien]
```

## 🛠️ Technologie-Stack

- **Windows API** (Win32)
- **C++17** oder neuer
- **DirectX/OpenGL** (für Rendering - TODO)

## 📝 Nächste Schritte

1. Implementiere `RenderGame` Funktion (siehe DesignDocument.md, Sektion "Rendering")
2. Füge Kollisionsdetektion hinzu
3. Implementiere Spielstände
4. Integriere Gegner/Hindernisse

## 🔧 Entwicklungshinweise

- Alle Änderungen müssen mit dem Design-Dokument abgestimmt sein
- Neue Features sollten in die Dokumentation aufgenommen werden
- Verwende Delta-Time für zeitunabhängige Bewegung
- Behalte Normalisierung bei diagonalen Bewegungen bei

---

*Für detaillierte technische Informationen: [DesignDocument.md](./DesignDocument.md)*
