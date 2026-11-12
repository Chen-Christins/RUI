# Bootstrap RUI build environment and build the test app.
# Downloads the MinGW 13.1.0 toolchain and ninja if missing, then runs CMake.
# Usage: powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

function Test-Aqt {
    python -c "import aqt" *> $null
    return ($LASTEXITCODE -eq 0)
}

function Ensure-Aqt {
    if (Test-Aqt) {
        return
    }
    Write-Host "aqtinstall not found; installing via pip..."
    python -m pip install --user aqtinstall
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install aqtinstall"
    }
}

# MinGW 13.1.0 toolchain (matches Qt 6.8.0 win64_mingw build).
$mingwGpp = Join-Path $root ".deps\tools\Tools\mingw1310_64\bin\g++.exe"
if (-not (Test-Path $mingwGpp)) {
    Write-Host "Downloading MinGW 13.1.0 toolchain..."
    Ensure-Aqt
    python -m aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O (Join-Path $root ".deps\tools")
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install MinGW 13.1.0 toolchain"
    }
}

# Ninja build tool.
$ninja = Join-Path $root ".deps\tools\ninja\ninja.exe"
if (-not (Test-Path $ninja)) {
    Write-Host "Downloading ninja..."
    New-Item -ItemType Directory -Force -Path (Split-Path $ninja) | Out-Null
    $zip = Join-Path $env:TEMP "ninja-win.zip"
    Invoke-WebRequest -Uri "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip" -OutFile $zip
    Expand-Archive -Path $zip -DestinationPath (Split-Path $ninja) -Force
    Remove-Item $zip -Force
}

# Configure (auto-downloads Qt and spdlog) and build.
cmake --preset mingw-debug
if ($LASTEXITCODE -ne 0) {
    throw "cmake configure failed"
}
cmake --build --preset mingw-debug
if ($LASTEXITCODE -ne 0) {
    throw "cmake build failed"
}

Write-Host "Build complete: $(Join-Path $root 'bin\test.exe')"
