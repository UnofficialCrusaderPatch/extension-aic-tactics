Lege fest, wie deine KI Truppen rekrutiert und ihre Angriffsziele auswählt.

Trage die gewünschten Felder in deine AIC-Konfiguration ein. Ohne neue Einstellungen behalten bestehende KI-Persönlichkeiten ihr Verhalten.

### Angriffsziele

| `AttackTargetPolicy` | Zielwahl |
| --- | --- |
| `Inherit` (Standard) | Bisherige Auswahl über `TargetChoice`. |
| `LowestPopulation` | Gegner mit den wenigsten Zivilisten. |
| `FewestTroops` | Gegner mit den wenigsten Militäreinheiten. |
| `LowestCombatPower` | Gegner mit der geringsten geschätzten Kampfstärke, unabhängig von der Entfernung. |
| `Random` | Zufälliger Gegner; alle geeigneten Gegner haben dieselbe Chance. |
| `LastAggressor` | Letzter Angreifer, dessen Angriff die Bedingungen für einen Gegenschlag erfüllt. |

Mit `AttackTargetCommitment` legst du fest, wie lange die KI am gewählten Gegner festhält:

- `Default`: `PerAttack` bei neuen Zielregeln oder `DuringAttack`; sonst das bisherige Verhalten.
- `PerAttack`: Ziel zu Beginn wählen und während dieses Angriffs beibehalten.
- `UntilDefeated`: denselben Gegner über mehrere Angriffe hinweg behalten, solange er ein gültiges Ziel ist.

Zufälliger Gegner pro Angriff; derselbe Gegner kann erneut gewählt werden:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Rekrutierung

`RecruitPolicy` steht standardmäßig auf `Native`. Mit `WeightedRoles` verteilst du die Rekrutierung auf Verteidigung, Überfälle, Hauptangriffsarmee und Ausfälle aus der Burg. Truppenlisten, Intervalle und Kontingente gelten weiter.

Für **jede** Stärkestufe (`Default`, `Weak`, `Strong`) müssen die vier ganzzahligen Gewichte zusammen **100** ergeben. `RecruitProbSortie…` ist standardmäßig 0. Die Gewichte steuern die Rekrutierung, nicht die genaue Zusammensetzung der fertigen Armee.

Beispiel: mehr Verteidigung bei schwacher KI; Vorrang für Verteidigung, solange deren Sollstärke nicht erreicht ist:

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
  "RecruitProbSortieStrong": 10,
  "RecruitConditions": [
    {
      "When": {
        "DefenseIncomplete": true
      },
      "Defense": 60, "Raid": 0, "Attack": 30, "Sortie": 10
    }
  ]
}
```

`RecruitConditions` ist optional (Standard: `[]`, höchstens acht Regeln). Die erste passende Regel ersetzt die Gewichte der aktuellen Stärkestufe. Jede Regel braucht alle vier Gewichte mit Summe 100.

In `When` kannst du mit `Strength` eine Stärkestufe angeben oder folgende Bedingungen prüfen:

| Bedingung | Prüft, ob … |
| --- | --- |
| `HomeUnderThreat` | die eigene Burg bedroht ist. |
| `AttackActive` | ein Angriff läuft. |
| `DefenseIncomplete` | die Verteidigung ihre Sollstärke noch nicht erreicht hat. |
| `EquipmentSurplus` | überschüssige Ausrüstung vorhanden ist. |

`true` verlangt, dass die Bedingung zutrifft; `false`, dass sie nicht zutrifft. Alle angegebenen Bedingungen müssen passen. Weggelassene Bedingungen spielen keine Rolle; ein leeres `When` passt immer.

### Einstellungen ändern

Ändere zusammengehörige Felder gemeinsam. Teiländerungen behalten nicht angegebene Werte bei; mit `Default` löst du ein zuvor gesetztes `UntilDefeated` ab. So kehren beide Systeme zum bisherigen Verhalten zurück:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Frühere Ausfallgewichte und Regeln bleiben unter `Native` gespeichert, sind aber inaktiv. Gib sie bei dieser Änderung nicht mit an. Ein Ausfallgewicht von 0 unter `WeightedRoles` stellt `Native` nicht wieder her.

### Vorbereitung und Gegenschläge

- `AttackPreparation`: `Native` (Standard) oder `DuringAttack`, um während eines Angriffs die nächste Welle zu Hause aufzustellen. Truppenlimits und Wellenwachstum gelten weiter.
- `AttackActivation`: `Immediate` (Standard) oder `AfterProvocation`. Dann beginnen Angriffe und Überfälle erst nach einem ausreichend schweren Angriff des Gegners. Defensive Ausfälle bleiben möglich.
- `ProvocationRules`: Schwellen für Gegenschläge: standardmäßig `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800**. Längere Kämpfe am Bergfried oder ausreichende Truppenverluste lösen einen Gegenschlag aus; Schaden am Burgherrn zählt sofort. Alle vier Werte gemeinsam angeben. **800 Ticks = ein Spielmonat**.

### Überfälle

- `RaidTargetPolicy`: `Native` (Standard), `NearestReachable` für nahe erreichbare Gebäude oder `Opportunistic` für eine Auswahl nach Entfernung, Schwerpunkt und Gefahr.
- `RaidGroupCount`: **1–4**, Standard **1**. Teilt die vorhandenen Überfalltruppen auf, ohne zusätzliche Truppen zu rekrutieren.
- `RaidMinGroupSize`: **1–256**, Standard **4**. Kleinere Gruppen warten oder schließen sich zusammen.
- `RaidFocus`: `Any` (Standard), `Food` (Nahrung), `Industry` (Produktion) oder `HighValue` (Wiederaufbaukosten). Benötigt `Opportunistic`.
- `RaidRiskTolerance`: `Low`, `Medium` (Standard) oder `High`. Bestimmt, wie viel Gefahr durch feindliche Truppen und Verteidigungsanlagen akzeptiert wird.
- `RaidEnemyScope`: `PrimeTarget` (Standard) oder `AnyEnemy`. Das Ziel der Hauptarmee bleibt davon unberührt.

Die fünf Überfalloptionen benötigen eine neue `RaidTargetPolicy`. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` und `RaidRetargetDelay` gelten weiter. Zurück zum bisherigen Verhalten: `AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
