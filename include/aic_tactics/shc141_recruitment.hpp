#ifndef AIC_TACTICS_SHC141_RECRUITMENT_HPP
#define AIC_TACTICS_SHC141_RECRUITMENT_HPP

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error SHC 1.41 recruitment requires the Microsoft x86 thiscall ABI
#endif

namespace AicTactics {
namespace SHC141 {

typedef int (__thiscall *RecruitFunction)(void*, int, int, int, int);

struct RecruitmentServices {
    void* units;
    RecruitFunction european;
    RecruitFunction nonEuropean;
    int* failureReason;
    int* requiredResource;
    short* availableHorses[8];
};

struct RecruitmentAvailability {
    bool eligible;
    int failureReason;
    int requiredResource;
};

// Original, signature-verified services only. Returns false for invalid inputs.
// Does not establish AIC role quotas, building validity or tribe admission.
// Must run on the existing simulation thread outside a recruitment callback.
bool queryRecruitment(const RecruitmentServices& services, int player,
    int unitType, int building, RecruitmentAvailability& result);

} // namespace SHC141
} // namespace AicTactics

#endif
