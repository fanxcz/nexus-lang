$ErrorActionPreference = 'Stop'
$Root = [System.IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$Compiler = Join-Path $Root 'nexus.cmd'
$CoreCompiler = Join-Path $Root 'bin\nexus-core.exe'
if (-not (Test-Path -LiteralPath $Compiler) -or -not (Test-Path -LiteralPath $CoreCompiler)) {
    throw "NEXUS portable compiler was not found in $Root. Extract the complete ZIP and run this script again."
}

$trimChars = [char[]]@('\','/')
$NormalizedRoot = $Root.TrimEnd($trimChars)
$CurrentPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$Entries = @()
if (-not [string]::IsNullOrWhiteSpace($CurrentPath)) {
    $Entries = @($CurrentPath -split ';' | ForEach-Object { $_.Trim().Trim('"') } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

# Prioritize this installation. Appending it would leave an older compiler
# earlier in PATH and Windows would continue launching that stale copy.
# Preserve unrelated entries and their original relative order.
$OtherEntries = @()
foreach ($Entry in $Entries) {
    try {
        $NormalizedEntry = [System.IO.Path]::GetFullPath($Entry).TrimEnd($trimChars)
        if ([string]::Equals($NormalizedEntry, $NormalizedRoot, [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
    } catch {
        # Preserve unrelated PATH entries that are not plain file-system paths.
    }
    $OtherEntries += $Entry
}
$NewPath = (@($Root) + $OtherEntries) -join ';'
if (-not [string]::Equals($NewPath, $CurrentPath, [StringComparison]::OrdinalIgnoreCase)) {
    [Environment]::SetEnvironmentVariable('Path', $NewPath, 'User')
    Write-Host '[NEXUS] Moved this installation to the beginning of your user PATH:' -ForegroundColor Green
    Write-Host "        $Root"
} else {
    Write-Host '[NEXUS] This installation is already first in your user PATH.' -ForegroundColor Green
}

Write-Host ''
Write-Host 'Close this terminal and open a new PowerShell/CMD window.' -ForegroundColor Yellow
Write-Host 'Then test:'
Write-Host '  Get-Command nexus -All'
Write-Host '  nexus --version'
Write-Host '  nexus "C:\path\to\hello.nx"'
Write-Host ''
Write-Host 'Keep the NEXUS folder in this location; moving it later requires running this installer again.'
