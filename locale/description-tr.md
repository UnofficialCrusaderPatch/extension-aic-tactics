AIC Tactics, yapay zekâ kişiliklerini hazırlayanların asker toplama, saldırı ve akın davranışlarını isteğe bağlı kurallarla yönetebilmesi için geliştirilmektedir.

Planlanan özellikler arasında koşullara bağlı asker toplama ve savunma çıkışı olasılıkları, saldırı sürerken bir sonraki ordunun hazırlanması, rakip seçimi ve misilleme ile akın kuvvetlerinin sınırlı sayıda gruba ayrılması bulunmaktadır. Mevcut kişiliklerin, hazırlayan kişi yeni bir kuralı açıkça etkinleştirmedikçe önceki davranışlarını koruması amaçlanmaktadır.

Yeni rakip seçimi kuralları, varsayılan olarak bir saldırı boyunca aynı hedefi koruyacak şekilde tasarlanmıştır (`PerAttack`). Hazırlayanlar, saldırılar arasında aynı rakibi korumak için `UntilDefeated` seçebilir. Değiştirilmemiş kişilikler, Legacy'nin sağladığı hedef kararlılığı da dâhil olmak üzere mevcut davranışlarını korur.

**Geliştirme aşamasında: henüz oynamaya hazır bir sürüm yoktur.**

## Kurulum kılavuzu (geliştirme önizlemesi)

Bunlar bir yapay zekâ kişiliğinin AIC yapılandırmasındaki alanlardır; yeni arayüz kontrolleri değildir. Örnekler geliştirilmekte olan yapılandırmayı açıklar; tam yapay zekâ paketleri veya oynamaya hazır ayarlar değildir. Arayüz dili ne olursa olsun parametre adlarını ve değerlerini aynen kullanın.

### Mevcut davranışı koruma

Değiştirilmemiş bir kişilikte yeni alanları eklemeyin. Varsayılanlar: `RecruitPolicy: Native`, `AttackTargetPolicy: Inherit`, `AttackTargetCommitment: Default`. Asker alımı ve hedef seçimi ayrı ayrı etkinleştirilebilir.

### Rakip seçme

`AttackTargetPolicy` için amaçlanan anlamlar:

| Değer | Anlam |
| --- | --- |
| `Inherit` | Kişiliğin özgün `TargetChoice` ayarını kullanır (varsayılan). |
| `LowestPopulation` | En az sivili olan rakibi tercih eder. |
| `FewestTroops` | En az askerî birimi olan rakibi tercih eder. |
| `LowestCombatPower` | Mesafe ağırlığı kullanmadan tahminî askerî gücü en düşük rakibi tercih eder. |
| `Random` | Uygun her rakibe eşit şans verir; aynı rakip tekrar seçilebilir. |
| `LastAggressor` | Her darbeye değil, koşulları karşılayan son düşmanca olaya tepki verir. |

`AttackTargetCommitment: Default`, yeni hedef politikalarında `PerAttack` demektir: hedef saldırı başlarken seçilir ve saldırı boyunca korunur. `Inherit` ile mevcut Native/Legacy davranışı korunur. Geçerli bir rakibi birden fazla saldırı boyunca tutmak için açıkça `UntilDefeated` seçin. Hedef geçersiz olursa yenisi seçilmeden önce mevcut saldırının sonlandırılması gerekir; sahadaki ordu saldırı ortasında rakip değiştirmez.

Örnek: her saldırıda rastgele rakip, saldırı sırasında değişiklik yok:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### Asker alımını dağıtma

`RecruitPolicy: WeightedRoles`, asker alımı seçimlerini savunma, akınlar, ana saldırı ordusu ve huruç birlikleri arasında paylaştırır. Huruç birlikleri kaleden çıkarak yakındaki düşmanlarla savaşır. Mevcut alım aralıkları, birlik listeleri ve kotalar geçerliliğini korur. Ağırlıklar uygun alım seçeneklerini etkiler; tamamlanan orduda sabit bir oran garanti etmez.

Üç güç satırını da ayarlayın: `Default`, `Weak`, `Strong`. Her satırdaki dört tam sayı ağırlığın toplamı 100 olmalıdır. Mevcut `RecruitProbDef…`, `RecruitProbRaid…`, `RecruitProbAttack…` alanları kullanılır; `RecruitProbSortie…` huruçları ekler ve varsayılanı 0'dır. Otomatik yeniden ölçekleme yapılmaz. Örnek, denge önerisi değildir:

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

### Koşullu asker alımı (gelişmiş)

`RecruitConditions` varsayılan olarak boş listedir; en fazla sekiz sıralı kural alır. Her kural `When` ve toplamı 100 olan dört ağırlığı gerektirir: `Defense`, `Raid`, `Attack`, `Sortie`. İlk eşleşen kural güç satırının yerine geçer; eşleşme yoksa o satır kullanılır.

`When` içindeki `Strength`, `Default`, `Weak` veya `Strong` seçer. Evet/hayır denetimleri: `HomeUnderThreat` (üs tehdit altında), `AttackActive` (saldırı sürüyor), `DefenseIncomplete` (savunma eksik), `EquipmentSurplus` (fazla teçhizat var). `true` koşulun varlığını, `false` yokluğunu gerektirir; yazılmayan denetimler dikkate alınmaz. Yazılanların tümü eşleşmelidir; boş `When` her zaman eşleşir.

### Ayarları değiştirme veya geri alma

İlgili alanları birlikte uygulayın. Kısmi güncellemeler önceki `UntilDefeated` dâhil yazılmayan değerleri korur. Otomatik hedef bağlılığını geri getirmek için `Default` kullanın. Bu politikaları kapatmak için:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

Yalnızca `RecruitPolicy: Native` ayarına geçmek, önceki huruç ağırlıklarını ve kuralları devre dışı değerler olarak saklar. `Native` kullanırken huruç alanlarını veya kuralları ayarlamayın. `WeightedRoles` içinde huruç ağırlığının 0 olması Native asker alımını geri getirmez.

**Henüz tamamlanmadı:** oyun entegrasyonu, tehdidin, teçhizat fazlasının ve rakip sayımlarının kesin yerel tanımları, misilleme eşikleri, sonraki dalganın hazırlanması, bölünmüş akınlar ve Legacy seçeneklerinin zorunlu uygulanması. Henüz yayıma hazır bir Legacy ayar tarifi yoktur. Bu açıklamalar çok oyunculu, kayıt veya tekrar oynatma uyumluluğunu kanıtlamaz.
