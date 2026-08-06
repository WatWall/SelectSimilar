#include "SelectSimilarAdjuster.h"
#include "SelectSimilarComputations.h"
#include "../core/adjuster_ids.h"

#include <maxapi.h>
#include <custcont.h>
#include <3dsmaxdlport.h>  // DLSetWindowLongPtr / DLGetWindowLongPtr
#include <windows.h>

#include <cstdio>

SelectSimilarAdjuster* SelectSimilarAdjuster::sInstance = nullptr;

// ------------------------------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------------------------------
void SelectSimilarAdjuster::Open(SelectSimilarContext ctx,
                                 FaceCriterion fc, EdgeCriterion ec, VertexCriterion vc)
{
    if (sInstance) return;   // already open

    sInstance = new SelectSimilarAdjuster();
    sInstance->mCtx      = ctx;
    sInstance->mLevel    = ctx.level;
    sInstance->mFaceCrit = fc;
    sInstance->mEdgeCrit = ec;
    sInstance->mVertCrit = vc;

    sInstance->mOriginalSel = ctx.currentSelection;

    // Initial params: load default threshold + last-used compare for this criterion.
    switch (ctx.level)
    {
    case SelLevel::Face:
        sInstance->mParams.threshold = DefaultThreshold(fc);
        break;
    case SelLevel::Edge:
        sInstance->mParams.threshold = DefaultThreshold(ec);
        break;
    case SelLevel::Vertex:
        sInstance->mParams.threshold = DefaultThreshold(vc);
        break;
    default: return;
    }
    sInstance->mParams.compare = CompareMode::Equal;
    sInstance->mParams.selMode = GetSelectionModeFromModifiers();

    sInstance->DoOpen();
}

bool SelectSimilarAdjuster::IsOpen()
{
    return sInstance != nullptr;
}

void SelectSimilarAdjuster::Close()
{
    if (sInstance && sInstance->mHwnd)
    {
        sInstance->DoClose(/*apply=*/false);
    }
}

SelectSimilarAdjuster::SelectSimilarAdjuster() = default;
SelectSimilarAdjuster::~SelectSimilarAdjuster() = default;

// ------------------------------------------------------------------------------------------------
// Open / close
// ------------------------------------------------------------------------------------------------
void SelectSimilarAdjuster::DoOpen()
{
    Interface* ip = GetCOREInterface();
    HWND hParent = ip ? static_cast<HWND>(ip->GetMAXHWnd()) : nullptr;

    // Center on the cursor (the popup menu was just dismissed at the same spot).
    mHwnd = CreateDialogParamW(
        hInstance,
        MAKEINTRESOURCEW(IDD_ADJUSTER_DIALOG),
        hParent,
        &SelectSimilarAdjuster::DlgProc,
        reinterpret_cast<LPARAM>(this));

    if (!mHwnd)
    {
        delete sInstance;
        sInstance = nullptr;
        return;
    }

    // Position the dialog so the Apply button lands exactly at the cursor —
    // then the user can click Apply with no mouse movement.  We measure the
    // Apply button's actual screen position via GetWindowRect (robust against
    // DPI, title-bar height, and border width) and offset the dialog so the
    // button's center coincides with the cursor.
    HWND   hApply = GetDlgItem(mHwnd, IDOK);
    POINT  cur{};
    GetCursorPos(&cur);

    int newX = cur.x;
    int newY = cur.y;
    if (hApply)
    {
        RECT dlgRect{}, applyRect{};
        GetWindowRect(mHwnd,   &dlgRect);
        GetWindowRect(hApply,  &applyRect);
        const int applyCenterOffsetX = ((applyRect.left + applyRect.right) / 2) - dlgRect.left;
        const int applyCenterOffsetY = ((applyRect.top  + applyRect.bottom) / 2) - dlgRect.top;
        newX = cur.x - applyCenterOffsetX;
        newY = cur.y - applyCenterOffsetY;

        // Clamp to the cursor's monitor work area so the whole dialog stays visible.
        const int dlgW = dlgRect.right  - dlgRect.left;
        const int dlgH = dlgRect.bottom - dlgRect.top;
        if (HMONITOR hMon = MonitorFromPoint(cur, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi{ sizeof(mi) };
            if (GetMonitorInfoW(hMon, &mi))
            {
                if (newX < mi.rcWork.left)                                newX = mi.rcWork.left;
                if (newY < mi.rcWork.top)                                 newY = mi.rcWork.top;
                if (newX + dlgW > mi.rcWork.right)                        newX = mi.rcWork.right  - dlgW;
                if (newY + dlgH > mi.rcWork.bottom)                       newY = mi.rcWork.bottom - dlgH;
            }
        }
    }

    SetWindowPos(mHwnd, HWND_TOP, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(mHwnd, SW_SHOWNORMAL);
}

void SelectSimilarAdjuster::DoClose(bool apply)
{
    if (!mHwnd) return;

    // Release the spinner control before destroying the window (the HWND it
    // wraps becomes invalid once DestroyWindow runs).
    if (mThresholdSpin) { ReleaseISpinner(mThresholdSpin); mThresholdSpin = nullptr; }

    if (apply)
    {
        // Compute final selection once more (the last live-preview state may be stale
        // if the user clicked Apply without nudging the spinner).
        BitArray similar;
        switch (mLevel)
        {
        case SelLevel::Face:   SelectSimilarComputations::ComputeFace  (mCtx, mFaceCrit, mParams, similar); break;
        case SelLevel::Edge:   SelectSimilarComputations::ComputeEdge  (mCtx, mEdgeCrit, mParams, similar); break;
        case SelLevel::Vertex: SelectSimilarComputations::ComputeVertex(mCtx, mVertCrit, mParams, similar); break;
        default: break;
        }
        BitArray final = SelectSimilarEngine::CombineWithOriginal(mOriginalSel, similar, mParams.selMode);

        const wchar_t* name = L"Select Similar";
        switch (mLevel)
        {
        case SelLevel::Face:   name = GetDisplayName(mFaceCrit);   break;
        case SelLevel::Edge:   name = GetDisplayName(mEdgeCrit);   break;
        case SelLevel::Vertex: name = GetDisplayName(mVertCrit);   break;
        default: break;
        }
        SelectSimilarEngine::WriteFinal(mCtx, mOriginalSel, final, name);
    }
    else
    {
        // Revert visible state to before dialog opened.
        SelectSimilarEngine::Revert(mCtx, mOriginalSel);
    }

    HWND h = mHwnd;
    mHwnd = nullptr;
    DestroyWindow(h);

    delete sInstance;
    sInstance = nullptr;
}

// ------------------------------------------------------------------------------------------------
// DlgProc
// ------------------------------------------------------------------------------------------------
INT_PTR CALLBACK SelectSimilarAdjuster::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        SelectSimilarAdjuster* self = reinterpret_cast<SelectSimilarAdjuster*>(lParam);
        DLSetWindowLongPtr(hWnd, self);
        Interface* ip = GetCOREInterface();
        if (ip) ip->RegisterDlgWnd(hWnd);
        return self ? self->HandleMessage(hWnd, msg, wParam, lParam) : FALSE;
    }

    SelectSimilarAdjuster* self = DLGetWindowLongPtr<SelectSimilarAdjuster*>(hWnd);
    if (!self) return FALSE;

    if (msg == WM_DESTROY)
    {
        Interface* ip = GetCOREInterface();
        if (ip) ip->UnRegisterDlgWnd(hWnd);
        return FALSE;
    }

    return self->HandleMessage(hWnd, msg, wParam, lParam);
}

INT_PTR SelectSimilarAdjuster::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        // IMPORTANT: mHwnd is still NULL here because CreateDialogParamW hasn't
        // returned yet (WM_INITDIALOG fires during creation). Capture it now so
        // PopulateControls() / ShowHideThreshold() / etc. can use it.
        mHwnd = hWnd;

        // Set dialog title to include the criterion name.
        const wchar_t* critName = L"";
        switch (mLevel)
        {
        case SelLevel::Face:   critName = GetDisplayName(mFaceCrit);   break;
        case SelLevel::Edge:   critName = GetDisplayName(mEdgeCrit);   break;
        case SelLevel::Vertex: critName = GetDisplayName(mVertCrit);   break;
        default: break;
        }
        wchar_t title[128];
        _snwprintf_s(title, _TRUNCATE, L"Select Similar - By %s", critName);
        SetWindowTextW(hWnd, title);

        PopulateControls();
        ShowHideThreshold();
        RecomputeAndPreview();
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_COMPARE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                ReadControls();
                RecomputeAndPreview();
            }
            return TRUE;

        case IDOK:
            DoClose(/*apply=*/true);
            return TRUE;

        case IDCANCEL:
            DoClose(/*apply=*/false);
            return TRUE;
        }
        break;
    }

    case WM_CLOSE:
        DoClose(/*apply=*/false);
        return TRUE;

    case WM_NOTIFY:
        // Spinner change notifications are NOT sent as CC_SPINNER_CHANGE here;
        // they are sent to the parent (this dialog) as CC_SPINNER_CHANGE.
        break;

    default:
        // Custom-control notifications land here.
        if (msg == CC_SPINNER_CHANGE)
        {
            if (LOWORD(wParam) == IDC_THRESH_SPIN)
            {
                ReadControls();
                RecomputeAndPreview();
            }
            return TRUE;
        }
        break;
    }

    return FALSE;
}

// ------------------------------------------------------------------------------------------------
// Control helpers
// ------------------------------------------------------------------------------------------------
void SelectSimilarAdjuster::PopulateControls()
{
    // Populate the Compare combo: index 0=Equal, 1=Greater, 2=Less.
    HWND hCombo = GetDlgItem(mHwnd, IDC_COMPARE_COMBO);
    if (hCombo)
    {
        SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Equal");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Greater");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Less");
        int sel = 0;
        switch (mParams.compare)
        {
        case CompareMode::Greater: sel = 1; break;
        case CompareMode::Less:    sel = 2; break;
        case CompareMode::Equal:
        default:                   sel = 0; break;
        }
        SendMessageW(hCombo, CB_SETCURSEL, sel, 0);
    }

    // Threshold spinner setup (range + units depend on the criterion).
    float lo = 0.0f, hi = 1.0f;
    switch (mLevel)
    {
    case SelLevel::Face:   GetThresholdRange(mFaceCrit, lo, hi); break;
    case SelLevel::Edge:   GetThresholdRange(mEdgeCrit, lo, hi); break;
    case SelLevel::Vertex: GetThresholdRange(mVertCrit, lo, hi); break;
    default: break;
    }

    if (mThresholdSpin) { ReleaseISpinner(mThresholdSpin); mThresholdSpin = nullptr; }

    mThresholdSpin = SetupFloatSpinner(
        mHwnd, IDC_THRESH_SPIN, IDC_THRESH_EDIT,
        lo, hi, mParams.threshold, /*scale=*/(hi - lo) * 0.01f + 0.001f);
    if (mThresholdSpin) mThresholdSpin->SetAutoScale(TRUE);

    // Unit suffix
    const wchar_t* unit = L"";
    switch (mLevel)
    {
    case SelLevel::Face:   unit = GetThresholdUnit(mFaceCrit);   break;
    case SelLevel::Edge:   unit = GetThresholdUnit(mEdgeCrit);   break;
    case SelLevel::Vertex: unit = GetThresholdUnit(mVertCrit);   break;
    default: break;
    }
    SetWindowTextW(GetDlgItem(mHwnd, IDC_THRESH_UNIT), unit);
}

void SelectSimilarAdjuster::ShowHideThreshold()
{
    // Hide threshold controls for compare-only (discrete) criteria.
    bool hasThreshold = false;
    switch (mLevel)
    {
    case SelLevel::Face:   hasThreshold = HasThreshold(mFaceCrit);   break;
    case SelLevel::Edge:   hasThreshold = HasThreshold(mEdgeCrit);   break;
    case SelLevel::Vertex: hasThreshold = HasThreshold(mVertCrit);   break;
    default: break;
    }
    const int showCmd = hasThreshold ? SW_SHOW : SW_HIDE;
    ShowWindow(GetDlgItem(mHwnd, IDC_THRESH_LABEL), showCmd);
    ShowWindow(GetDlgItem(mHwnd, IDC_THRESH_EDIT),  showCmd);
    ShowWindow(GetDlgItem(mHwnd, IDC_THRESH_SPIN),  showCmd);
    ShowWindow(GetDlgItem(mHwnd, IDC_THRESH_UNIT),  showCmd);
}

void SelectSimilarAdjuster::ReadControls()
{
    // Compare combo: index 0=Equal, 1=Greater, 2=Less.
    LRESULT sel = SendDlgItemMessageW(mHwnd, IDC_COMPARE_COMBO, CB_GETCURSEL, 0, 0);
    switch (sel)
    {
    case 1:  mParams.compare = CompareMode::Greater; break;
    case 2:  mParams.compare = CompareMode::Less;    break;
    case 0:
    default: mParams.compare = CompareMode::Equal;   break;
    }

    if (mThresholdSpin) mParams.threshold = mThresholdSpin->GetFVal();
}

void SelectSimilarAdjuster::RecomputeAndPreview()
{
    BitArray similar;
    switch (mLevel)
    {
    case SelLevel::Face:   SelectSimilarComputations::ComputeFace  (mCtx, mFaceCrit, mParams, similar); break;
    case SelLevel::Edge:   SelectSimilarComputations::ComputeEdge  (mCtx, mEdgeCrit, mParams, similar); break;
    case SelLevel::Vertex: SelectSimilarComputations::ComputeVertex(mCtx, mVertCrit, mParams, similar); break;
    default: break;
    }

    int total = 0;
    switch (mLevel)
    {
    case SelLevel::Face:   total = mCtx.mesh ? mCtx.mesh->FNum() : 0; break;
    case SelLevel::Edge:   total = mCtx.mesh ? mCtx.mesh->ENum() : 0; break;
    case SelLevel::Vertex: total = mCtx.mesh ? mCtx.mesh->VNum() : 0; break;
    default: break;
    }

    BitArray live = SelectSimilarEngine::CombineWithOriginal(mOriginalSel, similar, mParams.selMode);

    // Write live (no undo).  This updates the viewport as the user drags.
    SelectSimilarEngine::WriteLive(mCtx, live);

    UpdateCountLabel(live.NumberSet(), total);
}

void SelectSimilarAdjuster::UpdateCountLabel(int count, int total)
{
    wchar_t buf[64];
    const wchar_t* what = L"items";
    switch (mLevel)
    {
    case SelLevel::Face:   what = L"faces";   break;
    case SelLevel::Edge:   what = L"edges";   break;
    case SelLevel::Vertex: what = L"vertices"; break;
    default: break;
    }
    _snwprintf_s(buf, _TRUNCATE, L"%d of %d %s selected", count, total, what);
    SetWindowTextW(GetDlgItem(mHwnd, IDC_COUNT_LABEL), buf);
}
