#include <windows.h>
#include "aic_tactics/diagnostics.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace AicTactics {
namespace SHC141 {

DiagnosticLog diagnosticsLog = 0;

void diagnostic(const char* format, ...)
{
    if (!diagnosticsLog) return;
    char message[256] = "[aic-tactics] ";
    const size_t prefix = std::strlen(message);
    va_list arguments;
    va_start(arguments, format);
    _vsnprintf_s(message + prefix, sizeof(message) - prefix, _TRUNCATE, format, arguments);
    va_end(arguments);
    diagnosticsLog(message);
}

} // namespace SHC141
} // namespace AicTactics
