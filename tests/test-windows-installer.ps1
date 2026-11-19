param(
    [Parameter(Mandatory=$true)][string]$SourceDirectory,
    [Parameter(Mandatory=$true)][string]$InnoCompiler,
    [Parameter(Mandatory=$true)][string]$Version
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$identity = [guid]::NewGuid().ToString('N')
$fixture = [IO.Path]::GetFullPath((Join-Path $project "build/installer-test-$identity"))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $project 'build')) + [IO.Path]::DirectorySeparatorChar
if (!$fixture.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture escaped the build directory.' }
$installed = Join-Path $fixture 'Installed Application'
$registrySubkey = "Software\RazerMouseLiteInstallerTests\$identity"
$registryPath = "HKCU:\$registrySubkey"
New-Item -ItemType Directory -Path $fixture | Out-Null
& $InnoCompiler '/Q' "/DAppVersion=$Version" "/DSourceDirectory=$SourceDirectory" "/DOutputDirectory=$fixture" "/DInstallerAppId=RazerMouseLite.Test.$identity" "/DInstallerName=Razer Mouse Lite Test $identity" "/DStartupKey=$registrySubkey" "$project/packaging/windows/razer-mouse-lite.iss"
if ($LASTEXITCODE -ne 0) { throw 'Private test installer compilation failed.' }
$setup = Join-Path $fixture "razer-mouse-lite-$Version-windows-x64-setup.exe"
function Invoke-TestProcess([string]$Executable, [string[]]$Arguments) {
    $process = Start-Process -FilePath $Executable -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(60000)) { $process.Kill(); throw "Timed out: $Executable" }
    if ($process.ExitCode -ne 0) { throw "Exit code $($process.ExitCode): $Executable" }
}
$settings = Join-Path $fixture 'profiles.json'
Set-Content -LiteralPath $settings -Value "{`"sentinel`":`"$identity`"}" -Encoding ASCII
$settingsHash = (Get-FileHash -LiteralPath $settings).Hash
$uninstaller = Join-Path $installed 'unins000.exe'
try {
    Invoke-TestProcess $setup @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOICONS',"/DIR=`"$installed`"","/LOG=`"$fixture/install.log`"")
    $exe = Join-Path $installed 'razer-mouse.exe'
    if ((Get-FileHash -LiteralPath $exe).Hash -ne (Get-FileHash -LiteralPath "$SourceDirectory/razer-mouse.exe").Hash) { throw 'Installed application differs.' }
    if ((Get-Item -LiteralPath $exe).VersionInfo.ProductVersion -ne $Version) { throw 'Wrong installed version.' }
    Invoke-TestProcess $exe @('--smoke-test',"`"$fixture/installed.png`"",'--language','en')
    if (!(Test-Path -LiteralPath "$fixture/installed.png")) { throw 'Installed startup failed.' }
    $marker = Join-Path $installed 'user-marker.txt'
    Set-Content -LiteralPath $marker -Value $identity -Encoding ASCII
    $markerHash = (Get-FileHash -LiteralPath $marker).Hash
    # The resolved target is inside the checked fixture, never the user's portable app.
    Remove-Item -LiteralPath (Join-Path $installed 'Qt6Svg.dll')
    Invoke-TestProcess $setup @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOICONS',"/DIR=`"$installed`"","/LOG=`"$fixture/reinstall.log`"")
    if (!(Test-Path -LiteralPath (Join-Path $installed 'Qt6Svg.dll'))) { throw 'Reinstall did not restore an installed file.' }
    New-Item -Path $registryPath -Force | Out-Null
    New-ItemProperty -LiteralPath $registryPath -Name RazerMouseLite -Value "`"$exe`" --tray" -Force | Out-Null
    Invoke-TestProcess $uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$fixture/uninstall.log`"")
    if (Get-ItemProperty -LiteralPath $registryPath -Name RazerMouseLite -ErrorAction SilentlyContinue) { throw 'Own startup entry was left behind.' }
    if (Test-Path -LiteralPath $exe) { throw 'Uninstall left the executable.' }
    if ((Get-FileHash -LiteralPath $marker).Hash -ne $markerHash) { throw 'Uninstall changed an untracked file.' }
    if ((Get-FileHash -LiteralPath $settings).Hash -ne $settingsHash) { throw 'Installer changed separate profiles.' }
    Invoke-TestProcess $setup @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOICONS',"/DIR=`"$installed`"")
    $foreign = '"C:\OtherPortableCopy\razer-mouse.exe" --tray'
    New-ItemProperty -LiteralPath $registryPath -Name RazerMouseLite -Value $foreign -Force | Out-Null
    Invoke-TestProcess $uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
    if ((Get-ItemProperty -LiteralPath $registryPath).RazerMouseLite -ne $foreign) { throw 'Uninstall removed another copy startup entry.' }
} finally {
    if (Test-Path -LiteralPath $uninstaller) { Invoke-TestProcess $uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') }
    if (Test-Path -LiteralPath $registryPath) { Remove-Item -LiteralPath $registryPath }
}
Write-Output "PASS: install, reinstall/repair, startup, uninstall, settings retention and startup cleanup: $fixture"
