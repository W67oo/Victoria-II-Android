# ==============================================================================
# scripts/env.ps1
# Activates the portable D:\AndroidDev build environment in the current shell
# without polluting C:\ or global system state.
# ==============================================================================

$ErrorActionPreference = "Stop"

$DevRoot = "D:\AndroidDev"
$env:TEMP              = "$DevRoot\temp"
$env:TMP               = "$DevRoot\temp"
$env:GRADLE_USER_HOME  = "$DevRoot\gradle-home"
$env:ANDROID_USER_HOME = "$DevRoot\.android"
$env:ANDROID_HOME      = "$DevRoot\sdk"
$env:ANDROID_SDK_ROOT  = "$DevRoot\sdk"
$env:ANDROID_NDK_HOME  = "$DevRoot\sdk\ndk\27.2.12479018"

$JdkDir = Get-ChildItem "$DevRoot\jdk" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
if ($JdkDir) {
    $env:JAVA_HOME = $JdkDir.FullName
}

$CmakeSub = Get-ChildItem "$DevRoot\cmake" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
$CmakeBin = if ($CmakeSub) { "$($CmakeSub.FullName)\bin" } else { "$DevRoot\cmake\bin" }

$MingwSub = Get-ChildItem "$DevRoot\llvm-mingw" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
$MingwBin = if ($MingwSub) { "$($MingwSub.FullName)\bin" } else { "$DevRoot\llvm-mingw\bin" }

$GradleSub = Get-ChildItem "$DevRoot\gradle" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
$GradleBin = if ($GradleSub) { "$($GradleSub.FullName)\bin" } else { "$DevRoot\gradle\bin" }

$PrependPaths = @(
    "$env:JAVA_HOME\bin",
    $CmakeBin,
    "$DevRoot\ninja",
    $MingwBin,
    $GradleBin,
    "$DevRoot\sdk\cmdline-tools\latest\bin",
    "$DevRoot\sdk\platform-tools"
) | Where-Object { $_ -and (Test-Path $_) }

$env:PATH = ($PrependPaths -join ";") + ";" + $env:PATH
