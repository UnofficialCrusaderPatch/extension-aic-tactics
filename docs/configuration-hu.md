Állítsd be, hogyan toborozzon és válasszon támadási célpontot az MI-d.

Add hozzá a kívánt mezőket az AIC-konfigurációhoz. Új beállítások nélkül a meglévő személyiségek viselkedése megmarad.

### Támadási célpontok

| `AttackTargetPolicy` | Célpont |
| --- | --- |
| `Inherit` (alapérték) | A meglévő `TargetChoice` szerint. |
| `LowestPopulation` | A legkevesebb polgárral rendelkező ellenfél. |
| `FewestTroops` | A legkevesebb katonai egységgel rendelkező ellenfél. |
| `LowestCombatPower` | A legkisebb becsült katonai erő, távolságtól függetlenül. |
| `Random` | Véletlen választható ellenfél, egyenlő esélyekkel. |
| `LastAggressor` | A legutóbbi támadó, akinek támadása teljesíti a megtorlás feltételeit. |

Az `AttackTargetCommitment` szabja meg, meddig maradjon ugyanaz az ellenfél:

- `Default`: új szabályoknál `PerAttack`; `Inherit` mellett a meglévő viselkedés.
- `PerAttack`: választás a támadás kezdetén, azonos célpont az egész támadás alatt.
- `UntilDefeated`: azonos ellenfél több támadáson át, amíg érvényes célpont marad.

Véletlen ellenfél támadásonként; ugyanaz újra kiválasztható:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Toborzás

A `RecruitPolicy` alapértéke `Native`. A `WeightedRoles` a védelem, portyák, fő támadósereg és várból indított kitörések között osztja el a toborzást. A csapatlisták, időközök és létszámkeretek megmaradnak.

**Mindegyik** erősségi szinten (`Default`, `Weak`, `Strong`) a négy egész súly összege **100** legyen. A `RecruitProbSortie…` alapértéke 0. Ezek a toborzás súlyai, nem a kész sereg garantált arányai.

Példa: gyenge MI esetén több védelem; a védelem előnyt élvez, amíg nem teljes a létszáma:

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

A `RecruitConditions` opcionális (alapérték: `[]`, legfeljebb nyolc szabály). Az első illeszkedő szabály felülírja az aktuális erősségi szint súlyait. Minden szabályhoz mind a négy súly szükséges, összesen 100.

A `When` mezőben a `Strength` adja meg az erősségi szintet. További feltételek:

| Feltétel | Azt vizsgálja, hogy… |
| --- | --- |
| `HomeUnderThreat` | Veszélyben van-e a bázis. |
| `AttackActive` | Folyik-e támadás. |
| `DefenseIncomplete` | Hiányos-e a védelem létszáma. |
| `EquipmentSurplus` | Van-e felszereléstöbblet. |

A `true` a feltétel teljesülését, a `false` annak hiányát követeli meg. Minden megadott feltételnek egyeznie kell. A kihagyott feltételek nem számítanak; az üres `When` mindig illeszkedik.

### Beállítások módosítása

Az összetartozó mezőket együtt alkalmazd. A részleges frissítés megtartja a kihagyott értékeket; a korábbi `UntilDefeated` feloldásához használj `Default` értéket. Mindkét rendszer eredeti viselkedésének visszaállítása:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

A korábbi kitörési súlyok és szabályok `Native` mellett inaktívan megmaradnak; ezeket ne add meg ebben a frissítésben. A 0 kitörési súly `WeightedRoles` mellett nem állítja vissza a `Native` módot.
