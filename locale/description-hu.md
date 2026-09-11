Az AIC Tactics fejlesztésének célja, hogy az MI-személyiségek készítői igény szerint szabályozhassák a toborzást, a támadásokat és a portyázást.

A tervezett funkciók közé tartoznak a körülményektől függő toborzási és kitörési valószínűségek, a következő sereg előkészítése támadás közben, az ellenfél kiválasztása és a megtorlás, valamint a portyázó erők korlátozott számú csoportra osztása. A meglévő személyiségeknek meg kell őrizniük korábbi viselkedésüket, amíg készítőjük kifejezetten nem engedélyez egy új szabályt.

Az új ellenfélválasztási szabályok alapértelmezés szerint a teljes támadás során megtartják ugyanazt a célpontot (`PerAttack`). A készítők az `UntilDefeated` választásával több támadáson át is megtarthatják az ellenfelet. A módosítatlan személyiségek megőrzik korábbi viselkedésüket, beleértve a Legacy által biztosított célpontállandóságot.

**Fejlesztés alatt: még nincs játékra kész kiadás.**

## Beállítási útmutató (fejlesztési előzetes)

Ezek egy MI-személyiség AIC-konfigurációjának mezői, nem új kezelőfelületi vezérlők. A példák a fejlesztés alatt álló konfigurációt magyarázzák; nem teljes MI-csomagok és nem játékra kész beállítások. A paraméterek nevét és értékét a felület nyelvétől függetlenül pontosan így kell megadni.

### A meglévő viselkedés megőrzése

Változatlan személyiségnél hagyd el az új mezőket. Alapértékek: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. A toborzás és a célpontválasztás külön kapcsolható be.

### Ellenfél választása

Az `AttackTargetPolicy` tervezett jelentései:

| Érték | Jelentés |
| --- | --- |
| `Inherit` | A személyiség eredeti `TargetChoice` beállítását használja (alapérték). |
| `LowestPopulation` | A legkevesebb polgárral rendelkező ellenfelet részesíti előnyben. |
| `FewestTroops` | A legkevesebb katonai egységgel rendelkező ellenfelet részesíti előnyben. |
| `LowestCombatPower` | A legkisebb becsült katonai erőt részesíti előnyben, távolsági súlyozás nélkül. |
| `Random` | Minden választható ellenfél azonos esélyt kap; ismétlés lehetséges. |
| `LastAggressor` | A legutóbbi, feltételeknek megfelelő ellenséges eseményre reagál, nem minden találatra. |

Az `AttackTargetCommitment: Default` új célpontválasztási szabályoknál `PerAttack`: a támadás indításakor választ, és végig megtartja a célpontot. `Inherit` mellett megőrzi a meglévő Native/Legacy-viselkedést. Az `UntilDefeated` kifejezett választásával több támadáson át is megtartható egy érvényes ellenfél. Érvénytelen célpont esetén előbb le kell zárni a folyamatban lévő támadást; a kivonult sereg nem válthat ellenfelet támadás közben.

Példa: véletlen ellenfél támadásonként, támadás közbeni váltás nélkül:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### A toborzás elosztása

A `RecruitPolicy: WeightedRoles` a védelem, portyák, fő támadósereg és kitörések között osztja el a toborzási döntéseket. A kitörő csapatok a várból kilépve közeli ellenfelekkel harcolnak. A meglévő időközök, csapatlisták és létszámkeretek megmaradnak. A súlyok a megengedett toborzási lehetőségekre hatnak, nem garantálják a kész sereg arányait.

Mindhárom erősségi sort állítsd be: `Default`, `Weak`, `Strong`. Soronként négy egész súly kell, összesen 100. A `RecruitProbDef…`, `RecruitProbRaid…`, `RecruitProbAttack…` megmarad; a `RecruitProbSortie…` a kitörést adja hozzá, alapértéke 0. Nincs automatikus átskálázás. Példa, nem egyensúlyozási ajánlás:

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
  "RecruitProbSortieStrong": 10
}
```

### Feltételes toborzás (haladó)

A `RecruitConditions` alapból üres lista, legfeljebb nyolc sorrendben vizsgált szabállyal. Mindegyikhez kell `When` és a négy súly: `Defense`, `Raid`, `Attack`, `Sortie`, összesen 100. Az első illeszkedő szabály váltja fel az erősségi sort; találat nélkül az eredeti sor érvényes.

A `When` mezőben a `Strength` értéke `Default`, `Weak` vagy `Strong`. Igen/nem feltételek: `HomeUnderThreat` (veszélyben a bázis), `AttackActive` (támadás folyik), `DefenseIncomplete` (hiányos védelem), `EquipmentSurplus` (felszereléstöbblet). A `true` megköveteli az állapotot, a `false` annak hiányát; a kihagyott feltétel nem számít. Minden megadott feltételnek teljesülnie kell; az üres `When` mindig illeszkedik.

### Módosítás vagy visszaállítás

Az összetartozó mezőket együtt alkalmazd. A részleges frissítés megtartja a kihagyott értékeket, a korábbi `UntilDefeated` értéket is. Az automatikus célpontmegtartáshoz állíts be `Default` értéket. A szabályok kikapcsolása:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Csak a `RecruitPolicy: Native` beállítására váltva a korábbi kitörési súlyok és szabályok inaktívan megmaradnak. `Native` mellett ne adj meg kitörési mezőket vagy szabályokat. A 0 kitörési súly `WeightedRoles` módban nem állítja vissza a Native toborzást.

**Még hátravan:** játékintegráció, a fenyegetés, felszereléstöbblet és ellenfélszámlálás pontos natív meghatározása, megtorlási küszöbök, következő hullám előkészítése, osztott portyák és Legacy-opciók érvényesítése. Még nincs kiadásra kész Legacy-beállítási útmutató. A leírás nem igazolja a többjátékos, mentési vagy visszajátszási kompatibilitást.
