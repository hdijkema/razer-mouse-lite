param(
    [string]$CMake = 'cmake',
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build/fltk-release'),
    [string]$Version = '',
    [string]$InnoCompiler = '',
    [switch]$TestInstaller
)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
if (!$Version) { $Version = (& "$PSScriptRoot/tools/release-version.ps1").Version }
function Run([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
Run $CMake @('-S', $PSScriptRoot, '-B', $BuildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DMOUSE_LITE_VERSION=$Version")
Run $CMake @('--build', $BuildDirectory, '--config', 'Release', '--target', 'razer-mouse', '--parallel', '4')
$cmakePath = (Get-Command $CMake -ErrorAction Stop).Source
Run (Join-Path (Split-Path $cmakePath) 'ctest.exe') @('--test-dir', $BuildDirectory, '-C', 'Release', '--output-on-failure')
& "$PSScriptRoot/tools/package-windows.ps1" -BuildDirectory "$BuildDirectory/Release" -Version $Version -InnoCompiler $InnoCompiler -TestInstaller:$TestInstaller
