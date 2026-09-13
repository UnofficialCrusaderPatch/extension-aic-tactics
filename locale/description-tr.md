Yapay zekânın asker alımını, dalga hazırlığını, saldırı hedeflerini ve akınlarını AIC üzerinden ayarlayın. Yeni ayarlar olmadan mevcut davranış korunur.

### Asker alımı

- `RecruitPolicy`: `Native` (varsayılan) mevcut asker alımını korur. `WeightedRoles` alımı savunma, akınlar, ana ordu ve kaleden çıkış birlikleri arasında dağıtır.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: normal, zayıf ve güçlü yapay zekâ durumları için çıkış ağırlıkları. **0–100** arası tam sayı, varsayılan **0**. Mevcut savunma, akın ve saldırı ağırlıklarıyla birlikte her düzeyin toplamı **100** olmalıdır. `WeightedRoles` gerektirir; birlik listeleri, aralıklar ve kotalar korunur.
- `RecruitConditions`: en fazla **8** sıralı kural; varsayılan olarak boş. İlk eşleşen kural güç düzeyinin ağırlıklarını değiştirir. Her kural `When` ve toplamı 100 olan dört ağırlık içerir: `Defense`, `Raid`, `Attack`, `Sortie`. `WeightedRoles` gerektirir.
- `DefRecruitComposition`: `Native` (varsayılan) mevcut davranışı korur. `PreserveSlots`, `DefUnit1..8` girişlerinin paylarını ayırır; tekrarlanan girişler o birimin payını artırır. Ekipman yoksa yerler boş kalır. `WeightedRoles` gerektirir.
- `RecruitInitialDefenseMonths`: **0–30** ay, varsayılan **6**. Bu süre boyunca savunma kotası dolmadıysa akıncı ve ana ordu askerlerinin alımını erteler. Çıkış birlikleri alınabilir; 0 ağırlıklı roller kapalı kalır. **0** beklemeyi kapatır. `WeightedRoles` gerektirir.

`When` koşulları: `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (üs tehdit altında), `AttackActive` (saldırı sürüyor), `DefenseIncomplete` (savunma kadrosu eksik), `EquipmentSurplus` (fazla teçhizat var). Belirtilen tüm koşullar karşılanmalıdır; `true` koşulun varlığını, `false` yokluğunu gerektirir. Boş `When` her zaman eşleşir.

### Saldırı hedefleri

`AttackTargetPolicy`:

- `Inherit` (varsayılan): Mevcut `TargetChoice` ayarına göre.
- `LowestPopulation`: En az sivil.
- `FewestTroops`: En az askerî birim.
- `LowestCombatPower`: Mesafeden bağımsız en düşük tahminî askerî güç.
- `Random`: Uygun rakipler arasından eşit olasılıkla rastgele seçim.
- `LastAggressor`: Misilleme koşullarını karşılayan son saldırgan.

`AttackTargetCommitment`:

- `Default`: yeni hedef politikalarında veya `DuringAttack` ile `PerAttack`; diğer durumlarda mevcut davranış.
- `PerAttack`: saldırı boyunca aynı hedefi korur.
- `UntilDefeated`: geçerli bir rakip olduğu sürece sonraki saldırılarda da aynı hedefi korur.

### Hazırlık ve misilleme

- `AttackPreparation`: `Native` (varsayılan) veya mevcut ordu saldırırken sonraki dalgayı kalede hazırlayan `DuringAttack`. Birlik sınırları ve dalga artışı geçerliliğini korur.
- `AttackActivation`: `Immediate` (varsayılan) veya yeterince ciddi bir saldırıdan sonra ordu ve akın başlatan `AfterProvocation`. Savunma çıkışları kullanılabilir.
- `ProvocationRules`: misilleme eşikleri. Varsayılanlar: `ThreatPower` **100**, `CombatTicks` **200**, `LossPower` **100**, `WindowTicks` **800**. İç kale yakınındaki uzun çatışmalar veya yeterli asker kaybı misillemeyi tetikler; lordun hasar alması hemen yeterlidir. Dört değeri birlikte belirtin. **800 tick = bir oyun ayı**.

### Akınlar

- `RaidTargetPolicy`: `Native` (varsayılan), yakındaki erişilebilir binalar için `NearestReachable` veya mesafe, öncelik ve tehlikeyi değerlendiren `Opportunistic`.
- `RaidGroupCount`: **1–4**, varsayılan **1**. Mevcut akın kuvvetini böler; fazladan asker toplamaz.
- `RaidMinGroupSize`: **1–256**, varsayılan **4**. Küçük gruplar bekler veya birleşir.
- `RaidFocus`: `Any` (varsayılan), `Food` (gıda), `Industry` (üretim), `HighValue` (yeniden yapım maliyeti). `Opportunistic` gerektirir.
- `RaidRiskTolerance`: `Low`, `Medium` (varsayılan), `High`. Yakındaki düşman birlikleri ve savunmalarına karşı risk toleransı.
- `RaidEnemyScope`: `PrimeTarget` (varsayılan) veya `AnyEnemy`. Ana ordunun hedefini değiştirmez.

Bu beş akın ayarı yeni bir `RaidTargetPolicy` gerektirir. `RaidUnitsBase`, `RaidUnitsRandom`, `RaidUnit1..8` ve `RaidRetargetDelay` geçerlidir. Varsayılana dönüş: `AttackPreparation: Native`, `AttackActivation: Immediate`, `RaidTargetPolicy: Native`.
