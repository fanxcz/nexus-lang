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
$Found = $false
foreach ($Entry in $Entries) {
    try {
        $NormalizedEntry = [System.IO.Path]::GetFullPath($Entry).TrimEnd($trimChars)
        if ([string]::Equals($NormalizedEntry, $NormalizedRoot, [StringComparison]::OrdinalIgnoreCase)) {
            $Found = $true
            break
        }
    } catch {
        # Preserve unrelated PATH entries that are not plain file-system paths.
    }
}
if (-not $Found) {
    $Entries += $Root
    [Environment]::SetEnvironmentVariable('Path', ($Entries -join ';'), 'User')
    Write-Host '[NEXUS] Added this folder to your user PATH:' -ForegroundColor Green
    Write-Host "        $Root"
} else {
    Write-Host '[NEXUS] This folder is already in your user PATH.' -ForegroundColor Green
}

Write-Host ''
Write-Host 'Close this terminal and open a new PowerShell/CMD window.' -ForegroundColor Yellow
Write-Host 'Then test:'
Write-Host '  nexus --version'
Write-Host '  nexus "C:\path\to\hello.nx"'
Write-Host ''
Write-Host 'Keep the NEXUS folder in this location; moving it later requires running this installer again.'
