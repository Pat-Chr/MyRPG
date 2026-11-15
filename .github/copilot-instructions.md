# Copilot Instructions

## Projektrichtlinien
- Vor Änderungen die für die Aufgabe relevanten Abschnitte von `DesignDocument.md` prüfen und die darin festgelegten Vorgaben beachten. Insbesondere die Trennung von Debug-Anzeigen und Release-Build beibehalten.
- Festgelegte Vorgaben von offenen Fragen, Plänen und historischen Einträgen unterscheiden. Bei widersprüchlichen oder unklaren Vorgaben den Nutzer um Klärung bitten, statt eine Entscheidung zu unterstellen.
- Wenn eine Aufgabe eine Designentscheidung ausdrücklich ändert, die betroffenen Festlegungen im Design-Dokument entsprechend aktualisieren.
- Nutzer möchte eine saubere, modulare Daten- und Dateistruktur mit getrennten Dateien für Engine-Teile (z. B. später Sounds, Sprites, Story-Skripte).
- Entscheidungen zu Asset-Standards und -Struktur sollen im zentralen Design-Dokument gepflegt werden.
- Beim Kompilieren soll der Assets-Ordner inklusive Inhalte automatisch im Build-Ausgabeverzeichnis erstellt/kopiert werden.
- Nach dem Entfernen temporärer Testdateien prüfen, dass keine Verweise darauf in Projekt- oder Filterdateien verbleiben, und den Build nach der Bereinigung überprüfen.