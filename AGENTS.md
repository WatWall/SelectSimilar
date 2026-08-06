# Select Similar - Agent Instructions

## Build

**Required SDK**
- 3ds Max SDK: Checks `MAXSDK_PATH` env var, then `C:/Program Files/Autodesk/3ds Max 2027 SDK/maxsdk`, then `C:/Program Files/Autodesk/3ds Max 2025 SDK/maxsdk`

**Build commands**
```batch
REM From repository root
scripts\build.bat
```
Or:
```powershell
scripts\build.ps1
```

**Output**
- `build\output\Release\SelectSimilar<MaxYear>.dlu` (e.g. `SelectSimilar2027.dlu`), versioned per the SDK the build targets

## Deploy to 3ds Max

**Requires admin privileges** - Script copies to `C:\Program Files\Autodesk\3ds Max 2027\Plugins\`

```batch
scripts\deploy.bat
```
Or CMake target:
```batch
cmake --build build --config Release --target deploy
```

## Architecture

**Plugin Components** (1 Max plugin class)
- `SelectSimGUP` - Global Utility Plugin. On `Start()`, force-constructs the ActionTable so the hotkey is active for the whole Max session.

**Hotkey: Shift+G** (default binding, user-rebindable in Customize UI -> Keyboard -> "Select Similar")

**Directory Structure**
- `src/core/` - DllEntry, PluginDescriptor, SelectSimGUP, PluginDef, resource files
- `src/selectsim/` - SelectSimilarAction (ActionTable+Callback), SelectSimilarEngine (context detection + criteria), SelectSimilarMenu (popup), SimilarCriteria (enums)

## How it works

1. User presses Shift+G (or any rebound key) while in Edit Poly sub-object mode
2. `ActionCallback::ExecuteAction` fires -> calls `DoSelectSimilar()`
3. `SelectSimilarEngine::DetectContext()` walks the modify panel:
   - Gets `ip->GetCurEditObject()` - the currently-edited BaseObject
   - Verifies `ip->GetSubObjectLevel() > 0`
   - If `ClassID == EDIT_POLY_MODIFIER_CLASS_ID` and `SuperClassID == OSM_CLASS_ID`: it's the Edit Poly modifier
     - Gets `EPolyMod*` via `GetEPolyModInterface(mod)`
     - Reads nodes via `ip->GetModContexts(...)`
     - Reads current selection via `epMod->EpModGetSelection(MNM_SL_CURRENT, node)`
     - Reads mesh via `epMod->EpModGetMesh(node)`
   - Else if `ClassID == EPOLYOBJ_CLASS_ID`: it's the Editable Poly base object
     - Gets `EPoly*` via `GetEPolyInterface(obj)`
     - Reads selection by `MN_SEL` flag via `EpGet{Vertices,Edges,Faces}ByFlag`
4. Popup menu shows criteria valid for the current sub-object level
5. Compute matching BitArray (see criteria functions)
6. `ApplySelection()` writes back through `EpModSetSelection` (modifier) or `EpfnSetSelection` (base object), wrapped in `theHold.Begin/Accept` for undoability, then refreshes viewports.

## Class ID scheme

"SSI" = ASCII 'S','S','I' = `0x535349`
- GUP: `Class_ID(0x53534900, 0x47555000)`
- ActionTable ID: `0x53534901` (`SSI` + 0x01)

## PoC Criteria

Phase 1 covered 4. Phase 2 added the rest, bringing the total to **16 criteria**
across three sub-object levels:

### Face (7)
| Criterion | Behavior | Threshold | Source |
|---|---|---|---|
| Material | Direct-apply | — | `MNFace::material` |
| Flat/Smooth | Direct-apply | — | `MNFace::smGroup` (overlap) |
| Polygon Sides | Adjuster, compare-only | — | `MNFace::deg` |
| Area | Adjuster | % | `Length(GetFaceNormal(ff,FALSE))/2` or triangulate-sum |
| Perimeter | Adjuster | % | sum of corner distances |
| Normal | Adjuster | degrees | `MNMesh::GetFaceNormal` angle |
| Coplanar | Adjuster | degrees | same as Normal |

### Edge (6) — Sharpness skipped (identical to Crease)
| Criterion | Behavior | Threshold | Source |
|---|---|---|---|
| Seam | Direct-apply | — | `MN_EDGE_MAP_SEAM` flag |
| Length | Adjuster | % | vertex distance |
| Direction | Adjuster | degrees | edge vector angle (bidirectional) |
| Faces Around Edge | Adjuster, compare-only | — | `(MNEdge::f2>=0)?2:1` |
| Face Angles | Adjuster | degrees | `MNMesh::EdgeAngle` (dihedral) |
| Crease | Adjuster | absolute 0–1 | `edgeFloat(EDATA_CREASE)` |

### Vertex (3) — Vertex Groups skipped (no Max equivalent)
| Criterion | Behavior | Threshold | Source |
|---|---|---|---|
| Normal | Adjuster | degrees | `MNMesh::GetVertexNormal` |
| Adjacent Faces | Adjuster, compare-only | — | `(*GetVFac())[v].Count()` |
| Connecting Edges | Adjuster, compare-only | — | `(*GetVEdge())[v].Count()` |

## Adjuster dialog (Phase 2)

A modeless floating dialog appears for any criterion that supports Compare
(everything except the 3 direct-apply criteria).  It offers:
- **Compare**: Equal / Greater / Less radio buttons
- **Threshold**: float spinner (hidden for compare-only criteria like Polygon Sides)
- **Live count**: "N of M items selected"
- **Apply** (Enter) commits with one undo entry
- **Cancel** (Esc) reverts to pre-dialog state

The viewport updates live as the user drags the spinner.  Default thresholds per
kind: 5% for ratio criteria, 5° for angles, 0.05 for absolute.

## Out of scope (Phase 3+)

- Editable Mesh / Edit Mesh Modifier support
- Border & Element sub-object levels (currently collapse to Edge/Face)
- Vertex Groups (no Max equivalent) and Sharpness (same as Crease)
- Persistent threshold settings across Max restarts (currently in-session only)
- Configurable tolerances UI beyond the single threshold spinner
- More precise Coplanar (currently treats as parallel-plane match like Normal)

## Verification

**No unit/integration tests** - Manual testing in 3ds Max only.

For each of the 16 criteria:
1. Build/deploy plugin
2. Restart 3ds Max
3. Create a Box, apply Edit Poly modifier, go to sub-object level matching criterion
4. Select 1+ subobjects -> Shift+G -> pick criterion
5. For direct-apply criteria: selection updates immediately, single Ctrl+Z reverts
6. For adjuster criteria: dialog appears at cursor; spin Threshold, change Compare,
   verify viewport updates live; Apply commits, Cancel reverts

## Git Workflow

- Trunk-based development (commit directly, no PRs)
