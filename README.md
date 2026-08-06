# Select Similar for 3ds Max 2027

Selects sub-objects (vertices, edges, faces) that match the attributes of your current selection — all in one hotkey. Works in **Edit Poly** modifier and **Editable Poly** sub-object modes.

[Download the latest release](https://github.com/WatWall/SelectSimilar/releases)

## Features
- **16 criteria** across all three sub-object levels:
  - **Faces:** Material, Flat/Smooth, Polygon Sides, Area, Perimeter, Normal, Coplanar
  - **Edges:** Seam, Length, Direction, Faces Around Edge, Face Angles, Crease
  - **Vertices:** Normal, Adjacent Faces, Connecting Edges
- Adjustable thresholds (area %, angle degrees, crease 0–1) with a live viewport preview dialog
- Compare modes: Equal / Greater / Less

## Install
1. Download `SelectSimilar2027.dlu`
2. Copy it to `C:\Program Files\Autodesk\3ds Max 2027\Plugins\`
3. Restart 3ds Max (the plugin loads on startup)

> Requires admin rights to write into the Plugins folder.

## Usage
1. Select an object and enter a sub-object mode (Vertex / Edge / Polygon)
2. Select one or more sub-objects
3. Press **Shift+G** to select similar
   - `Ctrl+Shift+G` — add to current selection
   - `Alt+Shift+G` — subtract from current selection

The hotkey is rebindable in **Customize UI → Keyboard → "Select Similar"**.

## Uninstall
Delete `SelectSimilar2027.dlu` from the Plugins folder and restart 3ds Max.

## Requirements
- 3ds Max **2027** (x64) only
- Other versions (2023/2024/2025) need their own build — this DLL is compiled against the 2027 SDK

## Building from source
- Requires the [3ds Max 2027 SDK](https://aps.autodesk.com/developer/overview/3ds-max), Visual Studio 2022 (C++ workload), and CMake >= 3.29
- Run `scripts\build.bat` from the repo root → outputs `build\output\Release\SelectSimilar2027.dlu`
- Deploy with `scripts\deploy.bat` (run as administrator) or see `scripts\README.md`
