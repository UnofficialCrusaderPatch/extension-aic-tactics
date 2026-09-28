AIC Tactics, kurulu her yapay zekânın asker alımını, saldırılarını, baskınlarını ve kuşatma araçlarını ayarlamanı sağlar. Alanları o yapay zekânın AIC dosyasına ekle. Yeni asker alımı, saldırı ve baskın kuralları isteğe bağlıdır; boş bırakılan kuşatma alanları modül ayarlarını kullanır. Mevcut birlik listeleri ve sınırlar geçerlidir.

### Asker alımı

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: her güç durumundaki çıkış payı. Bunlardan birini ayarlamak, bu yapay zekâ için dört asker alım payını etkinleştirir. Savunma, baskın, saldırı ve çıkış payları her durumda toplam 100 olmalıdır.
- `RecruitConditions`: duruma göre farklı paylar. Örneğin kale tehdit altındaysa %70 savunma, %20 saldırı, %10 çıkış. Her asker alımında ilk eşleşen kural uygulanır.
- `RecruitPolicy`: isteğe bağlı uyumluluk ayarı. `Native` özgün seçimi korur; `WeightedRoles` dört payı açıkça etkinleştirir.
- `DefRecruitComposition`: `PreserveSlots`, `DefUnit1..8` oranlarını korur; `Native` önceki seçimi kullanır.
- `RecruitInitialDefenseMonths`: baskın ve ana ordunun savunma kotasını beklediği süre. 0 beklemeyi kapatır; varsayılan 6 aydır.

Çıkış payı `WeightedRoles` ayarını otomatik açar. Durum kuralları, savunma dağılımı ve `RecruitInitialDefenseMonths` için bunu açıkça seçmek gerekir. Asker alım aralıkları ve kotaları geçerliliğini korur.

### Saldırılar ve karşılık verme

- `AttackTargetPolicy`: `Inherit`, `TargetChoice` değerini kullanır. `LowestPopulation`, `FewestTroops`, `LowestCombatPower`, `Random` ve `LastAggressor` her saldırı için bir hedef seçer.
- `AttackTargetCommitment`: eski AIC'lerde geçerlidir. Yeni AIC'lerde seçim ve süreyi birlikte `AttackTargetPolicy` içinde yaz: `{ "Choice": "LastAggressor", "UntilDefeated": true }`.
- `AttackPreparation`: `DuringAttack`, ordu saldırırken sonraki dalgayı kalede hazırlar; `Native` önceki zamanlamayı korur.
- `AttackActivation`: `AfterProvocation`, ordu veya baskın başlatmadan önce yeterli düşman saldırısını bekler; `Immediate` beklemez. Savunma çıkışları kullanılabilir.
- `ProvocationRules`: eski AIC'ler, `AfterProvocation` ve asker alımındaki tehdit kuralları için geçerlidir. Yeni AIC'lerde `LastAggressor` eşiklerini `AttackTargetPolicy.Provocation` içinde yazabilirsin.

### Baskınlar

- `RaidTargetPolicy`: `Native` önceki baskınları korur; `NearestReachable` yakın erişilebilir binaları seçer; `Opportunistic` tercih ve tehlikeyi değerlendirir; `RandomNearby` erişilebilir bir binayı rastgele seçip yakınındakileri temizler.
- `RaidGroupCount` (1–4) mevcut baskın gücünü böler; `RaidMinGroupSize` (1–256) küçük grupların beklemesini veya birleşmesini sağlar.
- `RaidFocus`, `Any`, `Food`, `Industry`, `HighValue` veya `{ "Food": 60, "Industry": 20 }` gibi yüzdeleri kabul eder; kalan pay her binaya açıktır. `Opportunistic` ve `RandomNearby` ile çalışır. `RaidRiskTolerance` riski, `RaidEnemyScope` hedef düşmanı belirler.

`RaidTargetPolicy` sonrasındaki beş alan için yeni baskın politikası gerekir. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` ve `RaidRetargetDelay` geçerlidir.

### Kuşatma ve mühendisler

- `SafeSiegePlacement` dolu inşaat yerlerinden kaçınır (modül varsayılanı AÇIK).
- `ActualSiegeResourcePayment` malzeme ve altın ister; eksikleri yapay zekâ olağan ticaretle alır (KAPALI).
- `CoordinatedSiegeHarassment` toplanan araçları erişilebilir atış konumlarına yollar (KAPALI). `SiegeHarassMinEngines` istenen en az araç sayısıdır (0–20, varsayılan 3); `HarassingSiegeEnginesMax` toplam sınır olarak kalır.
- `LargerSiegeForces`, canlı ve boş saldırı mühendisleri ile geçerli yerler kaldıkça ayarlı araçları üretir (KAPALI). `SiegeForceMax` isteğe bağlı üst sınırdır (0–64); 0'da sayı mühendislerle belirlenir. `AttMaxEngineers` mürettebatı sınırlar.

Gelişmiş kuşatma ayarları güvenli yerleşimi otomatik açar. Mühendis rol sayımı varsayılan olarak açıktır; Fixed Engineers mürettebat ve araçların yaşam döngüsünü yönetir.

AIC'de belirtilen kuşatma ayarları modül varsayılanlarından önceliklidir.
