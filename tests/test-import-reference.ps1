param([Parameter(Mandatory)][string]$Reference)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
. "$project/tools/recipe-signature.ps1"
$Reference = (Resolve-Path -LiteralPath $Reference).Path
$fixture = Join-Path $project ('build/import-test-' + [guid]::NewGuid().ToString('N'))
$data = Join-Path $fixture 'reference/openrazer_win/devices/data'
New-Item -ItemType Directory -Force $data | Out-Null
$header = Join-Path $fixture 'razercommon.h'
[IO.File]::WriteAllText($header, "#define DRIVER_VERSION 1`nstruct report { int value; };`n")
$headerHash = Get-SourceFileHash $header
[IO.File]::WriteAllText($header, "#define DRIVER_VERSION 2`r`nstruct report { int value; };`r`n")
if ((Get-SourceFileHash $header) -ne $headerHash) { throw 'Line endings or a version bump changed the protocol fingerprint.' }
[IO.File]::WriteAllText($header, "#define DRIVER_VERSION 2`nstruct report { int different; };`n")
if ((Get-SourceFileHash $header) -eq $headerHash) { throw 'A protocol layout change was not detected.' }
$devicesFile = Join-Path $Reference 'openrazer_win/devices/data/devices.json'
$recipesFile = Join-Path $Reference 'openrazer_win/devices/data/recipes.json'
Copy-Item -LiteralPath $devicesFile -Destination $data
$baseline = Join-Path $fixture 'baseline.json'
$shifted = Join-Path $fixture 'shifted.json'
& "$project/tools/import-reference.ps1" -Reference $Reference -OutputFile $baseline
$recipes = Get-Content -LiteralPath $recipesFile -Raw | ConvertFrom-Json
$count = $recipes.pool.Count
$pool = @($recipes.pool)
[array]::Reverse($pool)
$recipes.pool = $pool
foreach ($driver in $recipes.drivers.PSObject.Properties) {
    foreach ($entry in $driver.Value.PSObject.Properties.Value) {
        if ($null -ne $entry.default) { $entry.default = $count - 1 - $entry.default }
        foreach ($product in $entry.pids.PSObject.Properties) { $product.Value = $count - 1 - $product.Value }
    }
}
$modifiedFile = Join-Path $data 'recipes.json'
ConvertTo-Json $recipes -Depth 80 -Compress | Set-Content -LiteralPath $modifiedFile -Encoding utf8
& "$project/tools/import-reference.ps1" -Reference (Join-Path $fixture 'reference') -OutputFile $shifted
$expected = Get-Content -LiteralPath $baseline -Raw
$actual = Get-Content -LiteralPath $shifted -Raw
if ($expected -ne $actual) { throw 'Moving recipe pool indices changed the imported definitions.' }
$models = $expected | ConvertFrom-Json
$cobra = @($models | Where-Object pid -eq 163)
if ($cobra.Count -ne 1 -or $cobra[0].maxDpi -ne 8500 -or $cobra[0].dpiTx -ne 255 -or $cobra[0].dpiByte) {
    throw 'Cobra DPI conversion contract failed.'
}
$recipes.pool += ,@([pscustomobject]@{op='build'; fn='unknown_new_dpi_protocol'; args=@()})
$recipes.drivers.mouse.'write:dpi'.pids.'00a3' = $recipes.pool.Count - 1
ConvertTo-Json $recipes -Depth 80 -Compress | Set-Content -LiteralPath $modifiedFile -Encoding utf8
$sentinel = Join-Path $fixture 'must-not-change.json'
'keep-existing-data' | Set-Content -LiteralPath $sentinel -Encoding utf8
$rejected = $false
try { & "$project/tools/import-reference.ps1" -Reference (Join-Path $fixture 'reference') -OutputFile $sentinel }
catch {
    if ($_.Exception.Message -notlike 'Unknown DPI recipe*') { throw }
    $rejected = $true
}
if (!$rejected -or (Get-Content -LiteralPath $sentinel -Raw).Trim() -ne 'keep-existing-data') {
    throw 'An unknown protocol was not rejected before modifying the output.'
}
Write-Output 'PASS: shifted pool indices, Cobra DPI mapping, and rejection of unknown DPI protocols without overwriting data.'
