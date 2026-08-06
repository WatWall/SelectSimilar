#pragma once

#include <max.h>
#include "SimilarCriteria.h"
#include "SimilarParams.h"
#include "SelectSimilarEngine.h"

class ISpinnerControl;

// Modeless live-preview adjuster.  At most one instance can be open at a time.
// Lifetime is owned by SelectSimilarAdjuster itself (singleton pattern); Max
// owns the HWND via RegisterDlgWnd/UnRegisterDlgWnd so the dialog integrates
// with Max's window management, accelerators, and skinning.
class SelectSimilarAdjuster
{
public:
    // Open the dialog at the cursor for the given context + criterion.
    // The criterion enum to use depends on ctx.level:
    //   - If level == Face, the FaceCriterion is read.
    //   - If level == Edge, the EdgeCriterion is read.
    //   - If level == Vertex, the VertexCriterion is read.
    // Returns immediately (modeless).  No-op if a dialog is already open.
    static void Open(SelectSimilarContext ctx,
                     FaceCriterion   fc,
                     EdgeCriterion   ec,
                     VertexCriterion vc);

    // True if a dialog instance is currently open.
    static bool IsOpen();

    // Force-close (treated as Cancel).  Used if the user changes sub-object
    // level or exits Edit Poly mode while a dialog is up.
    static void Close();

private:
    SelectSimilarAdjuster();
    ~SelectSimilarAdjuster();

    // Internal helpers
    void DoOpen();
    void DoClose(bool apply);
    INT_PTR HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void RecomputeAndPreview();
    void UpdateCountLabel(int count, int total);
    void PopulateControls();
    void ReadControls();
    void ShowHideThreshold();

    // Static DlgProc trampoline (registered via CreateDialogParam).
    static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    SelectSimilarContext mCtx;
    SelLevel             mLevel = SelLevel::None;
    FaceCriterion        mFaceCrit{};
    EdgeCriterion        mEdgeCrit{};
    VertexCriterion      mVertCrit{};

    BitArray             mOriginalSel;   // snapshot at dialog open
    SimilarParams        mParams;        // current compare + threshold

    HWND                 mHwnd = nullptr;
    ISpinnerControl*     mThresholdSpin = nullptr;

    static SelectSimilarAdjuster* sInstance;
};
