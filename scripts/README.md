# Select Similar - Build & Deploy Scripts

Three scripts live here. Pick whichever you prefer; they all do the same job.

| Script | When to use |
|---|---|
| `build.bat` | Plain cmd.exe. Run from a VS "x64 Native Tools" prompt (or it'll auto-find cmake/cl). |
| `build.ps1` | PowerShell. Same as `build.bat` but with red error stopping. |
| `deploy.bat` | Copies the freshly built `.dlu` into the 3ds Max `Plugins\` folder. **Requires admin.** |

## Prerequisites

- **3ds Max 2027 SDK** at `C:\Program Files\Autodesk\3ds Max 2027 SDK\maxsdk` (or set `MAXSDK_PATH`)
  - Must contain `include\maxapi.h` and `lib\x64\Release\*.lib`
  - Download from <https://aps.autodesk.com/developer/overview/3ds-max>
- **Visual Studio 2022** with the "Desktop development with C++" workload (for the MSVC + CMake toolchain)
- **CMake >= 3.29**

## Build

From the repo root (one level up from this folder):

```batch
scripts\build.bat
```
or
```powershell
scripts\build.ps1
```

The script will:
1. Verify the Max SDK is present (errors out with a download link if not)
2. Run `cmake -G "Visual Studio 17 2022" -A x64 -B build -S .` (first time only — reuses `build\` afterwards)
3. Run `cmake --build build --config Release`

**Output:** `build\output\Release\SelectSimilar.dlu` (typically ~60 KB)

### Overriding the SDK path

If the SDK is installed somewhere non-default, set the env var before running:

```batch
set MAXSDK_PATH=D:\maxsdk
scripts\build.bat
```
```powershell
$env:MAXSDK_PATH = "D:\maxsdk"; scripts\build.ps1
```

## Deploy

**Required: run as administrator.** (The destination is under `C:\Program Files\`.)

Either:
1. Right-click `deploy.bat` in Explorer → **Run as administrator**, or
2. Open an elevated cmd and run `scripts\deploy.bat`

The script:
- Looks for `build\output\Release\SelectSimilar.dlu` (falls back to `build\output\SelectSimilar.dlu` for older layouts)
- Refuses to run without admin (`net session` check)
- Copies the file to `C:\Program Files\Autodesk\3ds Max 2027\Plugins\`

After deploy, **fully restart 3ds Max** (the plugin won't hot-reload).

### CMake alternative

There is also a CMake `deploy` target that does the same copy:

```batch
cmake --build build --config Release --target deploy
```
This still needs to be run from an elevated shell for the same reason.

## Default hotkey

After deploy, **Shift+G** is bound by default and shows up in
Customize User Interface → Keyboard → category "Select Similar". The binding
is user-rebindable and persists across restarts via the user's `.kbx` file.

## Uninstall

To remove the plugin:
1. Delete `C:\Program Files\Autodesk\3ds Max 2027\Plugins\SelectSimilar.dlu`
2. Restart 3ds Max

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `3ds Max SDK not found at '...'` | SDK isn't installed at the expected path. Set `MAXSDK_PATH`. |
| `error MSB... vcproj not found` | CMake configure failed. Delete the `build\` folder and re-run `build.bat`. |
| `Access denied` from `deploy.bat` | You didn't run it as admin. Right-click → Run as administrator. |
| Shift+G doesn't fire after deploy | 3ds Max wasn't fully restarted. Close every Max window (check Task Manager) and reopen. |
| Shift+G fires but nothing happens | You're not in an Edit Poly / Editable Poly sub-object mode. Enter sub-object mode first. |

## See also

- `AGENTS.md` (repo root) - architecture, criteria list, code patterns
- `cmake\FindMAXSDK.cmake` - how the build locates the SDK
