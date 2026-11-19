$ErrorActionPreference = 'Stop'
$resolver = Join-Path $PSScriptRoot '../tools/release-version.ps1'
function Assert-Release([bool]$Condition, [string]$Description) { if (!$Condition) { throw $Description } }
$build = & $resolver -EventName push -RefType branch -RefName main
Assert-Release (!$build.Publish -and  $build.Version -eq (& $resolver).Version) 'Ordinary pushes must only build.'
$tag = & $resolver -EventName push -RefType tag -RefName v1.2.3
Assert-Release ($tag.Publish -and $tag.Version -eq '1.2.3' -and $tag.Tag -eq 'v1.2.3') 'Tag version resolution failed.'
$manual = & $resolver -EventName workflow_dispatch -RefType branch -RefName main -RequestedVersion 2.3.4
Assert-Release ($manual.Publish -and $manual.Tag -eq 'v2.3.4') 'Manual release resolution failed.'
$pr = & $resolver -EventName pull_request -RequestedVersion 9.9.9
Assert-Release (!$pr.Publish) 'Pull requests may not publish.'
$empty = & $resolver -EventName workflow_dispatch
Assert-Release (!$empty.Publish) 'Empty manual input may not publish.'
foreach ($invalid in @('v','v01.2.3','v1.2.3-beta','v1.2.65536','V1.2.3')) {
    $rejected = $false
    try { & $resolver -EventName push -RefType tag -RefName $invalid | Out-Null } catch { $rejected = $true }
    Assert-Release $rejected "Invalid release accepted: $invalid"
}
$rejected = $false
try { & $resolver -EventName workflow_dispatch -RefType tag -RefName v1.2.3 -RequestedVersion 2.3.4 | Out-Null } catch { $rejected = $true }
Assert-Release $rejected 'Mismatched tag accepted.'
Write-Output 'PASS: branch, tag, manual, PR and invalid release version checks.'
