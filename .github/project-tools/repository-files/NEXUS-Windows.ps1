param(
    [ValidateSet('Run','Build','Check','Install')]
    [string]$Mode = 'Run',
    [string]$SourcePath = ''
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Compiler = Join-Path $Root 'bin\nexus-core.exe'
if (-not (Test-Path -LiteralPath $Compiler)) {
    # Backward compatibility with older portable ZIP layouts.
    $Compiler = Join-Path $Root 'nexus.exe'
}
$Runtime = Join-Path $Root 'runtime'

function Write-Info([string]$Message) { Write-Host "[NEXUS] $Message" -ForegroundColor Cyan }
function Write-Fail([string]$Message) { Write-Host "[NEXUS ERROR] $Message" -ForegroundColor Red }

function Find-VsDevCmd {
    $pf86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $pf = [Environment]::GetFolderPath('ProgramFiles')
    $vswhereCandidates = @(
        (Join-Path $pf86 'Microsoft Visual Studio\Installer\vswhere.exe'),
        (Join-Path $pf 'Microsoft Visual Studio\Installer\vswhere.exe')
    )
    foreach ($vswhere in $vswhereCandidates) {
        if (Test-Path -LiteralPath $vswhere) {
            $vsPath = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null | Select-Object -First 1)
            if ($vsPath) {
                $candidate = Join-Path $vsPath 'Common7\Tools\VsDevCmd.bat'
                if (Test-Path -LiteralPath $candidate) { return $candidate }
            }
        }
    }
    $base = Join-Path $pf86 'Microsoft Visual Studio'
    foreach ($edition in @('2022\BuildTools','2022\Community','2022\Professional','2022\Enterprise','2019\BuildTools','2019\Community')) {
        $candidate = Join-Path (Join-Path $base $edition) 'Common7\Tools\VsDevCmd.bat'
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    return $null
}

function Import-VsEnvironment {
    $vsDevCmd = Find-VsDevCmd
    if (-not $vsDevCmd) { return $false }
    $tmpCmd = Join-Path $env:TEMP ("nexus-vsenv-{0}.cmd" -f [guid]::NewGuid().ToString('N'))
    $outFile = Join-Path $env:TEMP ("nexus-vsenv-{0}.txt" -f [guid]::NewGuid().ToString('N'))
    try {
        @(
            '@echo off',
            ('call "{0}" -arch=x64 -host_arch=x64 >nul' -f $vsDevCmd),
            'if errorlevel 1 exit /b 1',
            ('set > "{0}"' -f $outFile)
        ) | Set-Content -LiteralPath $tmpCmd -Encoding ASCII
        $proc = Start-Process -FilePath $env:ComSpec -ArgumentList @('/d','/c',('"{0}"' -f $tmpCmd)) -Wait -PassThru -WindowStyle Hidden
        if ($proc.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $outFile)) { return $false }
        foreach ($line in Get-Content -LiteralPath $outFile) {
            if ($line -match '^([^=]+)=(.*)$') {
                $name = $matches[1]
                $value = $matches[2]
                if ($name -match '^[A-Za-z_][A-Za-z0-9_()]*$') {
                    [Environment]::SetEnvironmentVariable($name, $value, 'Process')
                }
            }
        }
        return $true
    } finally {
        Remove-Item -LiteralPath $tmpCmd,$outFile -Force -ErrorAction SilentlyContinue
    }
}

function Find-Clang {
    # Prefer the portable NEXUS toolchain. It is bundled in the official ZIP,
    # so the user does not need to install LLVM or Visual Studio Build Tools.
    $portableZig = Join-Path $Root 'toolchain\zig.exe'
    $portableCompiler = Join-Path $Root 'bin\clang++.cmd'
    if ((Test-Path -LiteralPath $portableZig) -and (Test-Path -LiteralPath $portableCompiler)) {
        return $portableCompiler
    }

    $command = Get-Command 'clang++.exe' -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $pf = [Environment]::GetFolderPath('ProgramFiles')
    $pf86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $candidates = @(
        (Join-Path $pf 'LLVM\bin\clang++.exe'),
        (Join-Path $pf86 'LLVM\bin\clang++.exe'),
        (Join-Path $env:LOCALAPPDATA 'Programs\LLVM\bin\clang++.exe'),
        'C:\tools\llvm\bin\clang++.exe'
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            $env:PATH = (Split-Path -Parent $candidate) + ';' + $env:PATH
            return $candidate
        }
    }
    return $null
}

function Initialize-Toolchain {
    if (-not (Test-Path -LiteralPath $Compiler)) { throw "nexus.exe was not found in $Root" }
    $bundledZig = Join-Path $Root 'toolchain\zig.exe'
    $hasBundledZig = Test-Path -LiteralPath $bundledZig
    $vsLoaded = $false
    if (-not $hasBundledZig) { $vsLoaded = Import-VsEnvironment }
    $clang = Find-Clang
    if (-not $clang) {
        throw "The NEXUS compiler toolchain was not found. Re-extract the complete portable ZIP, including its toolchain folder."
    }
    if (-not $hasBundledZig -and -not $vsLoaded -and -not (Get-Command 'link.exe' -ErrorAction SilentlyContinue)) {
        throw "No native linker was found. Install LLVM/Clang and Visual Studio C++ Build Tools, or use the complete NEXUS portable ZIP."
    }
    $env:PATH = (Split-Path -Parent $clang) + ';' + $env:PATH
    # The generated EXE may live beside the user's .nx file, so expose the
    # portable package runtime directory through PATH for LoadLibraryA("SDL2.dll").
    if (Test-Path -LiteralPath $Runtime) { $env:PATH = $Runtime + ';' + $env:PATH }
    $env:NEXUS_RUNTIME_DIR = $Runtime
    $env:NEXUS_STD_DIR = Join-Path $Root 'stdlib'
    return $clang
}

function Install-Requirements {
    $bundledZig = Join-Path $Root 'toolchain\zig.exe'
    $bundledCompiler = Join-Path $Root 'bin\clang++.cmd'
    if ((Test-Path -LiteralPath $bundledZig) -and (Test-Path -LiteralPath $bundledCompiler)) {
        Write-Info 'The native compiler and linker are bundled. No LLVM or Visual Studio installation is needed.'
        Write-Info 'Run Check-NEXUS-Toolchain.bat to verify the portable toolchain.'
        return 0
    }

    $winget = Get-Command 'winget.exe' -ErrorAction SilentlyContinue
    $clang = Find-Clang
    if ($clang) {
        Write-Info ("LLVM/Clang already found: {0}" -f $clang)
    } elseif ($winget) {
        Write-Info 'Installing LLVM/Clang through winget...'
        & $winget.Source install --id LLVM.LLVM --exact --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -ne 0) {
            Write-Host '[NOTICE] winget could not install LLVM automatically. Use the official LLVM link below.'
        }
    } else {
        Write-Host '[NOTICE] winget was not found; install LLVM/Clang manually.'
    }

    $vsDevCmd = Find-VsDevCmd
    if (-not $vsDevCmd) {
        if ($winget) {
            $answer = Read-Host 'Visual Studio C++ Build Tools are missing. Install them now? This may download several GB [y/N]'
            if ($answer -match '^(y|yes)$') {
                Write-Info 'Installing Visual Studio Build Tools with the C++ workload. This can take a long time.'
                & $winget.Source install --id Microsoft.VisualStudio.2022.BuildTools --exact --accept-source-agreements --accept-package-agreements --override '--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
                if ($LASTEXITCODE -ne 0) { Write-Host '[NOTICE] Build Tools installation needs attention; use the official link below.' }
            } else {
                Write-Host 'Install Build Tools later, then run Check-NEXUS-Toolchain.bat.'
            }
        } else {
            Write-Host '[NOTICE] winget was not found; install Visual Studio Build Tools manually.'
        }
    } else {
        Write-Info 'Visual Studio C++ Build Tools already detected.'
    }

    Write-Host ''
    Write-Host 'Official installation pages:'
    Write-Host '  LLVM/Clang: https://github.com/llvm/llvm-project/releases'
    Write-Host '  Visual Studio Build Tools: https://visualstudio.microsoft.com/visual-cpp-build-tools/'
    Write-Host 'Select Desktop development with C++ and a Windows 10/11 SDK.'
    Write-Host 'After installing prerequisites, open a new terminal and run Check-NEXUS-Toolchain.bat.'
    return 0
}

try {
    if ($Mode -eq 'Install') {
        $result = Install-Requirements
        exit $result
    }

    $clang = Initialize-Toolchain
    if ($Mode -eq 'Check') {
        Write-Info ("Compiler: {0}" -f (& $clang --version | Select-Object -First 1))
        $testDir = Join-Path $env:TEMP ("nexus-check-{0}" -f [guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $testDir | Out-Null
        try {
            $testCpp = Join-Path $testDir 'check.cpp'
            $testExe = Join-Path $testDir 'check.exe'
            'int main() { return 0; }' | Set-Content -LiteralPath $testCpp -Encoding ASCII
            & $clang $testCpp '-std=c++20' '-o' $testExe
            if ($LASTEXITCODE -ne 0) { throw 'The bundled or detected C++ toolchain could not compile/link a Windows executable.' }
            & $testExe
            if ($LASTEXITCODE -ne 0) { throw 'The test executable did not start correctly.' }
            Write-Info 'Toolchain check passed: a native Windows EXE was compiled and launched.'
        } finally {
            Remove-Item -LiteralPath $testDir -Recurse -Force -ErrorAction SilentlyContinue
        }
        exit 0
    }

    if ([string]::IsNullOrWhiteSpace($SourcePath)) {
        $projectRoot = Get-Location
        $manifest = Join-Path $projectRoot.Path 'nexus.toml'
        if (($Mode -eq 'Run' -or $Mode -eq 'Build') -and (Test-Path -LiteralPath $manifest)) {
            Push-Location $projectRoot.Path
            try {
                if ($Mode -eq 'Run') {
                    Write-Info "Running project $($projectRoot.Path)"
                    & $Compiler 'run'
                    $exitCode = $LASTEXITCODE
                } else {
                    Write-Info "Building project $($projectRoot.Path)"
                    $buildOutput = @(& $Compiler 'build' 2>&1)
                    $exitCode = $LASTEXITCODE
                    $buildOutput | ForEach-Object { Write-Output $_ }
                    if ($exitCode -eq 0) {
                        $builtExecutable = $null
                        foreach ($line in $buildOutput) {
                            if ("$line" -match '^build: OK -> (.+)$') {
                                $builtExecutable = [System.IO.Path]::GetFullPath($matches[1])
                                break
                            }
                        }
                        if ($builtExecutable -and (Test-Path -LiteralPath $builtExecutable -PathType Leaf) -and (Test-Path -LiteralPath $Runtime)) {
                            $outputDirectory = Split-Path -Parent $builtExecutable
                            foreach ($runtimeDll in Get-ChildItem -LiteralPath $Runtime -Filter '*.dll' -File -ErrorAction SilentlyContinue) {
                                Copy-Item -LiteralPath $runtimeDll.FullName -Destination (Join-Path $outputDirectory $runtimeDll.Name) -Force
                            }
                        }
                        Write-Info 'Build completed. Bundled runtime DLLs were copied beside the EXE.'
                    }
                }
            } finally {
                Pop-Location
            }
            exit $exitCode
        }

        if ($Mode -eq 'Build') {
            throw 'No source file was supplied and no nexus.toml project was found in the current folder.'
        }
        $SourcePath = Join-Path $Root 'MyPrograms\hello.nx'
    }
    $SourcePath = [System.IO.Path]::GetFullPath($SourcePath)
    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) { throw "Source file was not found: $SourcePath" }
    if ([IO.Path]::GetExtension($SourcePath) -ine '.nx') { throw 'Choose a source file with the .nx extension.' }

    Push-Location (Split-Path -Parent $SourcePath)
    try {
        if ($Mode -eq 'Run') {
            Write-Info "Running $SourcePath"
            & $Compiler 'run' $SourcePath
            $exitCode = $LASTEXITCODE
        } elseif ($Mode -eq 'Build') {
            $outputPath = Join-Path (Split-Path -Parent $SourcePath) (([IO.Path]::GetFileNameWithoutExtension($SourcePath)) + '.exe')
            Write-Info "Building $SourcePath"
            Write-Info "Output: $outputPath"
            & $Compiler 'build' $SourcePath '-o' $outputPath
            $exitCode = $LASTEXITCODE
            if ($exitCode -eq 0) {
                $generatedIr = $outputPath + '.ll'
                if (Test-Path -LiteralPath $generatedIr) { Remove-Item -LiteralPath $generatedIr -Force -ErrorAction SilentlyContinue }

                # Make each generated Windows program portable on its own:
                # copy the bundled graphics/audio and database runtime DLLs
                # beside the EXE, where the Windows loader searches by default.
                if (Test-Path -LiteralPath $Runtime) {
                    $outputDirectory = Split-Path -Parent $outputPath
                    foreach ($runtimeDll in Get-ChildItem -LiteralPath $Runtime -Filter '*.dll' -File -ErrorAction SilentlyContinue) {
                        Copy-Item -LiteralPath $runtimeDll.FullName -Destination (Join-Path $outputDirectory $runtimeDll.Name) -Force
                    }
                }
                Write-Info 'Build completed. Bundled runtime DLLs were copied beside the EXE.'
            }
        }
    } finally {
        Pop-Location
    }
    exit $exitCode
} catch {
    Write-Fail $_.Exception.Message
    exit 1
}
