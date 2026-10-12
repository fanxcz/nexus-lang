$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$runner = Join-Path $root 'Run-NEXUS.bat'
if (-not (Test-Path -LiteralPath $runner)) { throw "Run-NEXUS.bat not found: $runner" }
$key = 'HKCU:\Software\Classes\SystemFileAssociations\.nx\shell\RunWithNEXUS'
New-Item -Path $key -Force | Out-Null
Set-Item -Path $key -Value 'Run with NEXUS'
Set-ItemProperty -Path $key -Name 'Icon' -Value (Join-Path $root 'nexus.exe')
$commandKey = Join-Path $key 'command'
New-Item -Path $commandKey -Force | Out-Null
$command = '"{0}" /d /c ""{1}" "%1""' -f $env:ComSpec, $runner
Set-Item -Path $commandKey -Value $command
Write-Host 'Context menu added: .nx -> Run with NEXUS'
Write-Host 'Keep this NEXUS folder in the same location while the menu entry is installed.'
