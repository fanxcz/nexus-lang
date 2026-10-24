$ErrorActionPreference = 'Stop'
$key = 'HKCU:\Software\Classes\SystemFileAssociations\.nx\shell\RunWithNEXUS'
if (Test-Path -LiteralPath $key) {
    Remove-Item -LiteralPath $key -Recurse -Force
    Write-Host 'NEXUS context-menu entry removed.'
} else {
    Write-Host 'NEXUS context-menu entry was not installed.'
}
