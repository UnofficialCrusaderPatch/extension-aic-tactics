AIC Tactics steuert für jede installierte KI Rekrutierung, Angriffe, Überfälle und Belagerungsgeräte. Die Einstellungen gehören in ihre AIC. Neue Rekrutierungs-, Angriffs- und Überfallregeln gelten nur, wenn sie gewählt werden; bei fehlenden Belagerungsfeldern gelten die Modulwerte. Truppenlisten und Obergrenzen bleiben gültig.

### Rekrutierung

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: Anteil für Ausfälle je Stärkestufe. Sobald du einen Wert setzt, nutzt diese KI vier Rekrutierungsanteile. Verteidigung, Überfälle, Angriff und Ausfälle müssen in jeder Stufe zusammen 100 ergeben.
- `RecruitConditions`: andere Anteile für bestimmte Situationen. Beispiel: Bei Gefahr zu Hause 70 % Verteidigung, 20 % Angriff und 10 % Ausfälle. Die erste passende Regel gilt für diese Rekrutierung.
- `RecruitPolicy`: optionaler Kompatibilitätsschalter. `Native` behält die ursprüngliche Auswahl; `WeightedRoles` aktiviert die vier Anteile ausdrücklich.
- `DefRecruitComposition`: `PreserveSlots` hält die Anteile aus `DefUnit1..8` ein; `Native` behält die bisherige Auswahl.
- `RecruitInitialDefenseMonths`: So lange warten Überfälle und Hauptarmee auf eine volle Verteidigung. 0 schaltet die Wartezeit aus; Standard sind 6 Monate.

Ein Ausfallanteil aktiviert `WeightedRoles` automatisch. Für Situationsregeln, Verteidigermix und `RecruitInitialDefenseMonths` musst du es ausdrücklich setzen. Rekrutierungsintervalle und Kontingente gelten weiter.

### Angriffe und Gegenschläge

- `AttackTargetPolicy`: `Inherit` nutzt `TargetChoice`. `LowestPopulation`, `FewestTroops`, `LowestCombatPower`, `Random` und `LastAggressor` wählen ein Ziel pro Angriff. Für mehrere Angriffe: `{ "Choice": "LastAggressor", "UntilDefeated": true }`. Bei `LastAggressor` kannst du z. B. `"Provocation": { "LossPower": 200 }` ergänzen. `InheritPerAttack` hält die bisherige Wahl während eines Angriffs fest.
- `AttackTargetCommitment`: bleibt für ältere AICs gültig. In neuen AICs stehen Zielwahl und Dauer zusammen in `AttackTargetPolicy`.
- `AttackPreparation`: `DuringAttack` stellt zu Hause die nächste Welle auf, während die Armee angreift; `Native` behält den bisherigen Ablauf.
- `AttackActivation`: `AfterProvocation` wartet vor Angriffen und Überfällen auf einen ausreichenden feindlichen Angriff; `Immediate` wartet nicht. Defensive Ausfälle bleiben möglich.
- `ProvocationRules`: bleibt für ältere AICs sowie `AfterProvocation` und Bedrohungsregeln bei der Rekrutierung gültig. Bei `LastAggressor` können neue AICs die Werte unter `AttackTargetPolicy.Provocation` setzen.

### Überfälle

- `RaidTargetPolicy`: `Native` behält bisherige Überfälle; `NearestReachable` wählt nahe erreichbare Gebäude; `Opportunistic` berücksichtigt Schwerpunkt und Gefahr; `RandomNearby` wählt ein erreichbares Gebäude zufällig und räumt danach die Umgebung ab.
- `RaidGroupCount` (1–4) teilt die vorhandenen Überfalltruppen auf; `RaidMinGroupSize` (1–256) lässt zu kleine Gruppen warten oder zusammenrücken.
- `RaidFocus` erlaubt `Any`, `Food`, `Industry`, `HighValue` oder Anteile wie `{ "Food": 60, "Industry": 20 }`; der Rest gilt für beliebige Gebäude. Gilt bei `Opportunistic` und `RandomNearby`. `RaidRiskTolerance` bestimmt die Gefahr (`Low`, `Medium`, `High`); `RaidEnemyScope` das Hauptziel oder jeden Feind.

Die fünf Überfallfelder nach `RaidTargetPolicy` benötigen eine neue Überfallregel. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` und `RaidRetargetDelay` gelten weiter.

### Belagerung und Ingenieure

- `SafeSiegePlacement` meidet belegte Bauplätze (Modulstandard AN).
- `ActualSiegeResourcePayment` verlangt Rohstoffe und Gold; Fehlmengen kauft die KI über den normalen Handel (AUS).
- `CoordinatedSiegeHarassment` schickt gesammelte Geräte zu erreichbaren Schusspositionen (AUS). `SiegeHarassMinEngines` ist die angestrebte Mindestzahl (0–20, Standard 3); `HarassingSiegeEnginesMax` bleibt die Gesamtgrenze.
- `LargerSiegeForces` wiederholt den Gerätemix, solange lebende, freie Angriffsingenieure und Bauplätze vorhanden sind (AUS). `SiegeForceMax` ist eine optionale Obergrenze (0–64); 0 überlässt die Zahl den Ingenieuren. `AttMaxEngineers` begrenzt weiterhin Besatzungen.

Sichere Bauplätze gelten bei erweiterten Belagerungsregeln automatisch. Die KI zählt zugewiesene Ingenieure standardmäßig richtig; Fixed Engineers betreut Besatzung und Gerätelebenszyklus.

Ausdrückliche AIC-Belagerungswerte haben Vorrang vor Modulwerten.
