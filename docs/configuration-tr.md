Yapay zekânızın nasıl asker toplayacağını ve saldırı hedeflerini nasıl seçeceğini belirleyin.

İstediğiniz alanları AIC yapılandırmanıza ekleyin. Yeni ayar eklenmeyen kişilikler mevcut davranışlarını korur.

### Saldırı hedefleri

| `AttackTargetPolicy` | Hedef seçimi |
| --- | --- |
| `Inherit` (varsayılan) | Mevcut `TargetChoice` ayarına göre. |
| `LowestPopulation` | En az sivili olan rakip. |
| `FewestTroops` | En az askerî birimi olan rakip. |
| `LowestCombatPower` | Mesafeden bağımsız olarak tahminî askerî gücü en düşük rakip. |
| `Random` | Uygun rakipler arasından eşit olasılıkla rastgele seçim. |
| `LastAggressor` | Saldırısı misilleme koşullarını karşılayan son saldırgan. |

`AttackTargetCommitment`, aynı rakibin ne kadar süre hedef tutulacağını belirler:

- `Default`: yeni hedef politikalarında veya `DuringAttack` ile `PerAttack`; diğer durumlarda mevcut davranış.
- `PerAttack`: saldırı başlarken seç ve saldırı boyunca aynı hedefi koru.
- `UntilDefeated`: geçerli hedef olduğu sürece aynı rakibi sonraki saldırılarda da koru.

Her saldırıda rastgele rakip; aynı rakip yeniden seçilebilir:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Asker alımı

`RecruitPolicy` varsayılan olarak `Native` kullanır. Asker alımını savunma, akınlar, ana saldırı ordusu ve kaleden çıkış birlikleri arasında dağıtmak için `WeightedRoles` seçin. Birlik listeleri, alım aralıkları ve kotalar geçerliliğini korur.

**Her** güç düzeyinde (`Default`, `Weak`, `Strong`) dört tam sayı ağırlığın toplamı **100** olmalıdır. `RecruitProbSortie…` varsayılan olarak 0'dır. Bu ağırlıklar asker alımını yönlendirir; ordunun son bileşiminde sabit oranlar garanti etmez.

Örnek: yapay zekâ zayıfken daha fazla savunma; savunma kadrosu dolana kadar savunmaya öncelik:

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

`RecruitConditions` isteğe bağlıdır (varsayılan: `[]`, en fazla sekiz kural). İlk eşleşen kural, mevcut güç düzeyinin ağırlıklarını değiştirir. Her kural toplamı 100 olan dört ağırlığı da içermelidir.

`When` içinde güç düzeyini `Strength` ile belirtebilir veya şu koşulları kullanabilirsiniz:

| Koşul | Denetlenen durum |
| --- | --- |
| `HomeUnderThreat` | Üs tehdit altında. |
| `AttackActive` | Saldırı sürüyor. |
| `DefenseIncomplete` | Savunma kadrosu henüz dolmadı. |
| `EquipmentSurplus` | Fazladan teçhizat var. |

`true` koşulun varlığını, `false` yokluğunu gerektirir. Belirtilen koşulların tümü karşılanmalıdır. Yazılmayan koşullar dikkate alınmaz; boş `When` her zaman eşleşir.

### Ayarları değiştirme

İlgili alanları birlikte uygulayın. Kısmi güncellemeler yazılmayan değerleri korur; önceki `UntilDefeated` seçimini kaldırmak için `Default` kullanın. İki sistemi de mevcut davranışlarına döndürmek için:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Önceki çıkış ağırlıkları ve kurallar `Native` altında saklanır ancak devre dışıdır; bunları bu güncellemeye eklemeyin. `WeightedRoles` altında çıkış ağırlığının 0 olması `Native` davranışını geri getirmez.

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
