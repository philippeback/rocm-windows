# How to Build and Run HIP Programs on Windows (ROCm 7.2 / gfx1151)

This guide explains how to compile and run `test_hip.cpp` on Windows using the AMD ROCm 7.2 HIP SDK targeting the **AMD Radeon 8060S (`gfx1151`)** GPU.

---

## 1. Prerequisites

### A. AMD ROCm 7.2 HIP SDK

Ensure the AMD HIP SDK 7.2 is installed in the default location:

- **Root directory:** `C:\Program Files\AMD\ROCm\7.2`
- **Binaries (`clang++.exe`, `hipInfo.exe`, `amdhip64_7.dll`):** `C:\Program Files\AMD\ROCm\7.2\bin`
- **Headers:** `C:\Program Files\AMD\ROCm\7.2\include`
- **Libraries:** `C:\Program Files\AMD\ROCm\7.2\lib`

You can verify that ROCm detects your GPU by running:

```powershell
& "C:\Program Files\AMD\ROCm\7.2\bin\hipInfo.exe"
& "C:\Program Files\AMD\ROCm\7.2\bin\amdgpu-arch.exe"
```

### B. Microsoft Visual C++ (MSVC) Build Tools & Windows SDK

On Windows, ROCm's `clang++.exe` targets the MSVC ABI (`x86_64-pc-windows-msvc`). It relies on the **MSVC C++ Standard Library headers** (`<iostream>`, `<cmath>`, `<cstdlib>`, etc.) and the **Windows SDK** libraries to compile and link host code.

If you see the following error when compiling:

```text
clang++: warning: unable to find a Visual Studio installation; try running Clang from a developer command prompt [-Wmsvc-not-found]
error: "Could not find standard C++ header 'cmath'..."
fatal error: 'cstdlib' file not found
```

you need to install the **Desktop development with C++** workload:

#### Option 1: Install via `winget` (PowerShell as Administrator)

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

#### Option 2: Install via Visual Studio Installer

1. Open **Visual Studio Installer** (`C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe`).
2. Install **Visual Studio Build Tools 2022** (or Visual Studio Community 2022).
3. Check the **Desktop development with C++** workload (ensure *MSVC v143 - VS 2022 C++ x64/x86 build tools* and *Windows 11 SDK* are selected) and click **Install**.

---

## 2. Build Script (`build_hip.ps1`)

`build_hip.ps1` loads the MSVC environment via `vars.ps1`, compiles `test_hip.cpp` with ROCm's `clang++.exe`, and copies `amdhip64_7.dll` alongside `test_hip.exe`:

```powershell
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
```

### Why `amdhip64_7.dll` Is Copied Next to `test_hip.exe`

Windows searches `C:\Windows\System32` **before** directories in `PATH`. If an AMD graphics driver installs a different version of `C:\Windows\System32\amdhip64_7.dll` (which depends on `rocm_kpack.dll`), `test_hip.exe` will fail silently at startup with `0xC0000135` (`STATUS_DLL_NOT_FOUND`). Placing ROCm 7.2's `amdhip64_7.dll` in the same directory as `test_hip.exe` ensures Windows loads the SDK's self-contained runtime DLL first.

### Key Compiler Flags

- `-x hip`: Treats the input `.cpp` file as a HIP source file containing both host and device (`__global__`) code.
- `--offload-arch=gfx1151`: Compiles GPU device kernels for the RDNA 3.5 / Strix Halo Radeon 8060S architecture (`gfx1151`).
- `-I"C:\Program Files\AMD\ROCm\7.2\include"`: Adds the ROCm HIP headers (`<hip/hip_runtime.h>`).
- `-L"C:\Program Files\AMD\ROCm\7.2\lib"` & `-lamdhip64`: Links against the AMD HIP runtime import library (`amdhip64.lib`).

---

## 3. Building and Running

1. Open PowerShell in the project folder (`C:\Dev\github\philippeback\hip`):

   ```powershell
   .\build_hip.ps1
   ```

2. Run the compiled executable:

   ```powershell
   .\test_hip.exe
   ```

3. Expected output:

   ```text
   Calcul HIP sur 8060S (gfx1151) : SUCCÈS (1M éléments validés)
   ```

---

## 4. Troubleshooting: `test_hip.exe` Exits Silently with No Output (`0xC0000135`)

### Symptom

Running `.\test_hip.exe` in PowerShell prints nothing at all, and checking the exit code shows `-1073741515` (`0xC0000135` / `STATUS_DLL_NOT_FOUND`):

```powershell
.\test_hip.exe
Write-Host "Exit code: $LASTEXITCODE (0x$($LASTEXITCODE.ToString('X8')))"
# Output: Exit code: -1073741515 (0xC0000135)
```

Even adding `C:\Program Files\AMD\ROCm\7.2\bin` to `$env:PATH` does **not** resolve this error.

### Root Cause: Windows DLL Search Order & `System32\amdhip64_7.dll`

On Windows, the loader searches for imported DLLs (`amdhip64_7.dll`) in the following order:

1. **The directory from which the application loaded** (e.g., `C:\Dev\github\philippeback\hip\`)
2. **The system directory (`C:\Windows\System32`)**
3. The 16-bit system directory (`C:\Windows\System`)
4. The Windows directory (`C:\Windows`)
5. The current working directory
6. **The directories listed in the `PATH` environment variable** (`C:\Program Files\AMD\ROCm\7.2\bin`)

Because `C:\Windows\System32` is searched **before** `PATH`:

- Windows finds `C:\Windows\System32\amdhip64_7.dll` (installed by the AMD display driver) before it ever reaches `C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll`.
- The driver's `C:\Windows\System32\amdhip64_7.dll` has a transitive dependency on `rocm_kpack.dll`, which is missing from `System32` and `PATH`, causing the Windows loader to abort startup with `STATUS_DLL_NOT_FOUND` (`0xC0000135`).
- By contrast, the ROCm 7.2 SDK's `C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll` only depends on standard Windows system DLLs (`SETUPAPI.dll`, `KERNEL32.dll`, `ADVAPI32.dll`, `USER32.dll`, `GDI32.dll`, `WS2_32.dll`).
- Tools like `hipInfo.exe` work because they live inside `C:\Program Files\AMD\ROCm\7.2\bin` (matching step 1 of the search order).

### How to Inspect the DLL Dependencies

You can verify the difference between the two DLLs using `llvm-objdump.exe`:

```powershell
# Inspect C:\Windows\System32\amdhip64_7.dll (requires rocm_kpack.dll)
& "C:\Program Files\AMD\ROCm\7.2\bin\llvm-objdump.exe" -p "C:\Windows\System32\amdhip64_7.dll" | Select-String "DLL Name"

# Inspect ROCm 7.2 SDK's amdhip64_7.dll (no rocm_kpack.dll dependency)
& "C:\Program Files\AMD\ROCm\7.2\bin\llvm-objdump.exe" -p "C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll" | Select-String "DLL Name"
```

### Fix

Copy the ROCm 7.2 SDK's `amdhip64_7.dll` into the same directory as `test_hip.exe` so Windows loads it at step 1 (application directory) before checking `C:\Windows\System32`:

```powershell
Copy-Item "C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll" -Destination ".\amdhip64_7.dll" -Force
```

This step is automatically included at the end of `build_hip.ps1`.

---

## 5. Troubleshooting: Garbled Accented Characters (`SUCC├êS (1M ├¬l├¬ments valid├¬s)`)

### Symptom

Because `test_hip.cpp` is saved in UTF-8, printing accented French characters (`È`, `é`) to the Windows console using `std::cout` produces mojibake (`SUCC├êS (1M ├¬l├¬ments valid├¬s)`) when the console's active output code page is OEM 850 or 437 instead of UTF-8 (`65001`).

### Fix

`test_hip.cpp` sets the Windows console output code page to UTF-8 (`65001`) at the start of `main()`:

```cpp
#if defined(_WIN32) && !defined(__HIP_DEVICE_COMPILE__)
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
#endif

int main() {
#if defined(_WIN32) && !defined(__HIP_DEVICE_COMPILE__)
    SetConsoleOutputCP(65001); // CP_UTF8
#endif
    // ...
```

---

## 6. Troubleshooting: `warning: __declspec attribute 'dllimport' is not supported [-Wignored-attributes]`

### Symptom

When declaring a Windows API function with `__declspec(dllimport)` guarded only by `#ifdef _WIN32`, `clang++ -x hip` emits a warning during the GPU device compilation pass:

```text
test_hip.cpp:22:23: warning: __declspec attribute 'dllimport' is not supported [-Wignored-attributes]
   22 | extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
      |                       ^~~~~~~~~
1 warning generated when compiling for gfx1151.
```

### Root Cause: HIP Two-Pass Compilation (`__HIP_DEVICE_COMPILE__`)

When compiling with `-x hip --offload-arch=gfx1151`, Clang parses `test_hip.cpp` **twice**:

1. **Host pass (`x86_64-pc-windows-msvc`):** Compiles CPU code (`main()`, host API calls). `_WIN32` is defined and `__declspec(dllimport)` is valid.
2. **Device pass (`amdgcn-amd-amdhsa` / `gfx1151`):** Compiles GPU kernels (`__global__`, `__device__`). In this pass, Clang still inherits host preprocessor macros like `_WIN32`, **and** defines `__HIP_DEVICE_COMPILE__`. Because the AMDGPU target does not use Windows PE DLL imports, Clang warns that `__declspec(dllimport)` is unsupported when compiling for `gfx1151`.

### Fix

Guard Windows-specific declarations and calls with `#if defined(_WIN32) && !defined(__HIP_DEVICE_COMPILE__)` so they are only compiled during the x64 host pass and ignored during the `gfx1151` device pass.

---

## 7. Troubleshooting: PowerShell Prompts `[D] Do not run  [R] Run once`

### Symptom

When running `.\build_hip.ps1`, PowerShell pauses and asks for confirmation before executing `build_hip.ps1` or `vars.ps1`:

```text
Security warning
Run only scripts that you trust. While scripts from the internet can be useful, this script can potentially harm your computer.
Do you want to run C:\Dev\github\philippeback\hip\build_hip.ps1?
[D] Do not run  [R] Run once  [S] Suspend  [?] Help (default is "D"):
```

### Root Cause

Files downloaded or created by external tools on Windows can carry a `Zone.Identifier` Alternate Data Stream (Mark-of-the-Web). Under the `RemoteSigned` execution policy, PowerShell requires interactive confirmation before running any script tagged with `Zone.Identifier`.

### Fix

Remove the `Zone.Identifier` stream from the `.ps1` scripts in the directory using `Unblock-File`:

```powershell
Unblock-File .\*.ps1
```
