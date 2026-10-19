. "$PSScriptRoot\vars.ps1"

$rocmPath = "C:\Program Files\AMD\ROCm\7.2"
$rocmBin = "C:\Program Files\AMD\ROCm\7.2\bin"

if ($env:PATH -notlike "*$rocmBin*") {
    $env:PATH = "$rocmBin;$env:PATH"
}

& "$rocmBin\clang++.exe" -x hip --offload-arch=gfx1151 `
  -I"$rocmPath\include" `
  -L"$rocmPath\lib" `
  -lamdhip64 `
  test_hip.cpp -o test_hip.exe

Copy-Item "$rocmBin\amdhip64_7.dll" -Destination "$PSScriptRoot\amdhip64_7.dll" -Force