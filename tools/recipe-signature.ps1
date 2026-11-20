function Get-TextSignature([string]$Text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Get-SourceFileHash([string]$Path) {
    $text = [IO.File]::ReadAllText($Path).Replace("`r`n", "`n")
    # A release version bump alone does not change the wire protocol.
    if ([IO.Path]::GetFileName($Path) -eq 'razercommon.h') {
        $text = [regex]::Replace($text, '(?m)^#define DRIVER_VERSION [^\n]*\n', '')
    }
    return Get-TextSignature $text
}

function Get-RecipeSignature($Steps) {
    # Windows PowerShell 5 and PowerShell 7 escape these characters differently.
    $json = ConvertTo-Json -InputObject @($Steps) -Depth 80 -Compress
    $json = $json.Replace('\u003c', '<').Replace('\u003e', '>').Replace('\u0026', '&').Replace('\u0027', "'")
    return Get-TextSignature $json
}
