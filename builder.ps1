param(
    [string]$mode = "build",
    [string]$exe  = "aeroUI"
)

function _build {
    cmake -B build -S .
    cmake --build build
}

function _run {
    & ".\build\Debug\$($exe).exe"
}

if ($mode -eq "build") {
    _build
}
elseif ($mode -eq "run") {
    _run
}
elseif ($mode -eq "exec") {
    _build
    _run
}
else {
    Write-Host "Unknown command: $($mode)"
    Write-Host "
Available commands:
- build
- run
- exec ; (build and run)
    "
}