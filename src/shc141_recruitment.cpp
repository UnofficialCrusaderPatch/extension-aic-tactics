#include "aic_tactics/shc141_recruitment.hpp"

namespace AicTactics {
namespace SHC141 {

bool queryRecruitment(const RecruitmentServices& services, int player,
    int unitType, int building, RecruitmentAvailability& result)
{
    result.eligible = false;
    result.failureReason = 0;
    result.requiredResource = 0;
    const bool european = unitType >= 22 && unitType <= 28;
    const bool nonEuropean = unitType == 5 || unitType == 29 || unitType == 30
        || unitType == 37 || (unitType >= 70 && unitType <= 76);
    if (player < 1 || player > 8 || building <= 0 || (!european && !nonEuropean)
        || !services.units || !services.failureReason || !services.requiredResource)
        return false;
    RecruitFunction recruit = european ? services.european : services.nonEuropean;
    if (!recruit)
        return false;
    if (unitType == 28) {
        for (int index = 0; index < 8; ++index) {
            if (!services.availableHorses[index])
                return false;
        }
    }

    const int savedReason = *services.failureReason;
    const int savedResource = *services.requiredResource;
    short horses[8];
    if (unitType == 28) {
        for (int index = 0; index < 8; ++index)
            horses[index] = *services.availableHorses[index];
    }

    const int acquired = recruit(services.units, unitType, building, player, 1);
    result.eligible = acquired != 0;
    result.failureReason = *services.failureReason;
    if (result.failureReason == 2)
        result.requiredResource = *services.requiredResource;

    *services.failureReason = savedReason;
    *services.requiredResource = savedResource;
    if (unitType == 28) {
        for (int index = 0; index < 8; ++index)
            *services.availableHorses[index] = horses[index];
    }
    return true;
}

} // namespace SHC141
} // namespace AicTactics
