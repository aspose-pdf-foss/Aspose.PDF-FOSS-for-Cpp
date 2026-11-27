<#
.SYNOPSIS
  Builds the native libraries and packs the Aspose.PDF.Cpp.FOSS NuGet package.

.DESCRIPTION
  For every requested platform (x64, x86, ARM64):
    1. configures the project with the Visual Studio generator;
    2. builds Release (plus tests and examples) and runs the test suite when
       the host can execute that architecture, then builds the Debug library;
    3. installs both configurations and stages the libraries.
  Then it stages the public headers, the MSBuild .targets, the license texts
  and the package README, runs `nuget pack`, extracts the resulting .nupkg
  and builds nuget/test/PackageConsumer.vcxproj against it for every
  platform/configuration (running it where the host can).

  Debug libraries carry embedded debug info (/Z7), so consumers don't get
  LNK4099 "PDB not found" warnings.

  Requires: Visual Studio 2022+ with the C++ x64/x86 and ARM64 build tools,
  CMake 3.21+, Python 3. nuget.exe is downloaded if it is not on PATH.

.EXAMPLE
  ./nuget/pack.ps1
  ./nuget/pack.ps1 -Version <X.Y[.Z]>-rc.1 -Platforms x64 -OutputDir dist

.NOTES
  Publishing is a separate, explicit step:
    dotnet nuget push <nupkg> --api-key <key> --source https://api.nuget.org/v3/index.json
#>
[CmdletBinding()]
param(
    # Package version. Defaults to project(VERSION) in CMakeLists.txt.
    [string]$Version,
    [ValidateSet('x64', 'x86', 'ARM64')]
    [string[]]$Platforms = @('x64', 'x86', 'ARM64'),
    # Where the .nupkg is written. Defaults to build/nuget/out.
    [string]$OutputDir,
    # Skip the library test suite (the package consumer test still runs).
    [switch]$SkipTests,
    [string]$CMake = 'cmake',
    [string]$CTest = 'ctest'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 3.0

$PackageId = 'Aspose.PDF.Cpp.FOSS'
$RepoRoot  = Split-Path -Parent $PSScriptRoot
$WorkDir   = Join-Path $RepoRoot 'build\nuget'
$StageDir  = Join-Path $WorkDir 'stage'
if (-not $OutputDir) { $OutputDir = Join-Path $WorkDir 'out' }
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)

$CMakeArch = @{ 'x64' = 'x64'; 'x86' = 'Win32'; 'ARM64' = 'ARM64' }
$HostArch  = if ($env:PROCESSOR_ARCHITEW6432) { $env:PROCESSOR_ARCHITEW6432 } else { $env:PROCESSOR_ARCHITECTURE }

function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments)
    Write-Host "> $Exe $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

function Test-CanRun([string]$Platform) {
    if ($Platform -eq 'ARM64') { return $HostArch -eq 'ARM64' }
    return $true
}

function Get-ProjectVersion {
    $cml = Get-Content (Join-Path $RepoRoot 'CMakeLists.txt') -Raw
    if ($cml -notmatch 'project\(Aspose_PDF_FOSS VERSION (\d+\.\d+(?:\.\d+)?)[ )]') {
        throw 'Could not read project VERSION from CMakeLists.txt'
    }
    return $Matches[1]
}

function Get-NuGetExe {
    $cmd = Get-Command nuget.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $local = Join-Path $WorkDir 'tools\nuget.exe'
    if (-not (Test-Path $local)) {
        New-Item -ItemType Directory -Force (Split-Path $local) | Out-Null
        Write-Host 'Downloading nuget.exe'
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile $local -UseBasicParsing
    }
    return $local
}

function Get-MSBuildExe {
    $cmd = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($found) { return $found }
    }
    $found = Get-ChildItem "$env:ProgramFiles\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending | Select-Object -First 1
    if ($found) { return $found.FullName }
    throw 'MSBuild.exe not found; install Visual Studio 2022 or newer with the C++ workload.'
}

function Reset-Directory([string]$Path) {
    # Only ever called on directories under build/nuget.
    if (-not $Path.StartsWith($WorkDir, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clear $Path (outside $WorkDir)"
    }
    if (Test-Path $Path) { Remove-Item -Recurse -Force $Path }
    New-Item -ItemType Directory -Force $Path | Out-Null
}

if (-not $Version) { $Version = Get-ProjectVersion }
$baseVersion = ($Version -split '-')[0]
if ($baseVersion -ne (Get-ProjectVersion)) {
    throw "Version $Version does not match project(VERSION $(Get-ProjectVersion)) in CMakeLists.txt"
}
$commit = (& git -C $RepoRoot rev-parse HEAD 2>$null)
if (-not $commit) { $commit = 'unknown' }

Write-Host "== $PackageId $Version ($($Platforms -join ', ')) from $commit" -ForegroundColor Cyan

Reset-Directory $StageDir
$nativeDir = Join-Path $StageDir 'build\native'
New-Item -ItemType Directory -Force $nativeDir | Out-Null
New-Item -ItemType Directory -Force $OutputDir | Out-Null

# ── 1. Build, test and install every platform ────────────────────────────
foreach ($platform in $Platforms) {
    Write-Host "== $platform" -ForegroundColor Cyan
    $buildDir   = Join-Path $WorkDir "build-$platform"
    $installDir = Join-Path $WorkDir "install-$platform"
    Reset-Directory $installDir

    Invoke-Native $CMake @('-S', $RepoRoot, '-B', $buildDir, '-A', $CMakeArch[$platform], '-Wno-dev',
        '-DCMAKE_POLICY_DEFAULT_CMP0141=NEW', '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded')

    $runTests = (-not $SkipTests) -and (Test-CanRun $platform)
    if ($runTests) {
        Invoke-Native $CMake @('--build', $buildDir, '--config', 'Release', '--parallel')
        Invoke-Native $CTest @('--test-dir', $buildDir, '--build-config', 'Release', '--output-on-failure')
    } else {
        Invoke-Native $CMake @('--build', $buildDir, '--config', 'Release', '--target', 'aspose_pdf_foss', '--parallel')
    }
    Invoke-Native $CMake @('--build', $buildDir, '--config', 'Debug', '--target', 'aspose_pdf_foss', '--parallel')

    foreach ($config in 'Release', 'Debug') {
        $prefix = Join-Path $installDir $config
        Invoke-Native $CMake @('--install', $buildDir, '--config', $config, '--prefix', $prefix)
        $libDir = Join-Path $nativeDir "lib\$platform\$config"
        New-Item -ItemType Directory -Force $libDir | Out-Null
        Copy-Item (Join-Path $prefix 'lib\aspose_pdf_foss.lib') $libDir
    }
}

# ── 2. Stage headers, MSBuild integration, licenses, README ──────────────
$firstInstall = Join-Path $WorkDir "install-$($Platforms[0])\Release"
Copy-Item (Join-Path $firstInstall 'include') $nativeDir -Recurse
Copy-Item (Join-Path $PSScriptRoot "$PackageId.targets") $nativeDir

$licenseDir = Join-Path $StageDir 'licenses'
New-Item -ItemType Directory -Force $licenseDir | Out-Null
Copy-Item (Join-Path $firstInstall 'share\doc\aspose_pdf_foss\*') $licenseDir
Copy-Item (Join-Path $PSScriptRoot 'README.md') $StageDir
# 128x128 PNG rendered from https://products.aspose.com/pdf/images/aspose_pdf-for-cpp.svg
# (NuGet accepts only PNG/JPEG icons packed inside the package).
Copy-Item (Join-Path $PSScriptRoot 'icon.png') $StageDir

# ── 3. Pack ──────────────────────────────────────────────────────────────
$nuget = Get-NuGetExe
Invoke-Native $nuget @('pack', (Join-Path $PSScriptRoot "$PackageId.nuspec"),
    '-BasePath', $StageDir, '-OutputDirectory', $OutputDir,
    '-Properties', "version=$Version;commit=$commit",
    '-NoDefaultExcludes', '-NonInteractive')
# NuGet normalizes the version to at least three parts (26.11 -> 26.11.0)
# in the file name and in the packed .nuspec.
$nugetVersion = $Version -replace '^(\d+\.\d+)(?=$|-)', '$1.0'
$nupkg = Join-Path $OutputDir "$PackageId.$nugetVersion.nupkg"
if (-not (Test-Path $nupkg)) { throw "Expected package not found: $nupkg" }

# ── 4. Verify the packed .nupkg as a Visual Studio consumer ──────────────
$verifyDir = Join-Path $WorkDir 'verify'
Reset-Directory $verifyDir
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($nupkg, (Join-Path $verifyDir 'package'))
$targets = Join-Path $verifyDir "package\build\native\$PackageId.targets"

$msbuild = Get-MSBuildExe
$consumer = Join-Path $PSScriptRoot 'test\PackageConsumer.vcxproj'
foreach ($platform in $Platforms) {
    $msbuildPlatform = $CMakeArch[$platform]
    foreach ($config in 'Release', 'Debug') {
        $out = Join-Path $verifyDir "$platform\$config\"
        Invoke-Native $msbuild @($consumer, '-nologo', '-verbosity:minimal', '-restore:false',
            "-p:Platform=$msbuildPlatform", "-p:Configuration=$config",
            "-p:AsposePdfFossTargets=$targets", "-p:OutDir=$out", "-p:IntDir=$(Join-Path $out 'obj\')")
        if (Test-CanRun $platform) {
            Invoke-Native (Join-Path $out 'PackageConsumer.exe') @()
        } else {
            Write-Host "  (built only: $platform binaries cannot run on a $HostArch host)"
        }
    }
}

$hash = (Get-FileHash $nupkg -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host ''
Write-Host "Package: $nupkg" -ForegroundColor Green
Write-Host "SHA256:  $hash"
Write-Host "Publish: dotnet nuget push `"$nupkg`" --api-key <key> --source https://api.nuget.org/v3/index.json"
