<#
Creates a "Myna" shortcut with the Myna icon on your Desktop and in the Start
menu. Opening it starts Myna in its own terminal window.

Build Myna first (mingw32-make), then run from the project root:
  powershell -ExecutionPolicy Bypass -File scripts/install-shortcut.ps1
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root 'build\myna.exe'
$icon = Join-Path $root 'assets\myna.ico'

if (-not (Test-Path $exe)) {
    Write-Error "build\myna.exe not found. Build Myna first with: mingw32-make"
}

$shell = New-Object -ComObject WScript.Shell
$folders = @(
    [Environment]::GetFolderPath('Desktop'),
    (Join-Path ([Environment]::GetFolderPath('Programs')) '')
)

foreach ($folder in $folders) {
    $path = Join-Path $folder 'Myna.lnk'
    $link = $shell.CreateShortcut($path)
    $link.TargetPath = $exe
    $link.WorkingDirectory = $root
    $link.IconLocation = "$icon,0"
    $link.Description = 'Myna - Developer Intent Engine'
    $link.Save()
    Write-Output "Created $path"
}
