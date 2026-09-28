AIC Tactics, kurulu her yapay zekânın asker alımını, saldırılarını, baskınlarını ve kuşatma araçlarını ayarlamanı sağlar. Alanları o yapay zekânın AIC dosyasına ekle. Yeni asker alımı, saldırı ve baskın kuralları isteğe bağlıdır; boş bırakılan kuşatma ve mühendis alanları modül ayarlarını kullanır. Mevcut birlik listeleri ve sınırlar geçerlidir.

### Asker alımı

- `RecruitPolicy`: `Native` mevcut asker alımını korur. `WeightedRoles` yeni askerleri savunma, baskın, ana ordu ve çıkış harekâtı arasında paylaştırır.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: üç yerel güç durumunda çıkış harekâtının payı. Savunma, baskın, saldırı ve çıkış payları her durumda toplam 100 olmalıdır.
- `RecruitConditions`: en fazla 8 sıralı kural; ilk eşleşen kural bir asker alımı için bu payların yerini alır. `When`, `Strength`, `HomeUnderThreat`, `AttackActive`, `DefenseIncomplete` ve `EquipmentSurplus` koşullarını denetleyebilir.
- `DefRecruitComposition`: `PreserveSlots`, `DefUnit1..8` oranlarını korur; `Native` önceki seçimi kullanır.
- `RecruitInitialDefenseMonths`: baskın ve ana ordunun savunma kotasını beklediği süre. 0 beklemeyi kapatır; varsayılan 6 aydır.

Çıkış payları, durum kuralları, savunma dağılımı ve ilk bekleme süresi için `WeightedRoles` gerekir. Asker alım aralıkları ve kotaları geçerliliğini korur.

### Saldırılar ve karşılık verme

- `AttackTargetPolicy`: `Inherit`, `TargetChoice` değerini kullanır. Diğerleri en az sivili (`LowestPopulation`), en az askeri (`FewestTroops`), en zayıf orduyu (`LowestCombatPower`), rastgele bir rakibi (`Random`) veya son uygun saldırganı (`LastAggressor`) seçer.
- `AttackTargetCommitment`: `PerAttack` hedefi tek saldırı boyunca, `UntilDefeated` saldırılar arasında korur. `Default`, yeni hedef politikası veya sonraki dalga hazırlığında `PerAttack`, diğer durumlarda önceki davranıştır.
- `AttackPreparation`: `DuringAttack`, ordu saldırırken sonraki dalgayı kalede hazırlar; `Native` önceki zamanlamayı korur.
- `AttackActivation`: `AfterProvocation`, ordu veya baskın başlatmadan önce yeterli düşman saldırısını bekler; `Immediate` beklemez. Savunma çıkışları kullanılabilir.
- `ProvocationRules`: `ThreatPower` ve `CombatTicks` kale yakınındaki tehdidi; `LossPower` ve `WindowTicks` kayıpları ve süreyi belirler. Lordun hasar alması hemen yeterlidir. Dört değeri birlikte gir.

### Baskınlar

- `RaidTargetPolicy`: `Native` önceki baskınları korur; `NearestReachable` yakındaki erişilebilir binaları seçer; `Opportunistic` önceliği ve tehlikeyi de değerlendirir.
- `RaidGroupCount` (1–4) mevcut baskın gücünü böler; `RaidMinGroupSize` (1–256) küçük grupların beklemesini veya birleşmesini sağlar.
- `RaidFocus`, `Any`, `Food`, `Industry` ya da `HighValue` seçer ve `Opportunistic` gerektirir. `RaidRiskTolerance` kabul edilen riski (`Low`, `Medium`, `High`) belirler. `RaidEnemyScope` baskınları ana hedefle sınırlar veya her düşmana izin verir.

`RaidTargetPolicy` sonrasındaki beş alan için yeni baskın politikası gerekir. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` ve `RaidRetargetDelay` geçerlidir.

### Kuşatma ve mühendisler

- `SafeSiegePlacement` dolu inşaat yerlerinden kaçınır (modül varsayılanı AÇIK).
- `ActualSiegeResourcePayment` malzeme ve altın ister; eksikleri yapay zekâ olağan ticaretle alır (KAPALI).
- `CoordinatedSiegeHarassment` toplanan araçları erişilebilir atış konumlarına yollar (KAPALI). `SiegeHarassMinEngines` istenen en az araç sayısıdır (0–20, varsayılan 3); `HarassingSiegeEnginesMax` toplam sınır olarak kalır.
- `LargerSiegeForces` ana saldırının ayarlı kuşatma araçlarını yeniden üretir (KAPALI). `SiegeForceMax` etkin ve planlanan araçları sınırlar (0–20, varsayılan 10); 0 tek yerel üretim grubunu korur. `AttMaxEngineers` mürettebatı sınırlar.
- `CorrectEngineerRoleCounting` atanmış mühendisleri birlik kotalarına sayar; kuşatma ve yağ görevleri ayrı kalır (AÇIK).

Kuşatma ve mühendis seçeneklerinde, AIC içinde açıkça verilen `false` veya 0 dâhil her değer modül ayarından önceliklidir. Alan boşsa modül ayarı kullanılır.
