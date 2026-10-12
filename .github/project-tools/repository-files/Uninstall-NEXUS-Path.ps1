$ErrorActionPreference = 'Stop'
$Root = [System.IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$trimChars = [char[]]@('\','/')
$NormalizedRoot = $Root.TrimEnd($trimChars)
$CurrentPath = [Environment]::GetEnvironmentVariable('Path', 'User')
if ([string]::IsNullOrWhiteSpace($CurrentPath)) {
    Write-Host '[NEXUS] The user PATH is empty; nothing to remove.'
    exit 0
}
$Entries = @($CurrentPath -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
$Filtered = @()
foreach ($Entry in $Entries) {
    $Keep = $true
    try {
        $NormalizedEntry = [System.IO.Path]::GetFullPath($Entry.Trim().Trim('"')).TrimEnd($trimChars)
        if ([string]::Equals($NormalizedEntry, $NormalizedRoot, [StringComparison]::OrdinalIgnoreCase)) { $Keep = $false }
    } catch {
        # Preserve unrelated entries.
    }
    if ($Keep) { $Filtered += $Entry }
}
[Environment]::SetEnvironmentVariable('Path', ($Filtered -join ';'), 'User')
Write-Host '[NEXUS] Removed the portable folder from your user PATH. Open a new terminal.' -ForegroundColor Green
