AIC Tactics wird entwickelt, damit Autoren von KI-Persönlichkeiten Rekrutierung, Angriffe und Raubzüge bei Bedarf gezielt steuern können.

Geplant sind bedingte Rekrutierungs- und Ausfallwahrscheinlichkeiten, die Vorbereitung der nächsten Armee während eines Angriffs, Gegnerwahl und Vergeltung sowie Raubzüge mit einer begrenzten Anzahl getrennter Gruppen. Bestehende Persönlichkeiten sollen ihr Verhalten beibehalten, solange ihr Autor keine neue Verhaltensregel ausdrücklich aktiviert.

Neue Regeln zur Gegnerwahl sollen standardmäßig dasselbe Ziel während des gesamten Angriffs beibehalten (`PerAttack`). Mit `UntilDefeated` können Autoren den Gegner über mehrere Angriffe hinweg festlegen. Unveränderte Persönlichkeiten behalten ihr bisheriges Verhalten einschließlich der Zielstabilität durch Legacy.

**In Entwicklung: Es gibt noch keine spielbereite Veröffentlichung.**

## Einrichtung (Entwicklungsvorschau)

Dies sind Felder in der AIC-Konfiguration einer KI-Persönlichkeit, keine neuen GUI-Bedienelemente. Die Beispiele erläutern die Konfiguration in Entwicklung; sie sind weder vollständige KI-Pakete noch spielbereite Einstellungen. Parameternamen und Werte müssen unabhängig von der GUI-Sprache unverändert bleiben.

### Bisheriges Verhalten behalten

Bei einer unveränderten Persönlichkeit die neuen Felder weglassen. Standardwerte: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit` und `AttackTargetCommitment: Default`. Rekrutierung und Zielwahl lassen sich unabhängig aktivieren.

### Gegner auswählen

Für `AttackTargetPolicy` ist Folgendes vorgesehen:

| Wert | Bedeutung |
| --- | --- |
| `Inherit` | Ursprüngliches `TargetChoice` der Persönlichkeit verwenden (Standard). |
| `LowestPopulation` | Gegner mit den wenigsten Zivilisten bevorzugen. |
| `FewestTroops` | Gegner mit den wenigsten Militäreinheiten bevorzugen. |
| `LowestCombatPower` | Niedrigste geschätzte Militärstärke bevorzugen, ohne Entfernungsgewichtung. |
| `Random` | Jeder geeignete Gegner hat dieselbe Chance; Wiederholungen sind möglich. |
| `LastAggressor` | Auf den letzten feindlichen Vorfall reagieren, der die Kriterien erfüllt, nicht auf jeden Treffer. |

`AttackTargetCommitment: Default` bedeutet bei neuen Zielregeln `PerAttack`: Ziel beim Angriffsbeginn wählen und während des gesamten Angriffs beibehalten. Mit `Inherit` bleibt das bisherige Native-/Legacy-Verhalten erhalten. `UntilDefeated` ausdrücklich wählen, um einen gültigen Gegner über mehrere Angriffe hinweg beizubehalten. Wird das Ziel ungültig, muss der laufende Angriff erst abgewickelt werden, bevor ein neues Ziel gewählt wird; die ausgerückte Armee darf nicht mitten im Angriff den Gegner wechseln.

Beispiel: zufälliger Gegner pro Angriff, ohne Zielwechsel während des Angriffs:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Rekrutierung aufteilen

`RecruitPolicy: WeightedRoles` verteilt Rekrutierungsentscheidungen auf Verteidigung, Überfälle, Hauptangriffsarmee und Ausfälle (Truppen, die die Burg verlassen, um nahe Gegner zu bekämpfen). Bestehende Intervalle, Truppenlisten und Kontingente gelten weiter. Die Gewichte beeinflussen mögliche Rekrutierungsentscheidungen, garantieren aber kein festes Verhältnis in der fertigen Armee.

Alle drei Stärkestufen einstellen: `Default`, `Weak` und `Strong`. Die vier ganzzahligen Gewichte jeder Stufe müssen zusammen 100 ergeben. `RecruitProbDef…`, `RecruitProbRaid…` und `RecruitProbAttack…` werden weiterverwendet; `RecruitProbSortie…` ergänzt Ausfälle und hat den Standardwert 0. Es gibt keine automatische Umrechnung. Beispiel, keine Balance-Empfehlung:

```json
{
  "RecruitPolicy": "WeightedRoles",
  "RecruitProbDefDefault": 30,
  "RecruitProbRaidDefault": 20,
  "RecruitProbAttackDefault": 40,
  "RecruitProbSortieDefault": 10,
  "RecruitProbDefWeak": 60,
  "RecruitProbRaidWeak": 10,
  "RecruitProbAttackWeak": 20,
  "RecruitProbSortieWeak": 10,
  "RecruitProbDefStrong": 20,
  "RecruitProbRaidStrong": 20,
  "RecruitProbAttackStrong": 50,
  "RecruitProbSortieStrong": 10
}
```

### Bedingte Rekrutierung (erweitert)

`RecruitConditions` ist standardmäßig eine leere Liste und erlaubt bis zu acht geordnete Regeln. Jede benötigt `When` sowie alle vier Gewichte `Defense`, `Raid`, `Attack`, `Sortie` mit Summe 100. Die erste passende Regel ersetzt die Stärkenzeile. Ohne Treffer gilt diese Zeile.

In `When` wählt `Strength` zwischen `Default`, `Weak` und `Strong`. Ja/Nein-Prüfungen: `HomeUnderThreat` (Heimat bedroht), `AttackActive` (Angriff läuft), `DefenseIncomplete` (Verteidigung unvollständig), `EquipmentSurplus` (überschüssige Ausrüstung). `true` verlangt den Zustand, `false` dessen Abwesenheit; weggelassene Prüfungen werden ignoriert. Alle angegebenen Prüfungen müssen zutreffen; ein leeres `When` trifft immer zu.

### Einstellungen ändern oder zurücknehmen

Zusammengehörige Felder gemeinsam anwenden. Teiländerungen behalten weggelassene Werte bei, auch ein zuvor gewähltes `UntilDefeated`. Für die automatische Bindung `Default` setzen. Zum Abschalten dieser Regeln:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Der alleinige Wechsel zu `RecruitPolicy: Native` behält frühere Ausfallgewichte und Regeln als inaktive Werte. Unter `Native` keine Ausfallfelder oder Regeln setzen. Ein Ausfallgewicht von 0 unter `WeightedRoles` stellt die ursprüngliche Rekrutierung nicht wieder her.

**Noch offen:** Spielintegration, genaue native Definitionen für Bedrohung, Ausrüstungsüberschuss und Gegnerzählung, Vergeltungsschwellen, Vorbereitung der nächsten Welle, geteilte Überfälle und Durchsetzung der Legacy-Optionen. Es gibt noch keine freigabereife Anleitung für die Legacy-Schalter. Diese Beschreibungen belegen keine Mehrspieler-, Speicherstand- oder Replay-Kompatibilität.
