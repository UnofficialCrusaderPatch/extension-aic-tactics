#include "aic_tactics/recruitment.hpp"
#include <cassert>
#include <cstdio>
#include <climits>

using namespace AicTactics;

int main()
{
    RecruitmentPolicy policy = {};
    RecruitmentPlan plan;
    RoleWeights base = {{35, 15, 40, 10}};
    RoleWeights invalid = {{INT_MAX, -1, 0, 0}};

    // Native mode never interprets or normalizes old sheet values.
    policy.mode = NativeRecruitment;
    policy.conditionCount = INT_MAX;
    assert(prepareRecruitment(policy, invalid, -100, ~0U, ~0U, plan) == UseNativeRecruitment);
    assert(selectRecruitmentRole(plan, 0) == InvalidRole);

    policy.mode = WeightedRoles;
    policy.conditionCount = 0;
    assert(prepareRecruitment(policy, base, DefaultStrength, 0, 15, plan) == DrawRecruitmentRole);
    assert(plan.totalWeight == 100 && plan.conditionIndex == -1);
    int counts[4] = {};
    for (int ticket = 0; ticket < 100; ++ticket) ++counts[selectRecruitmentRole(plan, ticket)];
    for (int role = 0; role < 4; ++role) assert(counts[role] == base.values[role]);
    assert(selectRecruitmentRole(plan, -1) == InvalidRole);
    assert(selectRecruitmentRole(plan, 100) == InvalidRole);

    // Every eligibility mask excludes its roles without reroll or fallback.
    for (unsigned int mask = 0; mask < 16; ++mask) {
        prepareRecruitment(policy, base, StrongStrength, 0, mask, plan);
        int expected = 0;
        for (int role = 0; role < 4; ++role) if (mask & (1U << role)) expected += base.values[role];
        assert(plan.totalWeight == expected);
        for (int ticket = 0; ticket < plan.totalWeight; ++ticket) {
            int role = selectRecruitmentRole(plan, ticket);
            assert(role >= 0 && role < 4 && (mask & (1U << role)));
        }
        if (!mask) assert(plan.decision == SkipRecruitment);
    }

    for (int only = 0; only < 4; ++only) {
        RoleWeights single = {{0, 0, 0, 0}};
        single.values[only] = 100;
        prepareRecruitment(policy, single, DefaultStrength, 0, 15, plan);
        for (int ticket = 0; ticket < 100; ++ticket) assert(selectRecruitmentRole(plan, ticket) == only);
        assert(prepareRecruitment(policy, single, DefaultStrength, 0, 15U & ~(1U << only), plan) == SkipRecruitment);
    }

    policy.conditionCount = 2;
    policy.conditions[0].strength = -1;
    policy.conditions[0].requiredFacts = HomeUnderThreat;
    policy.conditions[0].weights.values[0] = 100;
    policy.conditions[1].strength = WeakStrength;
    policy.conditions[1].requiredFacts = AttackActive;
    policy.conditions[1].forbiddenFacts = EquipmentSurplus;
    policy.conditions[1].weights.values[2] = 100;
    prepareRecruitment(policy, base, WeakStrength, HomeUnderThreat | AttackActive, 15, plan);
    assert(plan.conditionIndex == 0 && selectRecruitmentRole(plan, 99) == DefenseRole);
    prepareRecruitment(policy, base, WeakStrength, AttackActive, 15, plan);
    assert(plan.conditionIndex == 1 && selectRecruitmentRole(plan, 0) == AttackRole);
    prepareRecruitment(policy, base, WeakStrength, AttackActive | EquipmentSurplus, 15, plan);
    assert(plan.conditionIndex == -1);
    prepareRecruitment(policy, base, StrongStrength, AttackActive, 15, plan);
    assert(plan.conditionIndex == -1);

    // A bad later row rejects the configuration even if an earlier row matches.
    policy.conditions[1].weights.values[2] = 99;
    assert(prepareRecruitment(policy, base, WeakStrength, HomeUnderThreat, 15, plan) == InvalidRecruitment);
    assert(plan.totalWeight == 0);
    policy.conditions[1].weights.values[2] = 100;
    policy.conditions[1].forbiddenFacts |= AttackActive;
    assert(prepareRecruitment(policy, base, WeakStrength, 0, 15, plan) == InvalidRecruitment);
    policy.conditionCount = 9;
    assert(prepareRecruitment(policy, base, WeakStrength, 0, 15, plan) == InvalidRecruitment);
    policy.conditionCount = -1;
    assert(prepareRecruitment(policy, base, WeakStrength, 0, 15, plan) == InvalidRecruitment);
    policy.conditionCount = 0;
    assert(prepareRecruitment(policy, invalid, WeakStrength, 0, 15, plan) == InvalidRecruitment);
    assert(prepareRecruitment(policy, base, 3, 0, 15, plan) == InvalidRecruitment);
    assert(prepareRecruitment(policy, base, WeakStrength, 16, 15, plan) == InvalidRecruitment);
    assert(prepareRecruitment(policy, base, WeakStrength, 0, 16, plan) == InvalidRecruitment);

    prepareRecruitment(policy, base, DefaultStrength, 0, 15, plan);
    plan.eligibleWeights.values[0] = INT_MAX;
    assert(selectRecruitmentRole(plan, 0) == InvalidRole);
    std::puts("Recruitment component checks passed; no native gameplay acceptance is implied.");
    return 0;
}
