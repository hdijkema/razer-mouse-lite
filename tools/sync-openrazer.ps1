param(
    [Parameter(Mandatory)][string]$Upstream,
    [string]$Converter,
    [string]$Python = 'python',
    [switch]$CheckOnly,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/recipe-signature.ps1"
$project = Split-Path $PSScriptRoot -Parent
$checkpointPath = Join-Path $project 'data/openrazer-source.json'
$checkpoint = Get-Content -LiteralPath $checkpointPath -Raw | ConvertFrom-Json
$Upstream = (Resolve-Path -LiteralPath $Upstream).Path
$revision = (& git -C $Upstream rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') { throw 'Cannot identify the OpenRazer checkout.' }
function Output([string]$Name, [string]$Value) {
    Write-Output "${Name}=$Value"
    if ($env:GITHUB_OUTPUT) { "${Name}=$Value" | Out-File $env:GITHUB_OUTPUT -Append -Encoding utf8 }
}
function Canonical($Value) {
    if ($null -eq $Value) { return $null }
    if ($Value -is [Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in @($Value.Keys | Sort-Object)) { $result[$key] = Canonical $Value[$key] }
        return $result
    }
    if ($Value -is [pscustomobject]) {
        $result = [ordered]@{}
        foreach ($key in @($Value.PSObject.Properties.Name | Sort-Object)) { $result[$key] = Canonical $Value.$key }
        return $result
    }
    if ($Value -is [Collections.IEnumerable] -and $Value -isnot [string]) {
        return ,@(foreach ($item in $Value) { Canonical $item })
    }
    return $Value
}
$protocolFiles = [ordered]@{}
foreach ($file in $checkpoint.protocolFiles.PSObject.Properties.Name) {
    $hash = Get-SourceFileHash (Join-Path $Upstream $file)
    if ($hash -ne $checkpoint.protocolFiles.$file) {
        throw "OpenRazer changed $file. Review the C++ packet builders and update the protocol checkpoint before automatic conversion."
    }
    $protocolFiles[$file] = $hash
}
$mouseFiles = [ordered]@{}
$changed = $Force.IsPresent -or !$checkpoint.revision
foreach ($file in $checkpoint.mouseFiles.PSObject.Properties.Name) {
    $hash = Get-SourceFileHash (Join-Path $Upstream $file)
    $mouseFiles[$file] = $hash
    if ($hash -ne $checkpoint.mouseFiles.$file) { $changed = $true }
}
Output 'convert' $changed.ToString().ToLowerInvariant()
Output 'converter_ref' $checkpoint.converterRevision
if ($CheckOnly) { return }
Output 'changed' 'false'
if (!$changed) { Write-Output 'No relevant OpenRazer mouse changes.'; return }
if (!$Converter) { throw 'A pinned converter checkout is required.' }
$Converter = (Resolve-Path -LiteralPath $Converter).Path
$converterRevision = (& git -C $Converter rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $converterRevision -ne $checkpoint.converterRevision) { throw 'Converter checkout does not match the pinned revision.' }
$scratch = Join-Path $project ('build/openrazer-sync-' + [guid]::NewGuid().ToString('N'))
$generator = Join-Path $scratch 'generator'
$reference = Join-Path $scratch 'reference'
$data = Join-Path $reference 'openrazer_win/devices/data'
New-Item -ItemType Directory -Force $generator, $data | Out-Null
# Copy only these pinned generators: no pending PR patches or runtime from the
# Windows port. The generator reads the actual OpenRazer checkout above.
foreach ($file in @('extract_device_db.py', 'transpile_recipes.py', 'upstream_source.py')) {
    Copy-Item -LiteralPath (Join-Path $Converter "tools/$file") -Destination $generator
}
& $Python (Join-Path $generator 'extract_device_db.py') $Upstream (Join-Path $data 'devices.json')
if ($LASTEXITCODE -ne 0) { throw 'OpenRazer device extraction failed.' }
& $Python (Join-Path $generator 'transpile_recipes.py') $Upstream (Join-Path $data 'recipes.json')
if ($LASTEXITCODE -ne 0) { throw 'OpenRazer recipe conversion failed.' }
$candidatePath = Join-Path $scratch 'mice.json'
& "$PSScriptRoot/import-reference.ps1" -Reference $reference -OutputFile $candidatePath
$candidate = Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
$current = Get-Content -LiteralPath (Join-Path $project 'data/mice.json') -Raw | ConvertFrom-Json
$candidateJson = ConvertTo-Json -InputObject (Canonical @($candidate | Sort-Object pid)) -Depth 80 -Compress
$currentJson = ConvertTo-Json -InputObject (Canonical @($current | Sort-Object pid)) -Depth 80 -Compress
if ($candidateJson -eq $currentJson) { Write-Output 'Upstream changes do not affect the supported mouse definitions.'; return }
# Only the caller's isolated CI checkout is changed. Publication requires tests
# and a separate pull-request job; the app reads main, never this candidate.
[IO.File]::WriteAllText((Join-Path $project 'data/mice.json'), $candidateJson, (New-Object Text.UTF8Encoding $false))
$nextCheckpoint = [ordered]@{
    format = 1
    upstream = 'https://github.com/openrazer/openrazer'
    revision = $revision
    converterRevision = $converterRevision
    protocolRevision = $checkpoint.protocolRevision
    protocolFiles = $protocolFiles
    mouseFiles = $mouseFiles
}
[IO.File]::WriteAllText($checkpointPath, (ConvertTo-Json $nextCheckpoint -Depth 8), (New-Object Text.UTF8Encoding $false))
$summaryPath = Join-Path $project 'build/sync-pr-body.md'
$currentIds = @($current | ForEach-Object pid)
$candidateIds = @($candidate | ForEach-Object pid)
$added = @($candidate | Where-Object { $_.pid -notin $currentIds })
$removed = @($current | Where-Object { $_.pid -notin $candidateIds })
$updated = @($candidate | Where-Object {
    $model = $_
    $previous = $current | Where-Object pid -eq $model.pid | Select-Object -First 1
    $previous -and (ConvertTo-Json -InputObject (Canonical $previous) -Depth 80 -Compress) -ne (ConvertTo-Json -InputObject (Canonical $model) -Depth 80 -Compress)
})
$details = foreach ($group in @(@{name='Added';models=$added}, @{name='Removed';models=$removed}, @{name='Changed';models=$updated})) {
    foreach ($model in $group.models) { '- {0}: {1} (USB 1532:{2:x4})' -f $group.name, $model.name, $model.pid }
}
$detailsText = $details -join "`n"
$body = @"
Update the converted mouse definitions from [OpenRazer $revision](https://github.com/openrazer/openrazer/commit/$revision).

- Models: $($current.Count) -> $($candidate.Count).
- Converter: [openrazer-win $converterRevision](https://github.com/Ar4ikov/openrazer-win/commit/$converterRevision), with pending upstream patches excluded.
- DPI recipes are matched against reviewed contents, not pool indices.
- The generating workflow builds the candidate and runs the C++ validation, packet, persistence, translation and UI tests before opening this PR.

$detailsText

Review new devices and changed commands before merging. This PR is not automatically merged; the app continues to download the definitions on main until merge. The full platform build is explicitly dispatched for this branch.

Huge thanks to the OpenRazer community for its device support and protocol research, and to openrazer-win for the conversion tools.
"@
[IO.File]::WriteAllText($summaryPath, $body, (New-Object Text.UTF8Encoding $false))
Output 'changed' 'true'
