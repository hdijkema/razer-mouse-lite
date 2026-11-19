param([string]$EventName = '', [string]$RefType = '', [string]$RefName = '', [string]$RequestedVersion = '')
$ErrorActionPreference = 'Stop'
$project = Get-Content (Join-Path $PSScriptRoot '../CMakeLists.txt') -Raw
if ($project -notmatch 'project\(razer-mouse VERSION ([0-9]+\.[0-9]+\.[0-9]+)') { throw 'Project version is missing.' }
$version = $Matches[1]
$tag = ''
$publish = $false
if ($EventName -eq 'push' -and $RefType -eq 'tag') {
    $tag = $RefName
    $version = $tag -creplace '^v', ''
    $publish = $true
}
if ($EventName -eq 'workflow_dispatch' -and $RequestedVersion) {
    $version = $RequestedVersion -creplace '^v', ''
    $tag = "v$version"
    $publish = $true
    if ($RefType -eq 'tag') {
        if ($RefName -cne $version -and $RefName -cne "v$version") { throw 'The version must match the selected tag.' }
        $tag = $RefName
    }
}
if ($version -notmatch '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$') { throw 'Use X.Y.Z without leading zeroes.' }
foreach ($part in $version.Split('.')) { if ([long]$part -gt 65535) { throw 'Version components must be between 0 and 65535.' } }
[pscustomobject]@{ Version = $version; Tag = $tag; Publish = $publish; Archive = "razer-mouse-lite-$version-windows-x64.zip" }
