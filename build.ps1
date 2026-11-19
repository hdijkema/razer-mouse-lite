param(
    [string]$Qt = $(if ($env:QT_ROOT_DIR) { $env:QT_ROOT_DIR } else { 'C:/Qt/6.11.1/msvc2022_64' }),
    [string]$CMake = 'C:/Qt/Tools/CMake_64/bin/cmake.exe',
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build/release'),
    [string]$Version = '',
    [string]$InnoCompiler = '',
    [switch]$TestInstaller
)
$ErrorActionPreference = 'Stop'
if (![System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path (Get-Location).ProviderPath $BuildDirectory
}
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
if (!$Version) { $Version = (& "$PSScriptRoot/tools/release-version.ps1").Version }
function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
Run $CMake @('-S', $PSScriptRoot, '-B', $BuildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_PREFIX_PATH=$Qt", "-DMOUSE_LITE_VERSION=$Version")
Run $CMake @('--build', $BuildDirectory, '--config', 'Release', '--parallel')
$env:PATH = "$Qt/bin;" + $env:PATH
$cmakePath = (Get-Command $CMake -ErrorAction Stop).Source
Run (Join-Path (Split-Path $cmakePath) 'ctest.exe') @('--test-dir', $BuildDirectory, '-C', 'Release', '--output-on-failure')
& "$PSScriptRoot/tools/package-windows.ps1" -BuildDirectory "$BuildDirectory/Release" -QtRoot $Qt -Version $Version -InnoCompiler $InnoCompiler -TestInstaller:$TestInstaller
