$sourceDir = "D:\Non-SteamGames\Victoria II"
$destDir = "E:\Antigravity Work\Vic2Mobile\game_data"

Write-Host "Linking Victoria II game data from: $sourceDir"
Write-Host "Target directory: $destDir"

if (-not (Test-Path $sourceDir)) {
    Write-Error "Source directory does not exist: $sourceDir"
    exit 1
}

Get-ChildItem -Path $sourceDir -Directory | ForEach-Object {
    $targetPath = Join-Path $destDir $_.Name
    if (-not (Test-Path $targetPath)) {
        New-Item -ItemType Junction -Path $targetPath -Target $_.FullName | Out-Null
        Write-Host "Created Junction: $($_.Name) -> $($_.FullName)"
    } else {
        Write-Host "Already exists: $($_.Name)"
    }
}

Get-ChildItem -Path $sourceDir -File | ForEach-Object {
    $targetPath = Join-Path $destDir $_.Name
    if (-not (Test-Path $targetPath)) {
        New-Item -ItemType HardLink -Path $targetPath -Target $_.FullName | Out-Null
        Write-Host "Created HardLink: $($_.Name)"
    }
}

Write-Host "Game data linking complete!"
