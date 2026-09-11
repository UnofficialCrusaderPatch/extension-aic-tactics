param(
    [Parameter(Mandatory=$true)][string]$CompilerRoot,
    [Parameter(Mandatory=$true)][string]$ReferenceExe,
    [Parameter(Mandatory=$true)][string]$PythonExe
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$env:PATH = "$CompilerRoot/VC/bin;$CompilerRoot/Common7/IDE;" + $env:PATH
$env:INCLUDE = "$CompilerRoot/VC/include;$CompilerRoot/VC/PlatformSDK/include"
$env:LIB = "$CompilerRoot/VC/lib;$CompilerRoot/VC/PlatformSDK/lib"
Push-Location $taskRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & "$CompilerRoot/VC/bin/cl.exe" /nologo /W4 /WX /MT /EHsc /O2 /D_CRT_SECURE_NO_WARNINGS /Iinclude /Fobuild/ /Febuild/shc141-probe-tests.exe src/shc141_recruitment.cpp src/shc141_damage.cpp src/shc141_groups.cpp tests/shc141_probe_tests.cpp tests/shc141_damage_cases.cpp tests/shc141_group_cases.cpp /link /MANIFEST:NO /BASE:0x400000 /FIXED /MERGE:.text=.aorigin '/SECTION:.aorigin,ERW' /IGNORE:4254 /MERGE:.rdata=.zrdata /MERGE:.data=.zdata kernel32.lib
    if ($LASTEXITCODE -ne 0) { throw "MSVC2005 probe build failed: $LASTEXITCODE" }
    & $PythonExe tests/run_shc141_probe.py --reference $ReferenceExe --test-exe build/shc141-probe-tests.exe --output build/probe-evidence --compiler "$CompilerRoot/VC/bin/cl.exe"
    if ($LASTEXITCODE -ne 0) { throw "Native instruction probe checks failed: $LASTEXITCODE" }
} finally {
    Pop-Location
}
