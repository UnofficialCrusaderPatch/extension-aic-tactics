Az AIC Tactics minden telepített MI-nél külön szabályozza a toborzást, a támadásokat, a portyákat és az ostromgépeket. A mezőket az adott MI AIC-jében add meg. Az új toborzási, támadási és portyaszabályok választhatók; a kihagyott ostrom- és mérnökmezők a modul beállításait követik. A meglévő egységlisták és korlátok változatlanok.

### Toborzás

- `RecruitPolicy`: a `Native` megtartja a megszokott toborzást. A `WeightedRoles` a védelem, a portyák, a fő sereg és a kitörések között osztja el az újoncokat.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: a kitörések aránya a három eredeti erősségi állapotban. A védelem, portya, támadás és kitörés aránya állapotonként összesen 100 legyen.
- `RecruitConditions`: legfeljebb 8 sorrendezett szabály; az első megfelelő egy toborzásra felülírja ezeket az arányokat. A `When` a `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` és `EquipmentSurplus` feltételeket vizsgálhatja.
- `DefRecruitComposition`: a `PreserveSlots` megtartja a `DefUnit1..8` arányait; a `Native` a megszokott választást használja.
- `RecruitInitialDefenseMonths`: ennyi ideig vár a portya és a fő sereg, amíg a védelem eléri a létszámát. A 0 kikapcsolja a várakozást; alapérték 6 hónap.

A kitörések arányaihoz, a helyzeti szabályokhoz, a védők összetételéhez és a kezdeti várakozáshoz `WeightedRoles` kell. A toborzási időközök és korlátok továbbra is érvényesek.

### Támadás és megtorlás

- `AttackTargetPolicy`: az `Inherit` a `TargetChoice` értékét használja. A többi lehetőség a legkevesebb lakost (`LowestPopulation`), katonát (`FewestTroops`), leggyengébb sereget (`LowestCombatPower`), véletlen ellenfelet (`Random`) vagy az utolsó megfelelő támadót (`LastAggressor`) választja.
- `AttackTargetCommitment`: a `PerAttack` egy támadás alatt, az `UntilDefeated` több támadáson át tartja meg a célt. A `Default` új célválasztás vagy következő hullám előkészítése esetén `PerAttack`, egyébként a korábbi működés.
- `AttackPreparation`: a `DuringAttack` otthon készíti elő a következő hullámot, miközben a sereg támad; a `Native` megtartja a korábbi ütemet.
- `AttackActivation`: az `AfterProvocation` kellően erős ellenséges támadásra vár a sereg vagy portya indítása előtt; az `Immediate` nem vár. A védekező kitörések megmaradnak.
- `ProvocationRules`: a `ThreatPower` és `CombatTicks` a vár közeli fenyegetést, a `LossPower` és `WindowTicks` a veszteségeket és az időablakot szabja meg. Az úr sérülése azonnal kiváltja a megtorlást. Mind a négy értéket add meg.

### Portyák

- `RaidTargetPolicy`: a `Native` megtartja a megszokott portyákat; a `NearestReachable` közeli, elérhető épületeket választ; az `Opportunistic` a cél fontosságát és a veszélyt is figyeli.
- A `RaidGroupCount` (1–4) a meglévő portyázókat osztja fel; a `RaidMinGroupSize` (1–256) a kis csoportokat várakoztatja vagy egyesíti.
- A `RaidFocus` értéke `Any`, `Food`, `Industry` vagy `HighValue`; ehhez `Opportunistic` kell. A `RaidRiskTolerance` a vállalt veszély (`Low`, `Medium`, `High`). A `RaidEnemyScope` a fő célra korlátoz, vagy bármely ellenséget enged.

A `RaidTargetPolicy` utáni öt mezőhöz új portyaszabály kell. A `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` és `RaidRetargetDelay` továbbra is érvényes.

### Ostrom és mérnökök

- `SafeSiegePlacement`: kerüli a foglalt építési helyeket (modul: BE).
- `ActualSiegeResourcePayment`: nyersanyagot és aranyat kér; a hiányt az MI a szokásos kereskedelemmel szerzi be (KI).
- `CoordinatedSiegeHarassment`: összegyűjtött ostromgépeket küld elérhető lőállásokba (KI). A `SiegeHarassMinEngines` a kívánt minimum (0–20, alapérték 3); a `HarassingSiegeEnginesMax` marad az összes gép korlátja.
- `LargerSiegeForces`: megismétli a fő támadás beállított ostromgép-összetételét (KI). A `SiegeForceMax` korlátozza az aktív és tervezett gépeket (0–20, alapérték 10); a 0 egy eredeti építési sorozatot hagy. Az `AttMaxEngineers` továbbra is korlátozza a kezelőket.
- `CorrectEngineerRoleCounting`: a beosztott mérnököket a megfelelő csapatkeretbe számítja; az ostrom és az olajkezelés külön marad (BE).

Az ostrom és a mérnökök esetében az AIC-ben megadott érték, így a `false` vagy a 0 is, felülírja a modul beállítását. Ha hiányzik, a modul értéke érvényes.
