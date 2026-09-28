Az AIC Tactics minden telepített MI-nél külön szabályozza a toborzást, a támadásokat, a portyákat és az ostromgépeket. A mezőket az adott MI AIC-jében add meg. Az új toborzási, támadási és portyaszabályok választhatók; a kihagyott ostrommezők a modul beállításait követik. A meglévő egységlisták és korlátok változatlanok.

### Toborzás

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: a kitörések aránya erősségi állapotonként. Bármelyik megadása bekapcsolja a négyféle toborzási arányt ennél az MI-nél. A védelem, portya, támadás és kitörés összege minden állapotban 100 legyen.
- `RecruitConditions`: eltérő arányok adott helyzetben. Például veszélyben lévő várnál 70% védelem, 20% támadás, 10% kitörés. Az első illeszkedő szabály érvényes az adott toborzásra.
- `RecruitPolicy`: opcionális kompatibilitási kapcsoló. A `Native` megtartja az eredeti választást; a `WeightedRoles` kifejezetten bekapcsolja a négy arányt.
- `DefRecruitComposition`: a `PreserveSlots` megtartja a `DefUnit1..8` arányait; a `Native` a megszokott választást használja.
- `RecruitInitialDefenseMonths`: ennyi ideig vár a portya és a fő sereg, amíg a védelem eléri a létszámát. A 0 kikapcsolja a várakozást; alapérték 6 hónap.

A kitörési arány automatikusan bekapcsolja a `WeightedRoles` módot. A helyzeti szabályokhoz, a védők összetételéhez és a `RecruitInitialDefenseMonths` értékhez külön meg kell adni. A toborzási időközök és korlátok továbbra is érvényesek.

### Támadás és megtorlás

- `AttackTargetPolicy`: az `Inherit` a `TargetChoice` értékét használja. A `LowestPopulation`, `FewestTroops`, `LowestCombatPower`, `Random` és `LastAggressor` támadásonként egy célpontot választ.
- `AttackTargetCommitment`: a régi AIC-kben továbbra is használható. Új AIC-ben a cél és az időtartam együtt állítható az `AttackTargetPolicy` mezőben: `{ "Choice": "LastAggressor", "UntilDefeated": true }`.
- `AttackPreparation`: a `DuringAttack` otthon készíti elő a következő hullámot, miközben a sereg támad; a `Native` megtartja a korábbi ütemet.
- `AttackActivation`: az `AfterProvocation` kellően erős ellenséges támadásra vár a sereg vagy portya indítása előtt; az `Immediate` nem vár. A védekező kitörések megmaradnak.
- `ProvocationRules`: régi AIC-khez, az `AfterProvocation` beállításhoz és a toborzási fenyegetéshez marad. Új AIC-ben a `LastAggressor` küszöbei az `AttackTargetPolicy.Provocation` alatt adhatók meg.

### Portyák

- `RaidTargetPolicy`: a `Native` megtartja a portyákat; a `NearestReachable` közeli elérhető épületet választ; az `Opportunistic` a preferenciát és veszélyt is nézi; a `RandomNearby` véletlen elérhető cél után a közeli épületeket támadja.
- A `RaidGroupCount` (1–4) a meglévő portyázókat osztja fel; a `RaidMinGroupSize` (1–256) a kis csoportokat várakoztatja vagy egyesíti.
- A `RaidFocus` lehet `Any`, `Food`, `Industry`, `HighValue` vagy például `{ "Food": 60, "Industry": 20 }`; a maradék bármely épületre jut. `Opportunistic` és `RandomNearby` mellett használható. A `RaidRiskTolerance` a kockázatot, a `RaidEnemyScope` az ellenség körét adja meg.

A `RaidTargetPolicy` utáni öt mezőhöz új portyaszabály kell. A `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` és `RaidRetargetDelay` továbbra is érvényes.

### Ostrom és mérnökök

- `SafeSiegePlacement`: kerüli a foglalt építési helyeket (modul: BE).
- `ActualSiegeResourcePayment`: nyersanyagot és aranyat kér; a hiányt az MI a szokásos kereskedelemmel szerzi be (KI).
- `CoordinatedSiegeHarassment`: összegyűjtött ostromgépeket küld elérhető lőállásokba (KI). A `SiegeHarassMinEngines` a kívánt minimum (0–20, alapérték 3); a `HarassingSiegeEnginesMax` marad az összes gép korlátja.
- `LargerSiegeForces`: addig ismétli a gépösszetételt, amíg élő, szabad támadó mérnök és megfelelő hely van (KI). A `SiegeForceMax` választható korlát (0–64); 0-nál a mérnökök száma dönt. Az `AttMaxEngineers` továbbra is korlátozza a kezelőket.

A fejlett ostrombeállítások automatikusan bekapcsolják a biztonságos elhelyezést. A mérnökök szerepkörszámlálása alapból aktív; a Fixed Engineers a kezelők és gépek életciklusát kezeli.

Az AIC-ben megadott ostromértékek megelőzik a modul alapértékeit.
