AIC Tactics steuert für jede installierte KI Rekrutierung, Angriffe, Überfälle und Belagerungsgeräte. Die Einstellungen gehören in ihre AIC. Neue Rekrutierungs-, Angriffs- und Überfallregeln gelten nur, wenn sie gewählt werden; bei fehlenden Belagerungs- und Ingenieurfeldern gelten die Modulwerte. Truppenlisten und Obergrenzen bleiben gültig.

### Rekrutierung

- `RecruitPolicy`: `Native` behält die bisherige Rekrutierung. `WeightedRoles` verteilt neue Truppen auf Verteidigung, Überfälle, Hauptarmee und Ausfälle.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: Anteil für Ausfälle in den drei bisherigen KI-Stärkestufen. Verteidigung, Überfälle, Angriff und Ausfälle müssen je Stufe zusammen 100 ergeben.
- `RecruitConditions`: bis zu 8 Regeln für eine Rekrutierungsentscheidung; die erste passende ersetzt diese Anteile. `When` kann `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` und `EquipmentSurplus` prüfen.
- `DefRecruitComposition`: `PreserveSlots` hält die Anteile aus `DefUnit1..8` ein; `Native` behält die bisherige Auswahl.
- `RecruitInitialDefenseMonths`: So lange warten Überfälle und Hauptarmee auf eine volle Verteidigung. 0 schaltet die Wartezeit aus; Standard sind 6 Monate.

Ausfallanteile, Lage-Regeln, Verteidigermix und anfängliche Wartezeit benötigen `WeightedRoles`. Rekrutierungsintervalle und Kontingente gelten weiter.

### Angriffe und Gegenschläge

- `AttackTargetPolicy`: `Inherit` nutzt `TargetChoice`. Die übrigen Werte wählen nach wenigen Zivilisten (`LowestPopulation`), wenigen Soldaten (`FewestTroops`), geringer Kampfstärke (`LowestCombatPower`), Zufall (`Random`) oder dem letzten passenden Angreifer (`LastAggressor`).
- `AttackTargetCommitment`: `PerAttack` hält das Ziel während eines Angriffs; `UntilDefeated` auch zwischen Angriffen. `Default` nutzt bei neuer Zielregel oder Vorbereitung der nächsten Welle `PerAttack`, sonst das bisherige Verhalten.
- `AttackPreparation`: `DuringAttack` stellt zu Hause die nächste Welle auf, während die Armee angreift; `Native` behält den bisherigen Ablauf.
- `AttackActivation`: `AfterProvocation` wartet vor Angriffen und Überfällen auf einen ausreichenden feindlichen Angriff; `Immediate` wartet nicht. Defensive Ausfälle bleiben möglich.
- `ProvocationRules`: `ThreatPower` und `CombatTicks` bestimmen die Bedrohung am Bergfried, `LossPower` und `WindowTicks` die Verluste und den Zeitraum. Schaden am Burgherrn genügt sofort. Alle vier Werte zusammen angeben.

### Überfälle

- `RaidTargetPolicy`: `Native` behält bisherige Überfälle; `NearestReachable` wählt nahe erreichbare Gebäude; `Opportunistic` berücksichtigt auch Schwerpunkt und Gefahr.
- `RaidGroupCount` (1–4) teilt die vorhandenen Überfalltruppen auf; `RaidMinGroupSize` (1–256) lässt zu kleine Gruppen warten oder zusammenrücken.
- `RaidFocus` wählt `Any`, `Food`, `Industry` oder `HighValue` und benötigt `Opportunistic`. `RaidRiskTolerance` bestimmt die Gefahrentoleranz (`Low`, `Medium`, `High`). `RaidEnemyScope` beschränkt Überfälle auf das Hauptziel oder erlaubt jeden Gegner.

Die fünf Überfallfelder nach `RaidTargetPolicy` benötigen eine neue Überfallregel. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` und `RaidRetargetDelay` gelten weiter.

### Belagerung und Ingenieure

- `SafeSiegePlacement` meidet belegte Bauplätze (Modulstandard AN).
- `ActualSiegeResourcePayment` verlangt Rohstoffe und Gold; Fehlmengen kauft die KI über den normalen Handel (AUS).
- `CoordinatedSiegeHarassment` schickt gesammelte Geräte zu erreichbaren Schusspositionen (AUS). `SiegeHarassMinEngines` ist die angestrebte Mindestzahl (0–20, Standard 3); `HarassingSiegeEnginesMax` bleibt die Gesamtgrenze.
- `LargerSiegeForces` wiederholt den eingestellten Gerätemix beim Hauptangriff (AUS). `SiegeForceMax` begrenzt aktive und geplante Geräte (0–20, Standard 10); 0 belässt eine normale Bauserie. `AttMaxEngineers` begrenzt weiter die Besatzungen.
- `CorrectEngineerRoleCounting` zählt zugewiesene Ingenieure für ihre Truppenkontingente; Belagerung und Öldienst bleiben getrennt (AN).

Bei Belagerung und Ingenieuren hat ein ausdrücklich gesetzter AIC-Wert, auch `false` oder 0, Vorrang vor dem Modulwert. Fehlt er, gilt der Modulwert.
