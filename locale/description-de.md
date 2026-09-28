AIC Tactics ergänzt die AIC um Rekrutierung, Angriffsziele, Überfälle und Belagerungen. Nicht gesetzte Felder behalten ihre Standardwerte; bei Belagerungsoptionen gilt dann der Modulschalter.

### Rekrutierung

- `RecruitPolicy`: `Native` belässt die Rekrutierung wie bisher. `WeightedRoles` verteilt neue Truppen auf Verteidigung, Überfälle, Hauptarmee und Ausfälle. Die folgenden vier Optionen benötigen `WeightedRoles`.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: Anteil der Ausfälle je KI-Stärke, **0–100**, Standard **0**. Die vier Rollenanteile jeder Stufe müssen zusammen **100** ergeben. AIC-Truppenlisten, Intervalle und Kontingente gelten weiter.
- `RecruitConditions`: Bis zu **8** Regeln; die erste passende ersetzt die Rollenanteile. `When` kann `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` und `EquipmentSurplus` prüfen. Mehrere Angaben gelten gemeinsam; `false` verlangt, dass ein Zustand nicht vorliegt.
- `DefRecruitComposition`: `Native` oder `PreserveSlots`. Letzteres behält die Anteile von `DefUnit1..8` bei; doppelte Einträge wiegen stärker. Fehlt Ausrüstung, bleibt der Platz frei.
- `RecruitInitialDefenseMonths`: **0–30**, Standard **6**. Solange die Verteidigung unter Sollstärke liegt, warten Hauptarmee und Überfälle. **0** schaltet die Wartezeit aus.

### Angriffsziele und Gegenschläge

- `AttackTargetPolicy`: `Inherit` nutzt `TargetChoice`; `LowestPopulation`, `FewestTroops` und `LowestCombatPower` wählen nach Zivilisten, Truppen oder Kampfstärke. `Random` lost gleichmäßig aus. `LastAggressor` wählt den letzten Gegner, der einen Gegenschlag ausgelöst hat.
- `AttackTargetCommitment`: `PerAttack` hält das Ziel für einen Angriff, `UntilDefeated` über mehrere Angriffe. `Default` nutzt bei neuen Zielregeln oder `DuringAttack` `PerAttack`, sonst das bisherige Verhalten.
- `AttackPreparation`: `DuringAttack` stellt zu Hause bereits die nächste Welle auf; `Native` belässt die bisherige Vorbereitung. Truppenlimits gelten weiter.
- `AttackActivation`: `AfterProvocation` startet Angriffe und Überfälle erst nach einem Gegenschlag-Auslöser; `Immediate` startet wie bisher. Defensive Ausfälle bleiben möglich.
- `ProvocationRules`: Schwellen für `ThreatPower` (**100**), `CombatTicks` (**200**), `LossPower` (**100**) und `WindowTicks` (**800**). Längere Kämpfe am Bergfried oder hohe Truppenverluste lösen einen Gegenschlag aus, Schaden am Burgherrn sofort. Alle vier Werte zusammen angeben; **800 Ticks = ein Spielmonat**.

### Überfälle

- `RaidTargetPolicy`: `Native`, `NearestReachable` (nahe erreichbare Gebäude) oder `Opportunistic` (Entfernung, Schwerpunkt und Gefahr). Die übrigen Optionen benötigen eine der beiden neuen Zielregeln.
- `RaidGroupCount`: **1–4**, Standard **1**; teilt vorhandene Truppen auf, ohne neue zu rekrutieren.
- `RaidMinGroupSize`: **1–256**, Standard **4**; kleinere Gruppen warten oder schließen sich zusammen.
- `RaidFocus`: `Any`, `Food`, `Industry` oder `HighValue`; nur für `Opportunistic`.
- `RaidRiskTolerance`: `Low`, `Medium` oder `High` für feindliche Truppen und Befestigungen.
- `RaidEnemyScope`: `PrimeTarget` oder `AnyEnemy`; das Ziel der Hauptarmee bleibt gleich.

`RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` und `RaidRetargetDelay` gelten weiter.

### Belagerung und Ingenieure

Ein AIC-Wert hat Vorrang; fehlt er, gilt der jeweilige Modulwert (Standard in Klammern).

- `SafeSiegePlacement` (**an**): Meidet belegte Bauplätze und schützt eigene Einheiten. `false` nutzt die bisherige Platzierung.
- `ActualSiegeResourcePayment` (**aus**): Verlangt Rohstoffe und Gold vor dem Bau; Fehlmengen kauft die KI über ihren normalen Handel.
- `CoordinatedSiegeHarassment` (**aus**): Sammelt Belagerungsgeräte und schickt sie gemeinsam zu erreichbaren Schusspositionen. `SiegeHarassMinEngines` (0–20, Standard 3) legt die Wartezahl fest; nach einem Spielmonat ziehen die erreichbaren Geräte los. `HarassingSiegeEnginesMax` bleibt die Gesamtgrenze.
- `LargerSiegeForces` (**aus**): Wiederholt beim Hauptangriff den eingestellten Gerätemix bis `SiegeForceMax` (0–20, Standard 10). **0** belässt eine normale Bauserie; `AttMaxEngineers` begrenzt die Besatzungen.
- `CorrectEngineerRoleCounting` (**an**): Zählt Ingenieure im zugewiesenen Truppenverband. Belagerungsbesatzungen und Öldienst zählen nicht für Truppenkontingente.
