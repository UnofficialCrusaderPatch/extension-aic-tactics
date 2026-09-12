Steuere Rekrutierung, die Vorbereitung weiterer Angriffswellen, Angriffsziele und Überfälle über deine AIC. Ohne neue Angaben bleibt das bisherige Verhalten erhalten.

### Rekrutierung

- `RecruitPolicy`: `Native` (Standard) behält die bisherige Rekrutierung bei. `WeightedRoles` verteilt sie auf Verteidigung, Überfälle, Hauptarmee und Ausfälle.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: Gewichtung für Ausfalltruppen, die vor der Burg kämpfen, jeweils bei normaler, schwacher und starker KI. Ganze Zahlen von **0–100**, Standard **0**. Zusammen mit den bisherigen Gewichten für Verteidigung, Überfälle und Angriff muss jede Stärkestufe **100** ergeben. Benötigt `WeightedRoles`; Truppenlisten, Rekrutierungsintervalle und Truppenlimits gelten weiter.
- `RecruitConditions`: bis zu **8** Regeln, standardmäßig leer. Die erste passende Regel ersetzt die Gewichte der Stärkestufe. Jede Regel enthält `When` und vier Gewichte mit Summe 100: `Defense`, `Raid`, `Attack`, `Sortie`. Benötigt `WeightedRoles`.
- `DefRecruitComposition`: `Native` (Standard) behält das bisherige Verhalten bei. `PreserveSlots` hält Plätze entsprechend `DefUnit1..8` frei. Mehrfache Einträge erhöhen den Anteil einer Einheit; fehlende Ausrüstung lässt ihre Plätze offen. Benötigt `WeightedRoles`.
- `RecruitInitialDefenseMonths`: **0–30** Monate, Standard **6**. Solange die Verteidigung noch nicht vollzählig ist, werden in dieser Zeit keine Räuber oder Truppen für die Hauptarmee rekrutiert. Ausfälle bleiben erlaubt; Gewicht 0 bleibt 0. **0** schaltet die Wartezeit aus. Benötigt `WeightedRoles`.

In `When` kannst du `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (Burg bedroht), `AttackActive` (Angriff läuft), `DefenseIncomplete` (Verteidigung unter Sollstärke) und `EquipmentSurplus` (Ausrüstung übrig) prüfen. Alle angegebenen Bedingungen müssen passen: `true` verlangt, dass sie zutreffen; `false`, dass sie nicht zutreffen. Ein leeres `When` passt immer.

### Angriffsziele

`AttackTargetPolicy`:

- `Inherit` (Standard): Bisherige Auswahl über `TargetChoice`.
- `LowestPopulation`: Gegner mit den wenigsten Zivilisten.
- `FewestTroops`: Gegner mit den wenigsten Militäreinheiten.
- `LowestCombatPower`: Geringste geschätzte Kampfstärke, unabhängig von der Entfernung.
- `Random`: Zufälliger Gegner; alle geeigneten Gegner haben dieselbe Chance.
- `LastAggressor`: Letzter Angreifer, der die Bedingungen für einen Gegenschlag erfüllt.

`AttackTargetCommitment` legt fest, wie lange das Ziel bleibt:

- `Default`: `PerAttack` bei neuen Zielregeln oder `DuringAttack`; sonst das bisherige Verhalten.
- `PerAttack`: denselben Gegner während des gesamten Angriffs beibehalten.
- `UntilDefeated`: denselben Gegner über mehrere Angriffe hinweg behalten, solange er ein gültiges Ziel ist.

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
