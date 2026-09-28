AIC Tactics سربازگیری، حمله، یورش و ابزار محاصره را برای هر هوش مصنوعی نصب‌شده جداگانه تنظیم می‌کند. این فیلدها را در AIC همان هوش مصنوعی بنویسید. قوانین تازهٔ سربازگیری، حمله و یورش اختیاری‌اند؛ فیلدهای محاصره در صورت حذف از تنظیمات ماژول پیروی می‌کنند. فهرست و سقف نیروهای موجود همچنان معتبر است.

### سربازگیری

- `RecruitProbSortieDefault`, `RecruitProbSortieWeak`, `RecruitProbSortieStrong`: سهم خروج دفاعی در هر وضعیت قدرت. تعیین هرکدام، چهار سهم سربازگیری را برای این هوش مصنوعی فعال می‌کند. مجموع سهم دفاع، یورش، حمله و خروج در هر وضعیت باید ۱۰۰ باشد.
- `RecruitConditions`: سهم‌های متفاوت برای موقعیت‌های خاص. مثلاً هنگام تهدید قلعه: ۷۰٪ دفاع، ۲۰٪ حمله و ۱۰٪ خروج. در هر نوبت سربازگیری نخستین قانون منطبق اعمال می‌شود.
- `RecruitPolicy`: گزینهٔ اختیاری سازگاری. `Native` انتخاب اصلی را نگه می‌دارد؛ `WeightedRoles` چهار سهم را صریحاً فعال می‌کند.
- `DefRecruitComposition`: `PreserveSlots` نسبت‌های `DefUnit1..8` را حفظ می‌کند؛ `Native` انتخاب قبلی را به کار می‌برد.
- `RecruitInitialDefenseMonths`: مدت انتظار یورش و ارتش اصلی تا تکمیل سهمیهٔ دفاع. ۰ انتظار را خاموش می‌کند؛ پیش‌فرض ۶ ماه است.

سهم خروج دفاعی `WeightedRoles` را خودکار فعال می‌کند. برای قوانین موقعیتی، ترکیب دفاع و `RecruitInitialDefenseMonths` باید آن را صریحاً انتخاب کرد. فاصله‌های زمانی و سهمیه‌های سربازگیری همچنان برقرارند.

### حمله و مقابله‌به‌مثل

- `AttackTargetPolicy`: `Inherit` از `TargetChoice` استفاده می‌کند. `LowestPopulation`، `FewestTroops`، `LowestCombatPower`، `Random` و `LastAggressor` برای هر حمله یک هدف انتخاب می‌کنند.
- `AttackTargetCommitment`: در AICهای قدیمی همچنان معتبر است. در AIC تازه، انتخاب هدف و مدت آن را با هم در `AttackTargetPolicy` بنویسید: `{ "Choice": "LastAggressor", "UntilDefeated": true }`.
- `AttackPreparation`: `DuringAttack` هنگام حملهٔ ارتش، موج بعدی را در قلعه آماده می‌کند؛ `Native` زمان‌بندی قبلی را نگه می‌دارد.
- `AttackActivation`: `AfterProvocation` پیش از حملهٔ ارتش یا یورش منتظر حملهٔ کافی دشمن می‌ماند؛ `Immediate` منتظر نمی‌ماند. خروج دفاعی همچنان ممکن است.
- `ProvocationRules`: برای AICهای قدیمی، `AfterProvocation` و شرط تهدید هنگام سربازگیری باقی می‌ماند. در AIC تازه، آستانه‌های `LastAggressor` را می‌توان در `AttackTargetPolicy.Provocation` نوشت.

### یورش‌ها

- `RaidTargetPolicy`: `Native` یورش قبلی را حفظ می‌کند؛ `NearestReachable` ساختمان نزدیک و قابل‌دسترسی را برمی‌گزیند؛ `Opportunistic` اولویت و خطر را می‌سنجد؛ `RandomNearby` ساختمانی قابل‌دسترسی را تصادفی انتخاب می‌کند و سپس ساختمان‌های نزدیک را می‌زند.
- `RaidGroupCount` (۱ تا ۴) نیروی موجود را تقسیم می‌کند؛ `RaidMinGroupSize` (۱ تا ۲۵۶) گروه کوچک را منتظر می‌گذارد یا با گروه دیگر یکی می‌کند.
- `RaidFocus` می‌تواند `Any`، `Food`، `Industry`، `HighValue` یا درصدهایی مانند `{ "Food": 60, "Industry": 20 }` باشد؛ سهم باقی‌مانده هر ساختمانی را می‌پذیرد. با `Opportunistic` و `RandomNearby` کار می‌کند. `RaidRiskTolerance` خطر و `RaidEnemyScope` دشمن هدف را تعیین می‌کند.

پنج فیلد پس از `RaidTargetPolicy` به سیاست یورش جدید نیاز دارند. `RaidUnitsBase`, `RaidUnitsRandom`، `RaidUnit1..8` و `RaidRetargetDelay` همچنان برقرارند.

### محاصره و مهندسان

- `SafeSiegePlacement` از محل ساخت اشغال‌شده دوری می‌کند (پیش‌فرض ماژول: روشن).
- `ActualSiegeResourcePayment` مواد و طلا می‌خواهد؛ هوش مصنوعی کمبود را با تجارت معمولی می‌خرد (خاموش).
- `CoordinatedSiegeHarassment` گروهی از ابزارهای محاصره را به مواضع تیراندازی دسترس‌پذیر می‌فرستد (خاموش). `SiegeHarassMinEngines` حداقل مطلوب را تعیین می‌کند (۰ تا ۲۰، پیش‌فرض ۳)؛ `HarassingSiegeEnginesMax` همچنان سقف کل است.
- `LargerSiegeForces` تا وقتی مهندسان حملهٔ زنده و آزاد و جای مناسب وجود دارد، ترکیب ابزارها را تکرار می‌کند (خاموش). `SiegeForceMax` سقف اختیاری است (۰ تا ۶۴)؛ با ۰ شمار مهندسان تصمیم می‌گیرد. `AttMaxEngineers` همچنان خدمه را محدود می‌کند.

با فعال‌شدن گزینه‌های پیشرفتهٔ محاصره، جای‌گذاری امن نیز خودکار فعال می‌شود. شمارش نقش مهندسان پیش‌فرض روشن است؛ Fixed Engineers چرخهٔ زندگی خدمه و ابزارها را مدیریت می‌کند.

مقدار صریح محاصره در AIC بر پیش‌فرض ماژول مقدم است.
