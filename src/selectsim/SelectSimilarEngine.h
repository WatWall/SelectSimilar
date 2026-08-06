#pragma once

#include <max.h>
#include "SimilarCriteria.h"

class EPolyMod;
class EPoly;

// Resolved context for a Select-Similar invocation.
struct SelectSimilarContext
{
    bool         valid       = false;
    bool         isModifier  = false;
    SelLevel     level       = SelLevel::None;
    Modifier*    modifier    = nullptr;
    BaseObject*  baseObj     = nullptr;
    INode*       node        = nullptr;
    EPolyMod*    epMod       = nullptr;
    EPoly*       epoly       = nullptr;
    MNMesh*      mesh        = nullptr;
    BitArray     currentSelection;
};

namespace SelectSimilarEngine
{
    // Walk the modify panel to determine what's being edited and what's selected.
    // Returns false if we are not inside an Edit Poly sub-object mode.
    bool DetectContext(SelectSimilarContext& ctx);

    // --- Low-level selection writers (used by both the direct path and the adjuster) ---

    // Writes the BitArray to the modifier/object's selection WITHOUT any undo
    // accounting.  Used for live-preview updates while dragging a spinner.
    void WriteLive(const SelectSimilarContext& ctx, const BitArray& sel);

    // Wraps the change in a single undo entry.  The originalSel is restored first
    // (to give theHold a known restore point), then finalSel is applied, then
    // the result is committed as one named undo step.
    void WriteFinal(const SelectSimilarContext& ctx,
                    const BitArray& originalSel,
                    const BitArray& finalSel,
                    const wchar_t* undoName);

    // Reverts visible selection to the snapshot.  No undo entry is created.
    void Revert(const SelectSimilarContext& ctx, const BitArray& originalSel);

    // Combines a freshly-computed similar-set with the snapshot, honouring the
    // selection mode (Replace / Add / Subtract) chosen via modifier keys.
    BitArray CombineWithOriginal(const BitArray& originalSel,
                                 const BitArray& similarSel,
                                 SelectionMode mode);
}
