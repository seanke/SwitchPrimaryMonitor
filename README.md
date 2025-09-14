# SwitchPrimaryMonitor

Small Windows 11/10 console utility that switches the primary monitor to the next attached monitor while preserving the relative layout of displays.

## Build (CMake + MSVC)

Prerequisites:
- Visual Studio 2019/2022 or Build Tools (C++ Desktop)
- CMake 3.16+

Steps:

```powershell
# From the repository root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable will be at:
`build/Release/SwitchPrimaryMonitor.exe` (multi-config) or `build/SwitchPrimaryMonitor.exe` (single-config generators).

## CI / Releases

- GitHub Actions builds on pushes and PRs to `main`.
- Creating a tag like `v1.0.0` triggers a release with a ZIP containing the EXE and README.

Badge: `Actions -> build` in this repo shows status.

## Usage

Run from an elevated or normal command prompt (no elevation required):

```powershell
./SwitchPrimaryMonitor.exe
```

Behavior:
- Identifies the current primary display.
- Selects the next attached display (in enumeration order) as the new primary.
- Re-bases the virtual desktop so the new primary is at (0,0) and preserves the relative arrangement of other displays.

Exit codes:
- `0` success
- `1` no attached displays
- `2` could not identify current primary
- `3` failed to set new primary
- `4` failed to reposition a display
- `5` failed to apply changes

## Notes

- Uses `EnumDisplayDevicesW` and `ChangeDisplaySettingsExW` with `CDS_SET_PRIMARY` and `DM_POSITION`.
- Skips mirroring drivers and only considers displays attached to the desktop.
- If only one monitor is attached, it exits without changes.
