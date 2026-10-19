# rocm-windows

Minimal working example and documentation for building and running AMD ROCm / HIP C++ programs on Windows (ROCm 7.2) targeting the **AMD Radeon 8060S (`gfx1151` / Strix Halo)** GPU.

## Contents

- [`test_hip.cpp`](./test_hip.cpp) — Minimal HIP vector addition kernel (`vectorAdd<<<...>>>`) validating 1,000,000 elements on `gfx1151`, with proper host/device guards (`__HIP_DEVICE_COMPILE__`) and Windows UTF-8 console output setup.
- [`build_hip.ps1`](./build_hip.ps1) — PowerShell build script that loads MSVC environment variables, compiles `test_hip.cpp` using ROCm's `clang++.exe`, and copies the SDK's `amdhip64_7.dll` next to the executable.
- [`vars.ps1`](./vars.ps1) — Helper script to import Visual Studio 2022 Build Tools (`vcvars64.bat`) environment variables into the current PowerShell session.
- [`howto.md`](./howto.md) — Step-by-step guide covering prerequisites, compilation flags, and troubleshooting common Windows ROCm pitfalls.
- [`dll_best_practice.md`](./dll_best_practice.md) — Best practices for preventing Windows DLL search order issues (`0xC0000135` / `STATUS_DLL_NOT_FOUND`) caused by stale `C:\Windows\System32\amdhip64_7.dll` binaries.
- [`.clangd`](./.clangd) & [`compile_flags.txt`](./compile_flags.txt) — Language server (`clangd`) configuration for HIP kernel and host code navigation in VS Code / editors without false-positive diagnostics.

## Quick Start

### Prerequisites

1. **AMD ROCm 7.2 HIP SDK** installed at `C:\Program Files\AMD\ROCm\7.2`
2. **Visual Studio 2022 Build Tools** with the **Desktop development with C++** workload (`MSVC v143` + Windows SDK)

### Build & Run

```powershell
# Unblock scripts if needed
Unblock-File .\*.ps1

# Compile test_hip.cpp and stage amdhip64_7.dll
.\build_hip.ps1

# Run the executable
.\test_hip.exe
```

Expected output:

```text
Calcul HIP sur 8060S (gfx1151) : SUCCÈS (1M éléments validés)
```

## Documentation

- See [**How to Build and Run HIP Programs on Windows (`howto.md`)**](./howto.md) for detailed instructions and troubleshooting.
- See [**ROCm DLL Best Practice on Windows (`dll_best_practice.md`)**](./dll_best_practice.md) for details on why `amdhip64_7.dll` should be copied alongside the executable.
