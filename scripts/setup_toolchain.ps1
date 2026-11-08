# ==============================================================================
# scripts/setup_toolchain.ps1
# Downloads and installs portable JDK 17, CMake 3.30, Ninja, LLVM-MinGW (C++20),
# Gradle 8.10.2, and Android SDK/NDK r27 strictly into D:\AndroidDev.
# ==============================================================================

$ErrorActionPreference = "Stop"
$DevRoot = "D:\AndroidDev"
$DlDir   = "$DevRoot\downloads"
$SevenZip = "E:\Softwares\7-Zip\7z.exe"

New-Item -ItemType Directory -Force -Path $DevRoot, $DlDir, "$DevRoot\temp", "$DevRoot\gradle-home", "$DevRoot\.android", "$DevRoot\sdk" | Out-Null
$env:TEMP              = "$DevRoot\temp"
$env:TMP               = "$DevRoot\temp"
$env:ANDROID_USER_HOME = "$DevRoot\.android"

function Download-IfMissing([string]$Url, [string]$OutFile) {
    if ((Test-Path $OutFile) -and ((Get-Item $OutFile).Length -gt 102400)) {
        Write-Host "[SKIP] Already downloaded: $OutFile"
        return
    }
    Write-Host "[DL] Downloading $Url -> $OutFile ..."
    & curl.exe -fL --retry 3 --connect-timeout 20 -o $OutFile $Url
    if ($LASTEXITCODE -ne 0) { throw "Failed to download $Url" }
}

function Extract-Archive([string]$Archive, [string]$DestDir) {
    New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
    Write-Host "[UNPACK] Extracting $Archive -> $DestDir ..."
    & $SevenZip x $Archive "-o$DestDir" -y | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Failed to extract $Archive" }
}

# 1. Microsoft OpenJDK 17
$JdkZip = "$DlDir\jdk17.zip"
Download-IfMissing "https://aka.ms/download-jdk/microsoft-jdk-17-windows-x64.zip" $JdkZip
if (-not (Test-Path "$DevRoot\jdk")) { Extract-Archive $JdkZip "$DevRoot\jdk" }

# 2. CMake 3.30.5 & Ninja 1.12.1
$CmakeZip = "$DlDir\cmake-3.30.5.zip"
Download-IfMissing "https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-windows-x86_64.zip" $CmakeZip
if (-not (Test-Path "$DevRoot\cmake")) { Extract-Archive $CmakeZip "$DevRoot\cmake" }

$NinjaZip = "$DlDir\ninja-1.12.1.zip"
Download-IfMissing "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip" $NinjaZip
if (-not (Test-Path "$DevRoot\ninja\ninja.exe")) { Extract-Archive $NinjaZip "$DevRoot\ninja" }
$CmakeSubForNinja = Get-ChildItem "$DevRoot\cmake" -Directory | Select-Object -First 1
if ($CmakeSubForNinja -and -not (Test-Path "$($CmakeSubForNinja.FullName)\bin\ninja.exe")) {
    Copy-Item "$DevRoot\ninja\ninja.exe" "$($CmakeSubForNinja.FullName)\bin\ninja.exe" -Force
}

# 3. LLVM-MinGW (Clang 18 x86_64 UCRT C++20 host compiler for DataContainerGenerator)
$MingwZip = "$DlDir\llvm-mingw-ucrt-x86_64.zip"
Download-IfMissing "https://github.com/mstorsjo/llvm-mingw/releases/download/20240619/llvm-mingw-20240619-ucrt-x86_64.zip" $MingwZip
if (-not (Test-Path "$DevRoot\llvm-mingw")) { Extract-Archive $MingwZip "$DevRoot\llvm-mingw" }

# 4. Gradle 8.10.2
$GradleZip = "$DlDir\gradle-8.10.2-bin.zip"
Download-IfMissing "https://services.gradle.org/distributions/gradle-8.10.2-bin.zip" $GradleZip
if (-not (Test-Path "$DevRoot\gradle")) { Extract-Archive $GradleZip "$DevRoot\gradle" }

# 5. Android Command-line Tools
$CmdToolsZip = "$DlDir\commandlinetools-win.zip"
Download-IfMissing "https://dl.google.com/android/repository/commandlinetools-win-11076708_latest.zip" $CmdToolsZip
if (-not (Test-Path "$DevRoot\sdk\cmdline-tools\latest\bin\sdkmanager.bat")) {
    Extract-Archive $CmdToolsZip "$DevRoot\sdk\cmdline-tools-tmp"
    New-Item -ItemType Directory -Force -Path "$DevRoot\sdk\cmdline-tools" | Out-Null
    Move-Item -Force "$DevRoot\sdk\cmdline-tools-tmp\cmdline-tools" "$DevRoot\sdk\cmdline-tools\latest"
    Remove-Item -Recurse -Force "$DevRoot\sdk\cmdline-tools-tmp"
}

# Activate paths and install Android SDK 34 + NDK r27
. "$PSScriptRoot\env.ps1"

Write-Host "[SDK] Accepting Android SDK licenses and installing platform-tools, android-34, build-tools 34.0.0, NDK 27.2.12479018..."
$YesInput = ("y`n" * 30)
$YesInput | & "$DevRoot\sdk\cmdline-tools\latest\bin\sdkmanager.bat" "--sdk_root=$DevRoot\sdk" --licenses | Out-Null
& "$DevRoot\sdk\cmdline-tools\latest\bin\sdkmanager.bat" "--sdk_root=$DevRoot\sdk" `
    "platform-tools" `
    "platforms;android-34" `
    "build-tools;34.0.0" `
    "ndk;27.2.12479018"

# Write android/local.properties
$CmakeSub = Get-ChildItem "$DevRoot\cmake" -Directory | Select-Object -First 1
$CmakeEsc = $CmakeSub.FullName.Replace('\', '\\').Replace(':', '\:')
$LocalProps = @"
sdk.dir=D\:\\AndroidDev\\sdk
cmake.dir=$CmakeEsc
"@
Set-Content -Path "$PSScriptRoot\..\android\local.properties" -Value $LocalProps -Encoding UTF8

Write-Host "=== Toolchain Setup Complete! ==="
java -version
cmake --version
ninja --version
clang++ --version
