$b = [System.IO.File]::ReadAllBytes("$PSScriptRoot\..\CMakeLists.txt")
if ($b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF) {
    [System.IO.File]::WriteAllBytes("$PSScriptRoot\..\CMakeLists.txt", $b[3..($b.Length - 1)])
    Write-Output "BOM stripped."
} else {
    Write-Output "No BOM found."
}
