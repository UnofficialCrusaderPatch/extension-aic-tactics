// Test-process execution of original instructions, not a running game fixture.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "aic_tactics/shc141_recruitment.hpp"

using AicTactics::SHC141::RecruitmentServices;
using AicTactics::SHC141::RecruitmentAvailability;
using AicTactics::SHC141::RecruitFunction;

static const unsigned int ImageBase = 0x00400000;
static const unsigned int ImageSize = 0x02091000;
#pragma section(".aorigin", read, write)
__declspec(allocate(".aorigin")) unsigned char referenceSpace[ImageSize - 0x1000] = {0};
static unsigned char units[0x10000];
static unsigned char savedUnits[sizeof(units)];
static unsigned char* savedImage;
static int cases;
static LONG WINAPI nativeException(EXCEPTION_POINTERS* info)
{
    std::fprintf(stderr, "Native fixture exception %08lX at %08lX (ESP %08lX)\n",
        info->ExceptionRecord->ExceptionCode, info->ContextRecord->Eip, info->ContextRecord->Esp);
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
void runGroupCases();
#ifdef AIC_RUNTIME_TESTS
void runRuntimeCases();
void runDamageCases(unsigned int reservationOffset);
void runCombatCases();
void runIntegrityBenchmark();
#endif

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "Case %d: %s\n", cases, message);
        std::exit(1);
    }
}

template<class T> static T& at(unsigned int address)
{
    return *reinterpret_cast<T*>(address);
}

static RecruitmentServices fixture(int gold, bool peasant, int state,
    int peasantField, int missingResource, int horses, int playerID = 1)
{
    std::memset(units, 0, sizeof(units));
    at<int>(0x191DD80) = 0;
    for (int resource = 1; resource <= 24; ++resource)
        at<int>(0x115C2C8 + playerID * 0x39F4 + 4 * resource) = resource == missingResource ? 0 : 10;
    at<int>(0x115C304 + playerID * 0x39F4) = gold;
    at<int>(0xF98528) = horses ? 2 : 1;
    if (horses) {
        at<short>(0xF98930) = 1;
        at<short>(0xF98932) = 0x23;
        at<short>(0xF98936) = static_cast<short>(playerID);
        at<signed char>(0xF98930 + 0x1C7) = static_cast<signed char>(horses);
        at<signed char>(0xF98930 + 0x1D7) = 0;
    }
    *reinterpret_cast<int*>(units) = peasant ? 2 : 1;
    unsigned char* candidate = units + 0xD64;
    *reinterpret_cast<short*>(candidate - 0x232) = 1;
    *reinterpret_cast<short*>(candidate - 0x22A) = static_cast<short>(playerID);
    *reinterpret_cast<short*>(candidate - 0x234) = 2;
    *reinterpret_cast<short*>(candidate) = static_cast<short>(state);
    *reinterpret_cast<short*>(candidate + 0xB2) = static_cast<short>(peasantField);
    RecruitmentServices services;
    services.units = units;
    services.european = reinterpret_cast<RecruitFunction>(0x52E960);
    services.nonEuropean = reinterpret_cast<RecruitFunction>(0x52EC10);
    services.failureReason = reinterpret_cast<int*>(units + 0x60C);
    services.requiredResource = reinterpret_cast<int*>(units + 0x610);
    *services.failureReason = 77;
    *services.requiredResource = 88;
    for (int player = 1; player <= 8; ++player) {
        services.availableHorses[player - 1] = reinterpret_cast<short*>(0x115E04A + player * 0x39F4);
        *services.availableHorses[player - 1] = static_cast<short>(100 + player);
    }
    return services;
}

static void check(const RecruitmentServices& services, int player, int unit,
    int building, bool valid, bool eligible, int reason, int resource)
{
    ++cases;
    std::memcpy(savedImage, reinterpret_cast<void*>(ImageBase), ImageSize);
    std::memcpy(savedUnits, units, sizeof(units));
    RecruitmentAvailability result;
    bool accepted = AicTactics::SHC141::queryRecruitment(services, player, unit, building, result);
    require(accepted == valid, "wrong argument admission");
    require(result.eligible == eligible, "wrong eligibility");
    require(result.failureReason == reason, "wrong failure reason");
    require(result.requiredResource == resource, "wrong required resource");
    require(result.availableHorses == (unit == 28 && eligible ? 1 : 0), "horse availability snapshot differs");
    require(std::memcmp(savedUnits, units, sizeof(units)) == 0, "UnitsState changed");
    require(std::memcmp(savedImage, reinterpret_cast<void*>(ImageBase), ImageSize) == 0,
        "Original image/global state changed");
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, 0, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(nativeException);
    require(argc == 2 || argc == 3, "Use run_shc141_probe.py with the hash-verified reference executable");
    void* image = reinterpret_cast<void*>(ImageBase);
    const unsigned int reservationOffset = reinterpret_cast<unsigned int>(referenceSpace) - ImageBase;
    require(reservationOffset >= 0x1000 && reservationOffset <= 0x50000,
        "Reference reservation overlaps a required original function");
    FILE* input = std::fopen(argv[1], "rb");
    require(input != 0, "Cannot read reference image");
    require(std::fseek(input, reservationOffset, SEEK_SET) == 0, "Cannot seek past test host code");
    const unsigned int referenceBytes = ImageSize - reservationOffset;
    DWORD oldProtection;
    require(VirtualProtect(referenceSpace, referenceBytes, PAGE_EXECUTE_READWRITE, &oldProtection) != 0,
        "Cannot make private reference reservation writable");
    const size_t loadedBytes = std::fread(referenceSpace, 1, referenceBytes, input);
    if (loadedBytes != referenceBytes)
        std::fprintf(stderr, "Loaded %lu of %u bytes, reservation RVA %X, stream error %d\n",
            static_cast<unsigned long>(loadedBytes), referenceBytes, reservationOffset, std::ferror(input));
    require(loadedBytes == referenceBytes,
        "Wrong reference image length");
    require(std::fgetc(input) == EOF, "Unexpected reference image suffix");
    std::fclose(input);
    require(FlushInstructionCache(GetCurrentProcess(), image, ImageSize) != 0, "Instruction cache flush failed");
#ifdef AIC_RUNTIME_TESTS
    if (argc == 3) {
        require(std::strcmp(argv[2],"--benchmark-integrity")==0,"Unknown benchmark option");
        runIntegrityBenchmark();
        return 0;
    }
#endif
    savedImage = static_cast<unsigned char*>(std::malloc(ImageSize));
    require(savedImage != 0, "Cannot allocate state comparison");
    const int types[] = {22,23,24,25,26,27,5,29,30,37,70,71,72,73,74,75,76};
    for (unsigned int index = 0; index < sizeof(types) / sizeof(types[0]); ++index) {
        check(fixture(1000,true,1,0,0,0),1,types[index],1,true,true,0,0);
        check(fixture(0,true,1,0,0,0),1,types[index],1,true,false,1,0);
        check(fixture(1000,false,1,0,0,0),1,types[index],1,true,false,3,0);
        check(fixture(1000,true,0x6D,0,0,0),1,types[index],1,true,false,3,0);
    }
    check(fixture(1000,true,1,0,17,0),1,22,1,true,false,2,17);
    check(fixture(1000,true,1,0,23,0),1,23,1,true,false,2,23);
    check(fixture(1000,true,0,0,0,0),1,22,1,true,true,0,0);
    check(fixture(1000,true,0,1,0,0),1,22,1,true,false,3,0);
    check(fixture(1000,true,0,1,0,0),1,29,1,true,true,0,0);
    check(fixture(1000,true,1,0,0,0),1,28,1,true,false,4,0);
    check(fixture(1000,true,1,0,0,1),1,28,1,true,true,0,0);
    for (int player = 2; player <= 8; ++player) {
        check(fixture(1000,true,1,0,0,1,player),player,28,1,true,true,0,0);
        check(fixture(1000,true,1,0,0,0,player),player,70,1,true,true,0,0);
    }
    check(fixture(1000,true,1,0,0,0),0,22,1,false,false,0,0);
    check(fixture(1000,true,1,0,0,0),9,22,1,false,false,0,0);
    check(fixture(1000,true,1,0,0,0),1,0,1,false,false,0,0);
    check(fixture(1000,true,1,0,0,0),1,69,1,false,false,0,0);
    check(fixture(1000,true,1,0,0,0),1,22,0,false,false,0,0);
    RecruitmentServices absent = fixture(1000,true,1,0,0,1);
    absent.availableHorses[7] = 0;
    check(absent,1,28,1,false,false,0,0);
    absent = fixture(1000,true,1,0,0,0);
    absent.units = 0;
    check(absent,1,22,1,false,false,0,0);
    absent = fixture(1000,true,1,0,0,0);
    absent.european = 0;
    check(absent,1,22,1,false,false,0,0);
    absent = fixture(1000,true,1,0,0,0);
    absent.nonEuropean = 0;
    check(absent,1,70,1,false,false,0,0);
    std::printf("%d x86 original-instruction probe cases passed; no running-game acceptance claimed\n", cases);
#ifdef AIC_RUNTIME_TESTS
    runDamageCases(reservationOffset);
#endif
    runGroupCases();
#ifdef AIC_RUNTIME_TESTS
    runRuntimeCases();
    runCombatCases();
#endif
    std::free(savedImage);
    return 0;
}
