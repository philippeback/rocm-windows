# ROCm DLL Best Practice on Windows

This document explains how to avoid loading the wrong AMD HIP/ROCm DLL on Windows, especially when a stale copy exists in `C:\Windows\System32`.

## Problem

Windows can resolve a system copy of `amdhip64_7.dll` before it ever checks the ROCm SDK installation in:

- `C:\Program Files\AMD\ROCm\7.2\bin`

When that happens, the application may load a broken or incomplete runtime that depends on missing ROCm DLLs such as `rocm_kpack.dll` or other runtime dependencies. The result is a failure like:

- `STATUS_DLL_NOT_FOUND`
- `0xC0000135`
- "The application was unable to start correctly"

## Root cause

The main issue is DLL search order on Windows. The EXE directory and PATH are checked before or alongside the system directories, but a stale system copy can still be selected if the correct ROCm DLL is not forced to be used.

For ROCm/HIP software, this is especially dangerous because the wrong runtime may look valid but still fail due to missing ROCm dependencies.

## Best fix: force the correct ROCm DLL to be used

The most reliable fix is to copy the ROCm runtime DLL next to the executable that is going to load it:

```powershell
Copy-Item "C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll" ".\amdhip64_7.dll" -Force
```

Then run the program from that directory:

```powershell
.\test_hip.exe
```

This ensures Windows picks the local ROCm DLL before the stale `C:\Windows\System32` version.

## Use PATH as a second layer

Also prepend the ROCm bin directory to PATH for the current PowerShell session:

```powershell
$env:PATH = "C:\Program Files\AMD\ROCm\7.2\bin;$env:PATH"
```

This helps when the app or child processes need the ROCm runtime from the installed SDK.

## For llama.cpp and other ROCm-based servers

The same rule applies. Any server that loads HIP/ROCm should be started with the ROCm bin directory first in PATH, and ideally with the required DLLs copied alongside the EXE or server binary.

Example:

```powershell
$env:PATH = "C:\Program Files\AMD\ROCm\7.2\bin;$env:PATH"
Copy-Item "C:\Program Files\AMD\ROCm\7.2\bin\amdhip64_7.dll" ".\amdhip64_7.dll" -Force
```

Then launch your server process.

## Recommended workflow

1. Verify the installed ROCm SDK is present at `C:\Program Files\AMD\ROCm\7.2`.
2. Ensure the ROCm bin directory is first in PATH.
3. Copy the required AMD HIP runtime DLL next to the EXE.
4. Launch the process from a fresh shell.
5. If needed, keep the DLL copies local to the app directory to prevent system DLL shadowing.

## Summary

For Windows ROCm apps, the safe default is:

- prefer `C:\Program Files\AMD\ROCm\7.2\bin`
- copy `amdhip64_7.dll` next to the executable
- avoid relying on `C:\Windows\System32`

This is the most reliable way to prevent the wrong ROCm runtime from being loaded.
