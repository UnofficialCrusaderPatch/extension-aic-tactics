Toborzási és célpontválasztási beállításokkal bővíti az AIC-t. Új beállítások nélkül az MI viselkedése megmarad.

### Toborzás

- `RecruitPolicy`: a `Native` (alapérték) megtartja a meglévő toborzást. A `WeightedRoles` a védelem, portyák, fő sereg és kitörések között osztja el.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: kitörési súlyok normál, gyenge és erős MI-állapothoz. **0–100** közötti egész számok, alapérték **0**. A meglévő védelmi, portya- és támadási súlyokkal együtt minden szint összege **100** legyen. `WeightedRoles` szükséges; a csapatlisták, időközök és létszámkeretek megmaradnak.
- `RecruitConditions`: legfeljebb **8** sorrendben vizsgált szabály, alapból üres. Az első illeszkedő szabály felülírja az erősségi szint súlyait. Mindegyikhez kell `When` és négy súly, összesen 100: `Defense`, `Raid`, `Attack`, `Sortie`. `WeightedRoles` szükséges.

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

- `Default`: új szabályoknál `PerAttack`; `Inherit` mellett a meglévő viselkedés.
- `PerAttack`: azonos célpont az egész támadás alatt.
- `UntilDefeated`: azonos célpont több támadáson át, amíg érvényes ellenfél marad.

A részleges frissítés megtartja a kihagyott értékeket. Visszatérés a meglévő viselkedéshez: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. A korábbi kitörési súlyok és szabályok inaktívan megmaradnak; a `Native` módra váltáskor ne add meg őket.
