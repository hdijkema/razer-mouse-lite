param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../build/release/Release'),
    [string]$QtRoot = 'C:/Qt/6.11.1/msvc2022_64',
    [string]$Version = '',
    [string]$InnoCompiler = '',
    [switch]$TestInstaller
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$QtRoot = (Resolve-Path -LiteralPath $QtRoot).Path
if (!$Version) { $Version = (& (Join-Path $PSScriptRoot 'release-version.ps1')).Version }
if ($Version -notmatch '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$') { throw 'Use an X.Y.Z package version.' }
if ($TestInstaller -and !$InnoCompiler) { throw 'TestInstaller requires InnoCompiler.' }
$executable = Join-Path $BuildDirectory 'razer-mouse.exe'
if ((Get-Item -LiteralPath $executable).VersionInfo.ProductVersion -ne $Version) { throw 'Executable and package versions differ. Rebuild with MOUSE_LITE_VERSION.' }
$fixture = Join-Path $project ('build/packaging-' + [guid]::NewGuid().ToString('N'))
$application = Join-Path $fixture 'RazerMouseLite'
New-Item -ItemType Directory -Path $application -Force | Out-Null
Copy-Item -LiteralPath $executable -Destination $application
& "$QtRoot/bin/windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime "$application/razer-mouse.exe"
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or !$visualStudio) { throw 'Visual Studio C++ runtime location could not be determined.' }
$redist = Get-ChildItem "$visualStudio/VC/Redist/MSVC" -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$redist) { throw 'The MSVC runtime is missing.' }
Copy-Item -Path "$($redist.FullName)/x64/Microsoft.VC143.CRT/*.dll" -Destination $application
Copy-Item -LiteralPath "$project/LICENSE", "$project/README.md", "$project/THIRD_PARTY.md" -Destination $application
Copy-Item -LiteralPath "$project/licenses" -Destination $application -Recurse
Copy-Item -LiteralPath "$project/docs" -Destination $application -Recurse
@('[Paths]', 'Prefix=.', 'Plugins=.') | Set-Content "$application/qt.conf" -Encoding ASCII

# Test the distributable with development DLL and plugin paths removed.
$oldPath = $env:PATH
$oldPluginPath = $env:QT_PLUGIN_PATH
$oldPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = $application
    $env:QT_QPA_PLATFORM = 'windows'
    $image = Join-Path $fixture 'startup.png'
    $process = Start-Process -FilePath "$application/razer-mouse.exe" -ArgumentList @('--smoke-test', "`"$image`"", '--language', 'en') -WorkingDirectory $application -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(15000)) { $process.Kill(); throw 'Portable startup check timed out.' }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $image)) { throw 'Portable startup check failed.' }
} finally {
    $env:PATH = $oldPath
    $env:QT_PLUGIN_PATH = $oldPluginPath
    $env:QT_QPA_PLATFORM = $oldPlatform
}
$destination = Join-Path $project 'release'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$archiveName = "razer-mouse-lite-$Version-windows-x64.zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = Join-Path $fixture $archiveName
[System.IO.Compression.ZipFile]::CreateFromDirectory($application, $archive, [System.IO.Compression.CompressionLevel]::Optimal, $true)
Move-Item -LiteralPath $archive -Destination (Join-Path $destination $archiveName) -Force
# Keep the existing local portable launch path available; release packages use the fresh fixture.
New-Item -ItemType Directory -Path "$project/dist" -Force | Out-Null
Copy-Item -Path "$application/*" -Destination "$project/dist" -Recurse -Force
Write-Output "Portable ZIP: $destination/$archiveName"
if ($InnoCompiler) {
    $InnoCompiler = (Resolve-Path -LiteralPath $InnoCompiler).Path
    & $InnoCompiler "/DAppVersion=$Version" "/DSourceDirectory=$application" "/DOutputDirectory=$destination" "$project/packaging/windows/razer-mouse-lite.iss" | Tee-Object -FilePath "$fixture/installer-build.txt"
    if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
    Write-Output "Installer: $destination/razer-mouse-lite-$Version-windows-x64-setup.exe"
    if ($TestInstaller) { & "$project/tests/test-windows-installer.ps1" -SourceDirectory $application -InnoCompiler $InnoCompiler -Version $Version }
}
