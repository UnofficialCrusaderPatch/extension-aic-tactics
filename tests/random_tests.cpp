#include <cassert>
#include "aic_tactics/random.hpp"

using namespace AicTactics;

struct Samples {
    int first;
    int rest;
    int calls;
};

static int take(void* context)
{
    Samples& samples = *static_cast<Samples*>(context);
    ++samples.calls;
    return samples.calls == 1 ? samples.first : samples.rest;
}

static void equalAcceptedCounts()
{
    for (int bound = 2; bound <= 100; ++bound) {
        int counts[100] = {0};
        int rejected = 0;
        for (int sample = 0; sample < 32768; ++sample) {
            Samples samples = {sample, 0, 0};
            BoundedDraw result;
            assert(drawBounded(bound, take, &samples, result) == DrawSucceeded);
            assert(result.ticket >= 0 && result.ticket < bound);
            assert(result.samplesConsumed == samples.calls);
            if (samples.calls == 1)
                ++counts[result.ticket];
            else {
                assert(samples.calls == 2 && result.ticket == 0);
                ++rejected;
            }
        }
        for (int ticket = 1; ticket < bound; ++ticket)
            assert(counts[ticket] == counts[0]);
        assert(counts[0] * bound + rejected == 32768);
    }
}

static void originalGeneratorBound()
{
    // The reference CRT's 32-bit recurrence and 15-bit output were disassembled
    // at 0x5816FB. Bit 31 never affects the masked output, including future steps.
    // 32670 is the minimum acceptance limit across every bound 1..100.
    unsigned int maximum = 0;
    for (unsigned int initial = 32670u << 16; initial < 0x80000000u; ++initial) {
        unsigned int state = initial, length = 0;
        while (((state >> 16) & 32767u) >= 32670u && length < 4) {
            state = state * 214013u + 2531011u;
            ++length;
        }
        assert(length <= 3);
        if (length > maximum) maximum = length;
    }
    assert(maximum == 3);
}

int main()
{
    BoundedDraw result;
    Samples samples = {32767, 32767, 0};
    assert(drawBounded(0, take, &samples, result) == InvalidRandomBound);
    assert(drawBounded(101, take, &samples, result) == InvalidRandomBound);
    assert(samples.calls == 0 && result.ticket == -1);
    assert(drawBounded(1, 0, 0, result) == DrawSucceeded);
    assert(result.ticket == 0 && result.samplesConsumed == 0);
    assert(drawBounded(2, 0, 0, result) == InvalidRandomSource);
    assert(result.ticket == -1 && result.samplesConsumed == 0);
    assert(drawBounded(100, take, &samples, result) == RandomSourceExhausted);
    assert(samples.calls == 8 && result.samplesConsumed == 8 && result.ticket == -1);
    samples.first = -1; samples.calls = 0;
    assert(drawBounded(100, take, &samples, result) == InvalidRandomSample);
    assert(samples.calls == 1 && result.ticket == -1);
    samples.first = 32768; samples.calls = 0;
    assert(drawBounded(100, take, &samples, result) == InvalidRandomSample);
    assert(samples.calls == 1 && result.ticket == -1);
    equalAcceptedCounts();
    originalGeneratorBound();
    return 0;
}
