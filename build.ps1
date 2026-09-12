param (
    [ValidateSet('Release', 'Debug')][string]$BuildType = 'Release',
    [string]$UCP3Path = ''
)
$ErrorActionPreference = 'Stop'
$build = Join-Path $PSScriptRoot 'build/store-native'
$stage = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'build/store'))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'build'))
if (-not $stage.StartsWith("$buildRoot\", [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package staging must remain inside the module build directory'
}
foreach ($path in @($buildRoot, $stage)) {
    if ((Test-Path -LiteralPath $path) -and
        ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Package staging cannot follow directory links'
    }
}
& cmake -S $PSScriptRoot -B $build -A Win32 -DAIC_TACTICS_BUILD_SHC141_PROBE=ON
if ($LASTEXITCODE -ne 0) { throw 'AIC runtime configuration failed' }
& cmake --build $build --config $BuildType --target aicTactics
if ($LASTEXITCODE -ne 0) { throw 'AIC runtime compilation failed' }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
& python "$PSScriptRoot/tests/module_payload.py" --dll "$build/$BuildType/aicTactics.dll" --stage $stage
if ($LASTEXITCODE -ne 0) { throw 'AIC runtime packaging failed' }
