AIC'nize asker alımı ve saldırı hedefi ayarları ekler. Yeni ayar eklenmezse mevcut yapay zekâ davranışı korunur.

### Asker alımı

- `RecruitPolicy`: `Native` (varsayılan) mevcut asker alımını korur. `WeightedRoles` alımı savunma, akınlar, ana ordu ve kaleden çıkış birlikleri arasında dağıtır.
- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: normal, zayıf ve güçlü yapay zekâ durumları için çıkış ağırlıkları. **0–100** arası tam sayı, varsayılan **0**. Mevcut savunma, akın ve saldırı ağırlıklarıyla birlikte her düzeyin toplamı **100** olmalıdır. `WeightedRoles` gerektirir; birlik listeleri, aralıklar ve kotalar korunur.
- `RecruitConditions`: en fazla **8** sıralı kural; varsayılan olarak boş. İlk eşleşen kural güç düzeyinin ağırlıklarını değiştirir. Her kural `When` ve toplamı 100 olan dört ağırlık içerir: `Defense`, `Raid`, `Attack`, `Sortie`. `WeightedRoles` gerektirir.

`When` koşulları: `Strength` (`Default`, `Weak`, `Strong`), `HomeUnderThreat` (üs tehdit altında), `AttackActive` (saldırı sürüyor), `DefenseIncomplete` (savunma kadrosu eksik), `EquipmentSurplus` (fazla teçhizat var). Belirtilen tüm koşullar karşılanmalıdır; `true` koşulun varlığını, `false` yokluğunu gerektirir. Boş `When` her zaman eşleşir.

### Saldırı hedefleri

| `AttackTargetPolicy` | Hedef seçimi |
| --- | --- |
| `Inherit` (varsayılan) | Mevcut `TargetChoice` ayarına göre. |
| `LowestPopulation` | En az sivil. |
| `FewestTroops` | En az askerî birim. |
| `LowestCombatPower` | Mesafeden bağımsız en düşük tahminî askerî güç. |
| `Random` | Uygun rakipler arasından eşit olasılıkla rastgele seçim. |
| `LastAggressor` | Misilleme koşullarını karşılayan son saldırgan. |

`AttackTargetCommitment`:

- `Default`: yeni politikalarda `PerAttack`; `Inherit` ile mevcut davranış.
- `PerAttack`: saldırı boyunca aynı hedefi korur.
- `UntilDefeated`: geçerli bir rakip olduğu sürece sonraki saldırılarda da aynı hedefi korur.

Kısmi güncellemeler yazılmayan değerleri korur. Mevcut davranışa dönmek için: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. Önceki çıkış ağırlıkları ve kurallar devre dışı olarak saklanır; `Native` ayarına geçerken bunları eklemeyin.
