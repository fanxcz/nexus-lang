# Downloads

[Download the NEXUS language manual (PDF)](https://fanxcz.github.io/nexus-lang/pdf/NEXUS-Documentation.pdf){ .md-button }

Choose an installer for the simplest setup, or a portable ZIP if you do not want to install. Each binary/archive is built and smoke-tested in CI, and its matching plain-text SHA-256 file is published beside it. The site is deployed only after artifact validation and link checks pass.

## One-click Windows installation

The repository root includes [`Install-NEXUS.bat`](https://raw.githubusercontent.com/fanxcz/nexus-lang/main/Install-NEXUS.bat) and its companion [`Install-NEXUS.ps1`](https://raw.githubusercontent.com/fanxcz/nexus-lang/main/Install-NEXUS.ps1). Download both files into the same folder and run the BAT file, or run it from a checkout of the repository. It downloads the official NEXUS 2.1.1 Setup installer and its SHA-256 checksum, verifies the digest and recorded filename before execution, then opens the normal per-user installer. Administrator rights are not required.

<div class="admonition info">
<p class="admonition-title">Recommended Windows download</p>
For most users, install NEXUS with the **NEXUS Setup** installer. For NEXUS Paint, use the Paint installer or the portable ZIP. The portable ZIP is the best choice when you do not want to install anything.
</div>

<div class="grid cards" markdown>

-   :material-microsoft-windows: **NEXUS Setup · Windows 10/11 x64**

    Installs the NEXUS compiler for the current user, including the bundled fallback toolchain, runtime files and standard library. It can add NEXUS to your user PATH and create shortcuts; administrator rights are not required.

    [Download NEXUS Setup (.exe)](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Setup-2.1.1.exe){ .md-button .md-button--primary download="NEXUS-Setup-2.1.1.exe" }

    [Download SHA-256 checksum](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Setup-2.1.1.exe.sha256.txt){ download="NEXUS-Setup-2.1.1.exe.sha256.txt" }

    For a fresh source checkout, you can also launch [`Install-NEXUS.bat`](https://github.com/fanxcz/nexus-lang/blob/main/Install-NEXUS.bat). It downloads the official installer and verifies SHA-256 before opening the setup wizard.

-   :material-archive: **NEXUS Portable · Windows x64**

    Contains the compiler launcher, compiler, bundled Zig-based toolchain, SDL runtime DLLs, standard library, templates, examples and helper scripts. Extract the entire ZIP to a writable folder; do not move component folders separately.

    [Download portable ZIP](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS_Windows_Portable.zip){ .md-button .md-button--primary download="NEXUS_Windows_Portable.zip" }

    [Download SHA-256 checksum](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS_Windows_Portable.zip.sha256.txt){ download="NEXUS_Windows_Portable.zip.sha256.txt" }

-   :material-palette: **NEXUS Paint · Windows x64**

    Desktop raster drawing application with brush, eraser, line/rectangle tools, fill, colour palette, undo/redo and BMP save/load. The **EXE is an installer**, not the standalone app. It installs Paint with its runtime DLLs and creates a Start menu shortcut. To launch Paint without installation, download the portable ZIP below.

    [Download NEXUS Paint installer (.exe)](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.exe){ .md-button .md-button--primary download="NEXUS-Paint-Windows-x64.exe" }

    [Installer SHA-256 checksum](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.exe.sha256.txt){ download="NEXUS-Paint-Windows-x64.exe.sha256.txt" }

    Prefer no installation? The portable ZIP includes `NEXUS-Paint.exe`, its runtime DLLs and editable `.nx` source.

    [Download Paint portable ZIP](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.zip){ .md-button download="NEXUS-Paint-Windows-x64.zip" }

    [ZIP SHA-256 checksum](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Paint-Windows-x64.zip.sha256.txt){ download="NEXUS-Paint-Windows-x64.zip.sha256.txt" }

    [View Paint source](https://github.com/fanxcz/nexus-lang/blob/main/examples/nexus_paint.nx)

-   :material-code-tags: **NEXUS extension · VS Code**

    Syntax highlighting and snippets for `.nx` files, with project LSP integration guidance.

    [Download VSIX](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Language-2.1.0.vsix){ .md-button download="NEXUS-Language-2.1.0.vsix" }

    [VSIX SHA-256 checksum](https://fanxcz.github.io/nexus-lang/assets/downloads/NEXUS-Language-2.1.0.vsix.sha256.txt){ download="NEXUS-Language-2.1.0.vsix.sha256.txt" }

-   :material-monitor: **NEXUS Studio**

    Source-buildable Windows desktop editor with project explorer, source editor, compiler selection and Build/Run controls. Install the NEXUS compiler separately.

    [Source and build instructions](https://github.com/fanxcz/nexus-lang/tree/main/.github/project-tools/nexus-studio){ .md-button }

-   :material-laptop: **Linux and macOS**

    Compiler archives are published through the official GitHub Releases workflow when a version-tag build completes. The available architecture depends on the release assets currently listed.

    [Browse GitHub Releases](https://github.com/fanxcz/nexus-lang/releases){ .md-button }

</div>

## Install and verify NEXUS on Windows

1. Download `NEXUS-Setup-2.1.1.exe` above and run it.
2. Choose whether to create Start menu/desktop shortcuts and add the launcher to your user PATH.
3. Open a **new** PowerShell window and run:

```powershell
nexus --version
nexus doctor
nexus self-test
```

If the command is not found, reopen the terminal. You can test the launcher directly:

```powershell
& "$env:LOCALAPPDATA\Programs\NEXUS\nexus.cmd" --version
```

The installers are currently unsigned. Download them only from this official website or the project's GitHub Actions output; Windows may show an unsigned-app warning.

## Verify a download before running it

Every `.sha256.txt` link is a small plain-text file containing the SHA-256 digest and matching filename. It is not an executable. Compare the digest against the downloaded file before launching an installer or extracting an archive.

## Verify the portable ZIP checksum

This PowerShell example downloads the portable bundle and matching checksum, verifies SHA-256, and confirms the expected compiler launcher exists before extracting it.

```powershell
$ErrorActionPreference = 'Stop'
$base = 'https://fanxcz.github.io/nexus-lang/assets/downloads/'
$work = Join-Path $env:TEMP 'NEXUS-Windows-Portable'
New-Item -ItemType Directory -Path $work -Force | Out-Null
$zip = Join-Path $work 'NEXUS_Windows_Portable.zip'
$sum = Join-Path $work 'NEXUS_Windows_Portable.zip.sha256.txt'
$stage = Join-Path $work 'unpacked'

Invoke-WebRequest ($base + 'NEXUS_Windows_Portable.zip') -OutFile $zip
Invoke-WebRequest ($base + 'NEXUS_Windows_Portable.zip.sha256.txt') -OutFile $sum
$expected = ((Get-Content $sum -Raw).Trim() -split '\s+')[0].ToLowerInvariant()
$actual = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) { throw 'SHA-256 mismatch; archive rejected.' }

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
Expand-Archive -LiteralPath $zip -DestinationPath $stage -Force
$launcher = Get-ChildItem -LiteralPath $stage -Filter 'nexus.cmd' -File -Recurse | Select-Object -First 1
if (-not $launcher) { throw 'Archive does not contain nexus.cmd; do not install it.' }
Write-Host "Verified launcher: $($launcher.FullName)"
```

## Integrity and availability

The website copies artifacts created during the same workflow. Before publication, CI verifies that every expected download exists, executable files have valid Windows PE signatures, ZIP/VSIX archives are readable and contain required files, and each `.sha256.txt` value matches the corresponding file. After deployment, another workflow step requests every documentation page and download URL.
