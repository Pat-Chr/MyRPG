# 📚 MyRPG - Dokumentations-Ordner

Dieser Ordner enthält alle wichtigen Dokumentation für das Projekt. Alle Entwickler sollten diese Dateien kennen und bei Änderungen berücksichtigen.

## 📄 Wichtige Dokumente

### DesignDocument.md (Hauptdokument)
**Ort:** `[./../DesignDocument.md](../DesignDocument.md)` oder `DOKUMENTATION/DesignDocument.md`

Dies ist das **zentrale Dokument** für alle technischen Entscheidungen:
- ✅ Enthält die vollständige Architektur
- ✅ Dokumentiert alle Datenstrukturen
- ✅ Beschreibt den Game Loop und Update-Logik
- ✅ Listet Erweiterungsmöglichkeiten auf
- ✅ Enthält Referenzcode-Snippets

### Wie Copilot/AI-Assistenten das Dokument nutzen

Wenn du AI-Assistenten (wie Copilot) für Code-Erweiterungen verwendest:

1. **Öffne immer zuerst DesignDocument.md** im Editor
2. **Markiere relevante Abschnitte** (z.B. "UpdateGame", "GameState")
3. **Gib dem AI-Assistenten Kontext**, indem du sagst:
   - *"Basierend auf DesignDocument.md Sektion X..."*
   - *"Implementiere dies gemäß der Architektur in DesignDocument.md"*
   - *"Erweitere diesen Code, ohne die Struktur aus DesignDocument.md zu ändern"*

### Projektregeln für AI-Entwicklungen

❌ **Nicht machen:**
- Neue Funktionen ohne Berücksichtigung des bestehenden Designs
- Ändern von Datenstrukturen ohne Dokumentation
- Hinzufügen von Features, die nicht im Erweiterungsmöglichkeiten-Bereich stehen

✅ **Tun:**
- Alle Änderungen an Datenstrukturen in DesignDocument.md dokumentieren
- Neue Features dem "Erweiterungsmöglichkeiten"-Abschnitt hinzufügen
- Bei Rendering-Implementierung auf die darin beschriebene WM_PAINT Struktur achten

## 📋 Dokumentations-Checkliste

Bevor du Code änderst oder neue Features hinzufügst:

- [ ] Ist dies im DesignDocument.md dokumentiert?
- [ ] Muss ich ein neues Feature in "Erweiterungsmöglichkeiten" hinzufügen?
- [ ] Verstehe ich, wie das neue Feature mit der bestehenden Architektur interagiert?
- [ ] Habe ich alle relevanten Abschnitte von DesignDocument.md geöffnet?

## 🔄 Update-Prozess

Wenn du etwas Neues implementierst:

1. Code schreiben/ändern
2. Änderungen in DesignDocument.md unter "Erweiterungsmöglichkeiten" dokumentieren
3. README.md aktualisieren, wenn es neue Features gibt
4. Build testen
5. Optional: Neue Testfälle hinzufügen

---

*Dieses Dokument dient als Leitfaden für die Verwendung der Dokumentation im Projekt.*
