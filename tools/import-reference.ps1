# Development-only import. No scripts or reference checkout are needed at runtime.
param([string]$Reference = "$PSScriptRoot/../../openrazer-win")
$ErrorActionPreference = 'Stop'
$recipes = Get-Content "$Reference/openrazer_win/devices/data/recipes.json" -Raw | ConvertFrom-Json
$devices = (Get-Content "$Reference/openrazer_win/devices/data/devices.json" -Raw | ConvertFrom-Json).devices
function Get-Steps([string]$key, [string]$pidHex, [int]$led) {
    $entry = $recipes.drivers.mouse.$key
    if (!$entry) { return @() }
    if ($entry.alias) { return Get-Steps $entry.alias $pidHex $entry.bind.led_id }
    $index = $entry.default
    if ($null -ne $entry.pids.$pidHex) { $index = $entry.pids.$pidHex }
    $steps = @($recipes.pool[$index])
    if (!($steps | Where-Object { $_.op -eq 'build' })) { return @() }
    # Bind each lighting zone once, rather than keep an interpreter for variables.
    $json = ConvertTo-Json -InputObject $steps -Depth 40 -Compress
    $json = $json -replace '\{"k":"var","name":"led_id"\}', ('{"k":"const","value":' + $led + '}')
    $parsed = ConvertFrom-Json -InputObject $json
    foreach ($step in $parsed) { Write-Output $step }
}
$models = @()
foreach ($device in $devices | Where-Object { $_.type -eq 'mouse' -and $_.transport -eq 'hid' }) {
    $pidHex = '{0:x4}' -f $device.pid
    $dpiRecipe = $recipes.drivers.mouse.'write:dpi'.pids.$pidHex
    # These two legacy variants need additional device state. Do not guess.
    if ($null -eq $dpiRecipe -or $dpiRecipe -in @(436, 438)) { continue }
    $tx = switch ($dpiRecipe) { 437 {255} 439 {255} 440 {255} 441 {63} default {31} }
    $transport = $recipes.transport.mouse.default
    if ($recipes.transport.mouse.pids.$pidHex) { $transport = $recipes.transport.mouse.pids.$pidHex }
    $zones = @()
    foreach ($zone in @('backlight','logo','scroll','left','right')) {
        $led = @{backlight=5; logo=4; scroll=1; left=17; right=16}[$zone]
        $staticMethod = "set_${zone}_static"
        if ($zone -eq 'backlight' -and $device.methods -contains 'set_static_effect') { $staticMethod = 'set_static_effect' }
        if ($device.methods -notcontains $staticMethod) { continue }
        $prefix = "${zone}_"
        if ($staticMethod -eq 'set_static_effect') { $prefix = '' }
        $color = @(Get-Steps "write:${prefix}matrix_effect_static" $pidHex $led)
        $off = @(Get-Steps "write:${prefix}matrix_effect_none" $pidHex $led)
        $brightness = @()
        foreach ($key in $(if ($prefix -eq '') { @('matrix_brightness','backlight_led_brightness','led_brightness') } else { @("${zone}_led_brightness") })) {
            $brightness = @(Get-Steps "write:$key" $pidHex $led)
            if ($brightness.Count) { break }
        }
        if ($color.Count) { $zones += @{name=$zone; color=$color; off=$off; brightness=$brightness} }
    }
    $pollSteps = @(Get-Steps 'write:poll_rate' $pidHex 0)
    $pollTx = -1
    if (($pollSteps | Where-Object op -eq 'build' | Select-Object -First 1).fn -eq 'razer_chroma_misc_set_polling_rate') {
        $pollTx = ($pollSteps | Where-Object op -eq 'txid' | Select-Object -First 1).value.value
    }
    $models += @{pid=$device.pid; name=$device.name; maxDpi=$device.dpi_max; dpiByte=($dpiRecipe -eq 437); dpiTx=$tx;
        interface=$transport.index; waitUs=$transport.wait_us; zones=$zones; pollTx=$pollTx}
}
New-Item -ItemType Directory -Force "$PSScriptRoot/../data" | Out-Null
ConvertTo-Json -InputObject $models -Depth 45 -Compress | Set-Content "$PSScriptRoot/../data/mice.json" -Encoding UTF8
Write-Output "Imported $($models.Count) USB mouse models."
