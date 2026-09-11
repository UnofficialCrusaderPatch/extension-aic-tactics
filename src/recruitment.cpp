#include "aic_tactics/recruitment.hpp"

namespace AicTactics {
namespace {

bool validWeights(const RoleWeights& weights)
{
    int total = 0;
    for (int role = 0; role < RecruitmentRoleCount; ++role) {
        if (weights.values[role] < 0 || weights.values[role] > 100) return false;
        total += weights.values[role];
    }
    return total == 100;
}

} // namespace

RecruitmentDecision prepareRecruitment(const RecruitmentPolicy& policy,
    const RoleWeights& baseWeights, int strength, unsigned int facts,
    unsigned int eligibleRoles, RecruitmentPlan& plan)
{
    plan.decision = InvalidRecruitment;
    plan.conditionIndex = -1;
    plan.totalWeight = 0;
    for (int role = 0; role < RecruitmentRoleCount; ++role) plan.eligibleWeights.values[role] = 0;

    if (policy.mode == NativeRecruitment) {
        plan.decision = UseNativeRecruitment;
        return plan.decision;
    }
    if (policy.mode != WeightedRoles || policy.conditionCount < 0
        || policy.conditionCount > MaximumRecruitmentConditions
        || strength < DefaultStrength || strength > StrongStrength
        || (facts & ~static_cast<unsigned int>(AllRecruitmentFacts)) != 0
        || (eligibleRoles & ~((1U << RecruitmentRoleCount) - 1)) != 0
        || !validWeights(baseWeights)) return plan.decision;

    const RoleWeights* selected = &baseWeights;
    int conditionIndex = -1;
    for (int index = 0; index < policy.conditionCount; ++index) {
        const RecruitmentCondition& condition = policy.conditions[index];
        if (condition.strength < -1 || condition.strength > StrongStrength
            || ((condition.requiredFacts | condition.forbiddenFacts)
                & ~static_cast<unsigned int>(AllRecruitmentFacts)) != 0
            || (condition.requiredFacts & condition.forbiddenFacts) != 0
            || !validWeights(condition.weights)) return plan.decision;

        if (conditionIndex == -1 && (condition.strength == -1 || condition.strength == strength)
            && (facts & condition.requiredFacts) == condition.requiredFacts
            && (facts & condition.forbiddenFacts) == 0) {
            selected = &condition.weights;
            conditionIndex = index;
        }
    }

    plan.conditionIndex = conditionIndex;
    for (int role = 0; role < RecruitmentRoleCount; ++role) {
        if ((eligibleRoles & (1U << role)) != 0) {
            plan.eligibleWeights.values[role] = selected->values[role];
            plan.totalWeight += selected->values[role];
        }
    }
    plan.decision = plan.totalWeight == 0 ? SkipRecruitment : DrawRecruitmentRole;
    return plan.decision;
}

RecruitmentRole selectRecruitmentRole(const RecruitmentPlan& plan, int ticket)
{
    if (plan.decision != DrawRecruitmentRole || plan.totalWeight < 1 || plan.totalWeight > 100
        || ticket < 0 || ticket >= plan.totalWeight) return InvalidRole;

    int total = 0;
    for (int role = 0; role < RecruitmentRoleCount; ++role) {
        if (plan.eligibleWeights.values[role] < 0 || plan.eligibleWeights.values[role] > 100)
            return InvalidRole;
        total += plan.eligibleWeights.values[role];
    }
    if (total != plan.totalWeight) return InvalidRole;

    for (int role = 0; role < RecruitmentRoleCount; ++role) {
        if (ticket < plan.eligibleWeights.values[role]) return static_cast<RecruitmentRole>(role);
        ticket -= plan.eligibleWeights.values[role];
    }
    return InvalidRole;
}

} // namespace AicTactics
