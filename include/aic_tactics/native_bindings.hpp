#ifndef AIC_TACTICS_NATIVE_BINDINGS_HPP
#define AIC_TACTICS_NATIVE_BINDINGS_HPP

namespace AicTactics {
namespace SHC141 {

// Populated once from UCP/Loader-owned discovery before callbacks are installed.
// No reference addresses or fallback bindings belong in this runtime structure.
struct NativeBindings {
    unsigned int gameTick;
    unsigned int rngState;
    unsigned int rngValue;
    unsigned int rngNext;
    unsigned int initialDefenseTicks;
    unsigned int aicRecords;
};
extern NativeBindings nativeBindings;
int nativeRandom(void*);

} // namespace SHC141
} // namespace AicTactics
#endif
