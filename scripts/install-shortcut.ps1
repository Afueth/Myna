<#
Creates a "Myna" shortcut with the Myna icon on your Desktop and in the Start
menu. Opening it starts Myna in its own terminal window.

Run it from the unzipped release folder, or from the project root after
building Myna (mingw32-make):
  powershell -ExecutionPolicy Bypass -File scripts/install-shortcut.ps1
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$icon = Join-Path $root 'assets\myna.ico'

# bin\ in a release download, build\ in a source checkout.
$exe = @('bin\myna.exe', 'build\myna.exe') |
    ForEach-Object { Join-Path $root $_ } |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

if (-not $exe) {
    Write-Error "myna.exe not found. Build Myna first with: mingw32-make"
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
