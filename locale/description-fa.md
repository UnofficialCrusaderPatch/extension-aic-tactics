تنظیمات سربازگیری و انتخاب هدف را به AIC اضافه می‌کند. بدون تنظیمات جدید، رفتار فعلی هوش مصنوعی حفظ می‌شود.

### سربازگیری

- `RecruitPolicy`: مقدار `Native` (پیش‌فرض) سربازگیری فعلی را نگه می‌دارد. `WeightedRoles` آن را میان دفاع، یورش، ارتش اصلی و خروج از قلعه تقسیم می‌کند.
- `RecruitProbSortieDefault`، `RecruitProbSortieWeak`، `RecruitProbSortieStrong`: وزن خروج از قلعه در وضعیت عادی، ضعیف و قوی هوش مصنوعی. اعداد صحیح **۰ تا ۱۰۰**، پیش‌فرض **۰**. با وزن‌های فعلی دفاع، یورش و حمله، مجموع هر سطح باید **۱۰۰** باشد. نیازمند `WeightedRoles`؛ فهرست نیروها، فاصله‌های زمانی و سهمیه‌ها حفظ می‌شوند.
- `RecruitConditions`: حداکثر **۸** قانون مرتب، پیش‌فرض خالی. نخستین قانون منطبق جای وزن‌های سطح قدرت را می‌گیرد. هر قانون شامل `When` و چهار وزن با مجموع ۱۰۰ است: `Defense`، `Raid`، `Attack`، `Sortie`. نیازمند `WeightedRoles`.

شرط‌های `When`: مقدار `Strength` (`Default`، `Weak`، `Strong`)، `HomeUnderThreat` (پایگاه در خطر)، `AttackActive` (حمله در جریان)، `DefenseIncomplete` (دفاع تکمیل نشده) و `EquipmentSurplus` (تجهیزات اضافی). همهٔ شرط‌های نوشته‌شده باید منطبق باشند؛ `true` برقرار بودن شرط و `false` برقرار نبودن آن را لازم می‌داند. `When` خالی همیشه منطبق است.

### هدف‌های حمله

`AttackTargetPolicy`:

- `Inherit` (پیش‌فرض): طبق `TargetChoice` فعلی.
- `LowestPopulation`: کمترین جمعیت غیرنظامی.
- `FewestTroops`: کمترین تعداد واحد نظامی.
- `LowestCombatPower`: کمترین قدرت نظامی تخمینی، بدون در نظر گرفتن فاصله.
- `Random`: انتخاب تصادفی از حریف‌های واجد شرایط، با شانس برابر.
- `LastAggressor`: آخرین مهاجم واجد شرایط اقدام تلافی‌جویانه.

`AttackTargetCommitment`:

- `Default`: برای سیاست‌های جدید، `PerAttack`؛ همراه `Inherit`، رفتار فعلی.
- `PerAttack`: حفظ همان هدف تا پایان حمله.
- `UntilDefeated`: حفظ هدف در چند حمله، تا زمانی که حریف معتبری باشد.

به‌روزرسانی جزئی مقادیر ذکرنشده را حفظ می‌کند. بازگشت به رفتار قبلی: `RecruitPolicy: Native`، `AttackTargetPolicy: Inherit`، `AttackTargetCommitment: Default`. وزن‌های خروج و قوانین قبلی به‌صورت غیرفعال ذخیره می‌مانند؛ هنگام تغییر به `Native` آن‌ها را وارد نکنید.
