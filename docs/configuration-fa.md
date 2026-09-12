روش سربازگیری و انتخاب هدف‌های حملهٔ هوش مصنوعی خود را تنظیم کنید.

فیلدهای دلخواه را به پیکربندی AIC اضافه کنید. بدون تنظیمات جدید، رفتار شخصیت‌های موجود تغییر نمی‌کند.

### هدف‌های حمله

| `AttackTargetPolicy` | انتخاب هدف |
| --- | --- |
| `Inherit` (پیش‌فرض) | طبق `TargetChoice` فعلی. |
| `LowestPopulation` | حریف با کمترین جمعیت غیرنظامی. |
| `FewestTroops` | حریف با کمترین تعداد واحد نظامی. |
| `LowestCombatPower` | کمترین قدرت نظامی تخمینی، بدون در نظر گرفتن فاصله. |
| `Random` | انتخاب تصادفی از حریف‌های واجد شرایط، با شانس برابر. |
| `LastAggressor` | آخرین مهاجمی که حمله‌اش شرایط اقدام تلافی‌جویانه را دارد. |

`AttackTargetCommitment` تعیین می‌کند همان حریف تا چه زمانی هدف بماند:

- `Default`: برای سیاست‌های جدید، `PerAttack`؛ همراه `Inherit`، رفتار فعلی.
- `PerAttack`: انتخاب هدف در آغاز حمله و حفظ آن تا پایان همان حمله.
- `UntilDefeated`: حفظ همان حریف در چند حمله، تا زمانی که هدف معتبری باشد.

حریف تصادفی برای هر حمله؛ ممکن است همان حریف دوباره انتخاب شود:

```json
{
  "AttackTargetPolicy": "Random",
  "AttackTargetCommitment": "Default"
}
```

### سربازگیری

پیش‌فرض `RecruitPolicy` برابر `Native` است. با `WeightedRoles` سربازگیری را میان دفاع، یورش، ارتش اصلی حمله و نیروهای خروج از قلعه تقسیم کنید. فهرست نیروها، فاصله‌های زمانی و سهمیه‌ها همچنان اعمال می‌شوند.

برای **هر** سطح قدرت (`Default`، `Weak`، `Strong`)، مجموع چهار وزن صحیح باید **۱۰۰** باشد. پیش‌فرض `RecruitProbSortie…` برابر ۰ است. وزن‌ها انتخاب‌های سربازگیری را هدایت می‌کنند، نه نسبت قطعی نیروهای ارتش را.

مثال: دفاع بیشتر هنگام ضعف هوش مصنوعی؛ اولویت با دفاع تا تکمیل نیروهای آن:

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

`RecruitConditions` اختیاری است (پیش‌فرض: `[]`، حداکثر هشت قانون). نخستین قانون منطبق جای وزن‌های سطح قدرت فعلی را می‌گیرد. هر قانون باید هر چهار وزن را با مجموع ۱۰۰ داشته باشد.

در `When`، سطح قدرت را با `Strength` مشخص کنید یا این شرط‌ها را به کار ببرید:

| شرط | بررسی می‌کند که… |
| --- | --- |
| `HomeUnderThreat` | پایگاه در خطر است. |
| `AttackActive` | حمله در جریان است. |
| `DefenseIncomplete` | نیروهای دفاعی هنوز تکمیل نشده‌اند. |
| `EquipmentSurplus` | تجهیزات اضافی وجود دارد. |

`true` برقرار بودن شرط و `false` برقرار نبودن آن را لازم می‌داند. همهٔ شرط‌های نوشته‌شده باید مطابقت داشته باشند. شرط‌های حذف‌شده نادیده گرفته می‌شوند؛ `When` خالی همیشه منطبق است.

### تغییر تنظیمات

فیلدهای مرتبط را با هم تغییر دهید. به‌روزرسانی جزئی مقادیر ذکرنشده را حفظ می‌کند؛ برای لغو `UntilDefeated` قبلی، `Default` را قرار دهید. بازگرداندن هر دو سیستم به رفتار قبلی:

```json
{
  "RecruitPolicy": "Native",
  "AttackTargetPolicy": "Inherit",
  "AttackTargetCommitment": "Default"
}
```

وزن‌های خروج و قوانین قبلی در حالت `Native` ذخیره می‌مانند ولی غیرفعال‌اند؛ آن‌ها را در این به‌روزرسانی وارد نکنید. وزن خروج ۰ در `WeightedRoles` حالت `Native` را برنمی‌گرداند.
