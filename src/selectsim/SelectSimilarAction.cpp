#include "SelectSimilarAction.h"
#include "SelectSimilarEngine.h"
#include "SelectSimilarComputations.h"
#include "SelectSimilarMenu.h"
#include "SelectSimilarAdjuster.h"
#include "SimilarCriteria.h"
#include "SimilarParams.h"
#include "../core/resource.h"

#include <maxapi.h>
#include <windows.h>

SelectSimilarActionTable* SelectSimilarActionTable::sInstance = nullptr;

SelectSimilarActionTable* SelectSimilarActionTable::GetInstance()
{
    if (!sInstance)
        sInstance = new SelectSimilarActionTable();
    return sInstance;
}

SelectSimilarActionTable::SelectSimilarActionTable()
    : ActionTable(SELECTSIM_ACTION_TABLE_ID,
                  kActionMainUIContext,
                  TSTR(_M("Select Similar")))
{
    HACCEL hDefaults = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_SELECTSIM_ACCEL));

    static const ActionDescription sActions[] = {
        { ID_ACTION_SELECT_SIMILAR,
          IDS_SELECTSIM_ACTION_DESC,
          IDS_SELECTSIM_ACTION_NAME,
          IDS_SELECTSIM_CATEGORY },
    };

    BuildActionTable(hDefaults,
                     static_cast<int>(_countof(sActions)),
                     sActions,
                     hInstance);

    SetCallback(this);

    if (IActionManager* am = GetCOREInterface()->GetActionManager())
    {
        am->RegisterActionTable(this);
        am->ActivateActionTable(this, SELECTSIM_ACTION_TABLE_ID);
    }
}

BOOL SelectSimilarActionTable::IsEnabled(int cmdId)
{
    if (cmdId != ID_ACTION_SELECT_SIMILAR) return FALSE;
    SelectSimilarContext ctx;
    return SelectSimilarEngine::DetectContext(ctx) ? TRUE : FALSE;
}

BOOL SelectSimilarActionTable::ExecuteAction(int id)
{
    if (id != ID_ACTION_SELECT_SIMILAR) return FALSE;
    DoSelectSimilar();
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
// Direct-apply path for ExactMatch criteria (Material, FlatSmooth, Seam).
// They don't need the adjuster dialog - we apply immediately with default
// params and a single undo entry.
// ------------------------------------------------------------------------------------------------
static void DirectApply(const SelectSimilarContext& ctx,
                        FaceCriterion fc, EdgeCriterion ec, VertexCriterion vc)
{
    SimilarParams p;
    p.compare = CompareMode::Equal;

    BitArray similar;
    switch (ctx.level)
    {
    case SelLevel::Face:   SelectSimilarComputations::ComputeFace  (ctx, fc, p, similar); break;
    case SelLevel::Edge:   SelectSimilarComputations::ComputeEdge  (ctx, ec, p, similar); break;
    case SelLevel::Vertex: SelectSimilarComputations::ComputeVertex(ctx, vc, p, similar); break;
    default: return;
    }

    const wchar_t* name = L"Select Similar";
    switch (ctx.level)
    {
    case SelLevel::Face:   name = GetDisplayName(fc); break;
    case SelLevel::Edge:   name = GetDisplayName(ec); break;
    case SelLevel::Vertex: name = GetDisplayName(vc); break;
    default: break;
    }

    SelectSimilarEngine::WriteFinal(ctx, ctx.currentSelection, similar, name);
}

// ------------------------------------------------------------------------------------------------
// Main dispatch
// ------------------------------------------------------------------------------------------------
void SelectSimilarActionTable::DoSelectSimilar()
{
    // If an adjuster dialog is already open, treat Shift+G as Apply+close
    // (handy when the user has mouse focus on the viewport).
    if (SelectSimilarAdjuster::IsOpen())
    {
        SelectSimilarAdjuster::Close();
        return;
    }

    SelectSimilarContext ctx;
    if (!SelectSimilarEngine::DetectContext(ctx)) return;

    FaceCriterion   faceCrit   = FaceCriterion::Material;
    EdgeCriterion   edgeCrit   = EdgeCriterion::Length;
    VertexCriterion vertCrit   = VertexCriterion::ConnectingEdges;
    if (!ShowCriterionPopup(ctx.level, faceCrit, edgeCrit, vertCrit)) return;

    // Exact-match criteria (Material, FlatSmooth, Seam) apply directly without a dialog.
    const bool wantsDialog =
        (ctx.level == SelLevel::Face)   ? NeedsAdjuster(faceCrit) :
        (ctx.level == SelLevel::Edge)   ? NeedsAdjuster(edgeCrit) :
        (ctx.level == SelLevel::Vertex) ? NeedsAdjuster(vertCrit) : false;

    if (!wantsDialog)
    {
        DirectApply(ctx, faceCrit, edgeCrit, vertCrit);
        return;
    }

    // Everything else: open the live-preview adjuster dialog.
    SelectSimilarAdjuster::Open(ctx, faceCrit, edgeCrit, vertCrit);
}
