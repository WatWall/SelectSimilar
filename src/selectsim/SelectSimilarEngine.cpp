#include "SelectSimilarEngine.h"
#include "SimilarCriteria.h"

#include <maxapi.h>
#include <plugapi.h>
#include <object.h>
#include <iEPolyMod.h>
#include <iepoly.h>
#include <mnmesh.h>

// theHold is provided transitively via max.h; no separate include needed.

// ------------------------------------------------------------------------------------------------
// Context detection (unchanged from PoC)
// ------------------------------------------------------------------------------------------------
bool SelectSimilarEngine::DetectContext(SelectSimilarContext& ctx)
{
    Interface* ip = GetCOREInterface();
    if (!ip) return false;

    if (ip->GetSubObjectLevel() <= 0) return false;

    BaseObject* cur = ip->GetCurEditObject();
    if (!cur) return false;

    // ---- Edit Poly MODIFIER ----
    if (cur->ClassID() == EDIT_POLY_MODIFIER_CLASS_ID &&
        cur->SuperClassID() == OSM_CLASS_ID)
    {
        ctx.isModifier = true;
        ctx.modifier   = static_cast<Modifier*>(cur);
        ctx.epMod      = GetEPolyModInterface(ctx.modifier);
        if (!ctx.epMod) return false;

        ModContextList mcList;
        INodeTab        nodes;
        ip->GetModContexts(mcList, nodes);
        if (nodes.Count() == 0) return false;
        ctx.node = nodes[0];

        const int sl = ctx.epMod->GetEPolySelLevel();
        switch (sl)
        {
        case EPM_SL_VERTEX:  ctx.level = SelLevel::Vertex; break;
        case EPM_SL_EDGE:    ctx.level = SelLevel::Edge;   break;
        case EPM_SL_BORDER:  ctx.level = SelLevel::Edge;   break;
        case EPM_SL_FACE:    ctx.level = SelLevel::Face;   break;
        case EPM_SL_ELEMENT: ctx.level = SelLevel::Face;   break;
        default:             return false;
        }

        ctx.mesh = ctx.epMod->EpModGetMesh(ctx.node);
        if (!ctx.mesh) return false;

        BitArray* sel = ctx.epMod->EpModGetSelection(MNM_SL_CURRENT, ctx.node);
        if (!sel) return false;
        ctx.currentSelection = *sel;

        ctx.valid = true;
        return true;
    }

    // ---- Editable Poly BASE OBJECT ----
    if (cur->ClassID() == EPOLYOBJ_CLASS_ID)
    {
        ctx.isModifier = false;
        ctx.baseObj    = cur;
        ctx.epoly      = GetEPolyInterface(cur);
        if (!ctx.epoly) return false;

        if (ip->GetSelNodeCount() > 0)
            ctx.node = ip->GetSelNode(0);

        const int sl = ctx.epoly->GetEPolySelLevel();
        switch (sl)
        {
        case EP_SL_VERTEX:  ctx.level = SelLevel::Vertex; break;
        case EP_SL_EDGE:    ctx.level = SelLevel::Edge;   break;
        case EP_SL_BORDER:  ctx.level = SelLevel::Edge;   break;
        case EP_SL_FACE:    ctx.level = SelLevel::Face;   break;
        case EP_SL_ELEMENT: ctx.level = SelLevel::Face;   break;
        default:            return false;
        }

        ctx.mesh = ctx.epoly->GetMeshPtr();
        if (!ctx.mesh) return false;

        if (ctx.level == SelLevel::Face)
            ctx.epoly->EpGetFacesByFlag(ctx.currentSelection, MN_SEL);
        else if (ctx.level == SelLevel::Edge)
            ctx.epoly->EpGetEdgesByFlag(ctx.currentSelection, MN_SEL);
        else
            ctx.epoly->EpGetVerticesByFlag(ctx.currentSelection, MN_SEL);

        ctx.valid = true;
        return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
// Low-level writers
// ------------------------------------------------------------------------------------------------
namespace {

// Writes the given BitArray to the modifier/object's selection and refreshes
// viewports.  Does NOT touch theHold, so callers control undo semantics.
void WriteRaw(const SelectSimilarContext& ctx, const BitArray& sel)
{
    if (ctx.isModifier)
    {
        ctx.epMod->EpModSetSelection(MNM_SL_CURRENT, const_cast<BitArray&>(sel), ctx.node);
        ctx.epMod->EpModLocalDataChanged(SELECT_CHANNEL);
        ctx.epMod->EpModRefreshScreen();
    }
    else if (ctx.mesh)
    {
        const MNMesh& m = *ctx.mesh;
        if (ctx.level == SelLevel::Face)
        {
            BitArray all(m.FNum());
            for (int i = 0; i < m.FNum(); ++i)
                if (!m.f[i].GetFlag(MN_DEAD)) all.Set(i);
            ctx.epoly->EpSetFaceFlags(all,      0,      MN_SEL, true);
            ctx.epoly->EpSetFaceFlags(sel,      MN_SEL, MN_SEL, true);
        }
        else if (ctx.level == SelLevel::Edge)
        {
            BitArray all(m.ENum());
            for (int i = 0; i < m.ENum(); ++i)
                if (!m.e[i].GetFlag(MN_DEAD)) all.Set(i);
            ctx.epoly->EpSetEdgeFlags(all,      0,      MN_SEL, true);
            ctx.epoly->EpSetEdgeFlags(sel,      MN_SEL, MN_SEL, true);
        }
        else
        {
            BitArray all(m.VNum());
            for (int i = 0; i < m.VNum(); ++i)
                if (!m.v[i].GetFlag(MN_DEAD)) all.Set(i);
            ctx.epoly->EpSetVertexFlags(all,    0,      MN_SEL, true);
            ctx.epoly->EpSetVertexFlags(sel,    MN_SEL, MN_SEL, true);
        }
        ctx.epoly->LocalDataChanged(SELECT_CHANNEL);
        ctx.epoly->RefreshScreen();
    }
}

} // anonymous namespace

void SelectSimilarEngine::WriteLive(const SelectSimilarContext& ctx, const BitArray& sel)
{
    WriteRaw(ctx, sel);
}

void SelectSimilarEngine::WriteFinal(const SelectSimilarContext& ctx,
                                     const BitArray& originalSel,
                                     const BitArray& finalSel,
                                     const wchar_t* undoName)
{
    // Approach B from SDK research: snapshot is the pre-state.  Begin a hold,
    // restore the original, apply the final, then Accept.  This collapses into
    // a single undo entry that, on Ctrl+Z, reverts to originalSel.
    theHold.Begin();
    WriteRaw(ctx, originalSel);
    WriteRaw(ctx, finalSel);
    theHold.Accept(undoName);
}

void SelectSimilarEngine::Revert(const SelectSimilarContext& ctx, const BitArray& originalSel)
{
    WriteRaw(ctx, originalSel);
}
