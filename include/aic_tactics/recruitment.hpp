#ifndef AIC_TACTICS_RECRUITMENT_HPP
#define AIC_TACTICS_RECRUITMENT_HPP

namespace AicTactics {

enum RecruitmentMode { NativeRecruitment, WeightedRoles };
enum StrengthClass { DefaultStrength, WeakStrength, StrongStrength };
enum RecruitmentRole { InvalidRole = -2, NoRole = -1, DefenseRole, RaidRole, AttackRole, SortieRole };
enum RecruitmentDecision { InvalidRecruitment, UseNativeRecruitment, SkipRecruitment, DrawRecruitmentRole };
enum RecruitmentFact {
    HomeUnderThreat = 1,
    AttackActive = 2,
    DefenseIncomplete = 4,
    EquipmentSurplus = 8
};

enum { RecruitmentRoleCount = 4, MaximumRecruitmentConditions = 8, AllRecruitmentFacts = 15 };

struct RoleWeights {
    int values[RecruitmentRoleCount];
};

struct RecruitmentCondition {
    // -1 means any strength class. Facts are supplied by the native adapter.
    int strength;
    unsigned int requiredFacts;
    unsigned int forbiddenFacts;
    RoleWeights weights;
};

struct RecruitmentPolicy {
    RecruitmentMode mode;
    int conditionCount;
    RecruitmentCondition conditions[MaximumRecruitmentConditions];
};

struct RecruitmentPlan {
    RecruitmentDecision decision;
    int conditionIndex; // -1 selects the base row for the current strength class.
    RoleWeights eligibleWeights;
    int totalWeight;
};

// Call only at an eligible native recruitment opportunity. Base weights reuse
// the native defense/raid/attack fields, with the authored sortie weight added.
// Eligibility must already include native quotas, restrictions and affordability;
// this function neither probes acquisition nor advances RNG or world state.
RecruitmentDecision prepareRecruitment(const RecruitmentPolicy& policy,
    const RoleWeights& baseWeights, int strength, unsigned int facts,
    unsigned int eligibleRoles, RecruitmentPlan& plan);

// ticket must be uniformly selected from [0, plan.totalWeight), by the owning
// synchronized RNG adapter. Mapping a raw RNG sample with modulo is not implied.
RecruitmentRole selectRecruitmentRole(const RecruitmentPlan& plan, int ticket);

} // namespace AicTactics

#endif
