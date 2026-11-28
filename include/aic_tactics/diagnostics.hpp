#ifndef AIC_TACTICS_DIAGNOSTICS_HPP
#define AIC_TACTICS_DIAGNOSTICS_HPP

namespace AicTactics {
namespace SHC141 {

// Module option `diagnostics`: lord hunt, ally and patrol decisions are
// written through the Lua logging callback while it is set.
typedef void (*DiagnosticLog)(const char*);
extern DiagnosticLog diagnosticsLog;
void diagnostic(const char* format, ...);

} // namespace SHC141
} // namespace AicTactics
#endif
