#ifndef AIC_TACTICS_RANDOM_HPP
#define AIC_TACTICS_RANDOM_HPP

namespace AicTactics {

enum BoundedDrawStatus {
    DrawSucceeded, InvalidRandomBound, InvalidRandomSource,
    InvalidRandomSample, RandomSourceExhausted
};

struct BoundedDraw {
    int ticket;
    int samplesConsumed;
};

typedef int (*TakeNativeRandomSample)(void* context);

// takeSample must return the current synchronized 15-bit sample and advance its
// existing owner once. Bounds 1..100 cover recruitment weights and player rosters.
// Native/disabled callers must bypass this function. A bound of 1 consumes none.
BoundedDrawStatus drawBounded(int bound, TakeNativeRandomSample takeSample,
    void* context, BoundedDraw& result);

} // namespace AicTactics

#endif
