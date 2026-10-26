$ErrorActionPreference = 'Stop'

$version = '2.1.1'
$fileName = "NEXUS-Setup-$version.exe"
$baseUrl = 'https://fanxcz.github.io/nexus-lang/assets/downloads'
$installerUrl = "$baseUrl/$fileName"
$checksumUrl = "$installerUrl.sha256.txt"
$workDir = Join-Path $env:TEMP 'NEXUS-Official-Installer'
$installerPath = Join-Path $workDir $fileName
$checksumPath = Join-Path $workDir "$fileName.sha256.txt"

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    New-Item -ItemType Directory -Path $workDir -Force | Out-Null

    Write-Host "[NEXUS] Downloading $fileName from the official NEXUS website..."
    Invoke-WebRequest -UseBasicParsing -Uri $installerUrl -OutFile $installerPath
    Invoke-WebRequest -UseBasicParsing -Uri $checksumUrl -OutFile $checksumPath

    if (-not (Test-Path -LiteralPath $installerPath) -or
        (Get-Item -LiteralPath $installerPath).Length -lt 4096) {
        throw 'Downloaded installer is missing or unexpectedly small.'
    }

    $checksumText = (Get-Content -LiteralPath $checksumPath -Raw -Encoding ASCII).Trim()
    $match = [regex]::Match($checksumText, '^([0-9a-fA-F]{64})(?:\s+\*?(.+))?$')
    if (-not $match.Success) {
        throw 'The published checksum file is malformed; refusing to run the installer.'
    }

    if ($match.Groups[2].Success) {
        $recordedName = [IO.Path]::GetFileName($match.Groups[2].Value.Trim())
        if ($recordedName -ne $fileName) {
            throw "The checksum belongs to '$recordedName', not '$fileName'."
        }
    }

    $expectedHash = $match.Groups[1].Value.ToLowerInvariant()
    $actualHash = (Get-FileHash -LiteralPath $installerPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $expectedHash) {
        Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue
        throw 'SHA-256 mismatch. The installer was rejected and will not be executed.'
    }

    Write-Host '[NEXUS] SHA-256 verification passed.'
    Write-Host '[NEXUS] Launching the official setup wizard...'
    $process = Start-Process -FilePath $installerPath -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "The setup wizard returned exit code $($process.ExitCode)."
    }

    Write-Host '[NEXUS] Setup completed.'
    Write-Host 'Open a new terminal and run: nexus --version'
    exit 0
}
catch {
    Write-Error "[NEXUS] $($_.Exception.Message)"
    exit 1
}
finally {
    Remove-Item -LiteralPath $checksumPath -Force -ErrorAction SilentlyContinue
}
