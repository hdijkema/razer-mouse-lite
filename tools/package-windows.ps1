param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../build/fltk-release/Release'),
    [string]$Version = '',
    [string]$InnoCompiler = '',
    [switch]$TestInstaller
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
if (!$Version) { $Version = (& (Join-Path $PSScriptRoot 'release-version.ps1')).Version }
if ($Version -notmatch '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$') { throw 'Use an X.Y.Z package version.' }
if ($TestInstaller -and !$InnoCompiler) { throw 'TestInstaller requires InnoCompiler.' }
$executable = Join-Path $BuildDirectory 'razer-mouse.exe'
if ((Get-Item -LiteralPath $executable).VersionInfo.ProductVersion -ne $Version) { throw 'Executable and package versions differ. Rebuild with MOUSE_LITE_VERSION.' }
$fixture = Join-Path $project ('build/packaging-' + [guid]::NewGuid().ToString('N'))
$application = Join-Path $fixture 'RazerMouseLite'
New-Item -ItemType Directory -Path $application -Force | Out-Null
Copy-Item -LiteralPath $executable -Destination $application
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'skins') -Destination $application -Recurse
if (Get-ChildItem -LiteralPath $BuildDirectory -Filter 'Qt*.dll') { throw 'This distribution must not contain Qt DLLs. Use a fresh FLTK build directory.' }
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or !$visualStudio) { throw 'Visual Studio C++ runtime location could not be determined.' }
$redist = Get-ChildItem "$visualStudio/VC/Redist/MSVC" -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$redist) { throw 'The MSVC runtime is missing.' }
Copy-Item -Path "$($redist.FullName)/x64/Microsoft.VC143.CRT/*.dll" -Destination $application
Copy-Item -LiteralPath "$project/LICENSE", "$project/README.md", "$project/THIRD_PARTY.md" -Destination $application
Copy-Item -LiteralPath "$project/licenses" -Destination $application -Recurse
Copy-Item -LiteralPath "$project/docs" -Destination $application -Recurse

# Test with development DLL directories removed from PATH.
$oldPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $image = Join-Path $fixture 'startup.png'
    $process = Start-Process -FilePath "$application/razer-mouse.exe" -ArgumentList @('--smoke-test', "`"$image`"", '--language', 'en') -WorkingDirectory $application -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(15000)) { $process.Kill(); throw 'Portable startup check timed out.' }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $image)) { throw 'Portable startup check failed.' }
} finally {
    $env:PATH = $oldPath
}
$destination = Join-Path $project 'release'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$archiveName = "razer-mouse-lite-$Version-windows-x64.zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = Join-Path $fixture $archiveName
[System.IO.Compression.ZipFile]::CreateFromDirectory($application, $archive, [System.IO.Compression.CompressionLevel]::Optimal, $true)
Move-Item -LiteralPath $archive -Destination (Join-Path $destination $archiveName) -Force
# Preserve the previous generated distribution, including any old toolkit DLLs.
$distribution = [IO.Path]::GetFullPath((Join-Path $project 'dist'))
$projectRoot = [IO.Path]::GetFullPath($project) + [IO.Path]::DirectorySeparatorChar
$backup = [IO.Path]::GetFullPath((Join-Path $fixture 'previous-dist'))
if (!$distribution.StartsWith($projectRoot, [StringComparison]::OrdinalIgnoreCase) -or !$backup.StartsWith($projectRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Distribution paths escaped the project.' }
if (Test-Path -LiteralPath $distribution) { Move-Item -LiteralPath $distribution -Destination $backup }
New-Item -ItemType Directory -Path $distribution -Force | Out-Null
Copy-Item -Path "$application/*" -Destination $distribution -Recurse -Force
Write-Output "Portable ZIP: $destination/$archiveName"
if ($InnoCompiler) {
    $InnoCompiler = (Resolve-Path -LiteralPath $InnoCompiler).Path
    & $InnoCompiler "/DAppVersion=$Version" "/DSourceDirectory=$application" "/DOutputDirectory=$destination" "$project/packaging/windows/razer-mouse-lite.iss" | Tee-Object -FilePath "$fixture/installer-build.txt"
    if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
    Write-Output "Installer: $destination/razer-mouse-lite-$Version-windows-x64-setup.exe"
    if ($TestInstaller) { & "$project/tests/test-windows-installer.ps1" -SourceDirectory $application -InnoCompiler $InnoCompiler -Version $Version }
}
