#include "aic_tactics/random.hpp"

namespace AicTactics {

BoundedDrawStatus drawBounded(int bound, TakeNativeRandomSample takeSample,
    void* context, BoundedDraw& result)
{
    result.ticket = -1;
    result.samplesConsumed = 0;
    if (bound < 1 || bound > 100)
        return InvalidRandomBound;
    if (bound == 1) {
        result.ticket = 0;
        return DrawSucceeded;
    }
    if (!takeSample)
        return InvalidRandomSource;

    const int limit = (32768 / bound) * bound;
    for (int attempt = 0; attempt < 8; ++attempt) {
        const int sample = takeSample(context);
        ++result.samplesConsumed;
        if (sample < 0 || sample >= 32768)
            return InvalidRandomSample;
        if (sample < limit) {
            result.ticket = sample % bound;
            return DrawSucceeded;
        }
    }
    return RandomSourceExhausted;
}

} // namespace AicTactics
