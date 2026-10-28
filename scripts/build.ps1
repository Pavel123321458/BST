param(
    [ValidateSet('build','test','lab1','lab2','benchmark')][string]$Action = 'build',
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
$compilerCommand = Get-Command g++.exe -ErrorAction SilentlyContinue
$compilerPath = if ($compilerCommand) { $compilerCommand.Source } else { $null }
if (-not $compilerPath) {
    foreach ($candidate in @('C:\Program Files\CodeBlocks\MinGW\bin\g++.exe', 'C:\msys64\ucrt64\bin\g++.exe', 'C:\msys64\mingw64\bin\g++.exe')) {
        if (Test-Path -LiteralPath $candidate) { $compilerPath = $candidate; break }
    }
}
if (-not $compilerPath) { throw 'g++ not found. Install a C++17 compiler or add it to PATH.' }
$env:PATH = (Split-Path -Parent $compilerPath) + ';' + $env:PATH
New-Item -ItemType Directory -Path 'build' -Force | Out-Null
$flags = @('-std=c++17', '-Wall', '-Wextra', '-Wpedantic', '-finput-charset=UTF-8', '-fexec-charset=UTF-8', '-static')
if ($Configuration -eq 'Debug') { $flags += @('-O0','-g') } else { $flags += '-O2' }
$targets = @(
    @('lab1/main.cpp', 'lab1'),
    @('lab2/main.cpp', 'lab2'),
    @('tests/lab1_tests.cpp', 'lab1_tests'),
    @('tests/lab2_tests.cpp', 'lab2_tests'),
    @('benchmarks/main.cpp', 'benchmark')
)
foreach ($target in $targets) {
    Write-Host "Building $($target[1]) ($Configuration)"
    & $compilerPath @flags $target[0] '-o' "build/$($target[1]).exe"
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($target[1])" }
}
if ($Action -eq 'test') {
    New-Item -ItemType Directory -Path 'results' -Force | Out-Null
    foreach ($name in @('lab1_tests','lab2_tests')) {
        & ".\build\$name.exe" | Tee-Object -FilePath "results/$name.txt"
        if ($LASTEXITCODE -ne 0) { throw "Tests failed: $name" }
    }
} elseif ($Action -eq 'lab1' -or $Action -eq 'lab2') {
    & ".\build\$Action.exe"
    if ($LASTEXITCODE -ne 0) { throw "Program failed: $Action" }
} elseif ($Action -eq 'benchmark') {
    if ($Configuration -ne 'Release') { throw 'Use Release configuration for benchmarks.' }
    & '.\build\benchmark.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Benchmark failed.' }
    $bundledPython = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
    if (Test-Path -LiteralPath $bundledPython) {
        & $bundledPython 'scripts/plot.py'
        if ($LASTEXITCODE -ne 0) { throw 'Plot generation failed; CSV results are available.' }
    } else {
        Write-Host 'CSV files saved in results. Run python scripts/plot.py to generate graphs (requires matplotlib).'
    }
}
