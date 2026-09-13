Az AIC-ben állíthatod be az MI toborzását, a következő hullám előkészítését, célpontjait és portyáit. Új beállítások nélkül a viselkedés változatlan.

### Toborzás

- `RecruitPolicy`: a `Native` (alapérték) megtartja a meglévő toborzást. A `WeightedRoles` a védelem, portyák, fő sereg és kitörések között osztja el.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: kitörési súlyok normál, gyenge és erős MI-állapothoz. **0–100** közötti egész számok, alapérték **0**. A meglévő védelmi, portya- és támadási súlyokkal együtt minden szint összege **100** legyen. `WeightedRoles` szükséges; a csapatlisták, időközök és létszámkeretek megmaradnak.
- `RecruitConditions`: legfeljebb **8** sorrendben vizsgált szabály, alapból üres. Az első illeszkedő szabály felülírja az erősségi szint súlyait. Mindegyikhez kell `When` és négy súly, összesen 100: `Defense`, `Raid`, `Attack`, `Sortie`. `WeightedRoles` szükséges.
- `DefRecruitComposition`: a `Native` (alapérték) megtartja a jelenlegi működést. A `PreserveSlots` a `DefUnit1..8` bejegyzései szerint tart fenn helyeket; az ismétlések növelik az adott egység arányát. Hiányzó felszerelés esetén a helyek üresen maradnak. `WeightedRoles` szükséges.
- `RecruitInitialDefenseMonths`: **0–30** hónap, alapérték **6**. Ezalatt nem toboroz portyázókat vagy a főhadseregbe, amíg a védelem hiányos. Kitörő csapatokat továbbra is toborozhat; a 0 súly változatlan. **0** kikapcsolja a várakozást. `WeightedRoles` szükséges.

A `When` feltételei: `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (bázis veszélyben), `AttackActive` (támadás folyik), `DefenseIncomplete` (hiányos védelmi létszám), `EquipmentSurplus` (felszereléstöbblet). Minden megadott feltételnek egyeznie kell; a `true` teljesülést, a `false` annak hiányát követeli meg. Az üres `When` mindig illeszkedik.

### Támadási célpontok

`AttackTargetPolicy`:

- `Inherit` (alapérték): A meglévő `TargetChoice` szerint.
- `LowestPopulation`: A legkevesebb polgár.
- `FewestTroops`: A legkevesebb katonai egység.
- `LowestCombatPower`: A legkisebb becsült katonai erő, távolságtól függetlenül.
- `Random`: Véletlen választható ellenfél, egyenlő esélyekkel.
- `LastAggressor`: A megtorlás feltételeinek megfelelő legutóbbi támadó.

`AttackTargetCommitment`:

- `Default`: új célválasztási szabálynál vagy `DuringAttack` mellett `PerAttack`; egyébként a meglévő viselkedés.
- `PerAttack`: azonos célpont az egész támadás alatt.
- `UntilDefeated`: azonos célpont több támadáson át, amíg érvényes ellenfél marad.

### Felkészülés és visszavágás

- `AttackPreparation`: `Native` (alapérték), vagy `DuringAttack`, amely támadás közben otthon toborozza a következő hullámot. A létszámkorlátok és a hullámok növekedése továbbra is érvényesek.
- `AttackActivation`: `Immediate` (alapérték), vagy `AfterProvocation`: csak kellően súlyos ellenséges támadás után indít hadjáratot vagy portyát. A védekező kitörések továbbra is elérhetők.
- `ProvocationRules`: a visszavágás küszöbei. Alapértékek: `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800**. Tartós harc a vártorony közelében vagy elegendő katonai veszteség váltja ki; a várúr sérülése azonnal számít. Mind a négy értéket add meg. **800 tick = egy játékbeli hónap**.

### Portyák

- `RaidTargetPolicy`: `Native` (alapérték), `NearestReachable` a közeli, elérhető épületekhez, vagy `Opportunistic` a távolság, célprioritás és veszély mérlegeléséhez.
- `RaidGroupCount`: **1–4**, alapérték **1**. A meglévő portyázókat osztja szét, nem toboroz több katonát.
- `RaidMinGroupSize`: **1–256**, alapérték **4**. A kisebb csoportok várnak vagy egyesülnek.
- `RaidFocus`: `Any` (alapérték), `Food` (élelmiszer), `Industry` (termelés), `HighValue` (újjáépítési költség). `Opportunistic` szükséges.
- `RaidRiskTolerance`: `Low`, `Medium` (alapérték), `High`. A közeli ellenséges csapatok és védművek vállalható veszélye.
- `RaidEnemyScope`: `PrimeTarget` (alapérték) vagy `AnyEnemy`. A fő sereg célpontját nem módosítja.

Az öt portyabeállításhoz új `RaidTargetPolicy` szükséges. A `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` és `RaidRetargetDelay` továbbra is érvényes. Visszaállítás: `AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
