Ergänzt deine AIC um Einstellungen für Rekrutierung und Angriffsziele. Ohne neue Angaben bleibt das bisherige KI-Verhalten erhalten.

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

- `Default`: bei neuen Zielregeln gilt `PerAttack`; bei `Inherit` bleibt das bisherige Verhalten.
- `PerAttack`: denselben Gegner während des gesamten Angriffs beibehalten.
- `UntilDefeated`: denselben Gegner über mehrere Angriffe hinweg behalten, solange er ein gültiges Ziel ist.

Wenn du nur einzelne Werte änderst, bleiben die übrigen erhalten. Zurück zum bisherigen Verhalten: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. Bisherige Sortie-Werte und Regeln bleiben gespeichert, werden aber nicht angewendet; gib sie beim Wechsel zu `Native` nicht mit an.
