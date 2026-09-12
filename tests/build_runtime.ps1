param([Parameter(Mandatory=$true)][string]$CompilerRoot)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$env:PATH = "$CompilerRoot/VC/bin;$CompilerRoot/Common7/IDE;" + $env:PATH
$env:INCLUDE = "$CompilerRoot/VC/include;$CompilerRoot/VC/PlatformSDK/include"
$env:LIB = "$CompilerRoot/VC/lib;$CompilerRoot/VC/PlatformSDK/lib"
Push-Location $taskRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & "$CompilerRoot/VC/bin/cl.exe" /nologo /W4 /WX /MT /EHsc /O2 /LD /Iinclude /Fobuild/ /Febuild/aicTactics.dll src/runtime.cpp src/lua_module.cpp src/recruitment.cpp src/random.cpp src/composition.cpp src/shc141_recruitment.cpp src/shc141_groups.cpp /link /MANIFEST:NO kernel32.lib
    if ($LASTEXITCODE -ne 0) { throw "Native runtime build failed: $LASTEXITCODE" }
} finally { Pop-Location }
